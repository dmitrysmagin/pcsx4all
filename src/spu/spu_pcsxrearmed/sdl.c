/* SDL Driver for P.E.Op.S Sound Plugin
 * Copyright (c) 2010, Wei Mingzhi <whistler_wmz@users.sf.net>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <SDL.h>
#include <stdint.h>
#include "out.h"
#include "spu_config.h"  // senquack - To get spu settings
#include "psxcommon.h"   // senquack - To get emu settings

//senquack - Original PCSX-Rearmed dfsound SPU variables (for reference):
//#define BUFFER_SIZE		22050
//short			*pSndBuffer = NULL;
//int				iBufSize = 0;
//volatile int	iReadPos = 0, iWritePos = 0;

//senquack - in sdl/port.cpp, dsmagin was using SOUND_BUFFER_SIZE = 16384 bytes
//		and I've used that here too as it seems to work well and taking modulus
//		is fast bitwise-op. Perhaps make size a commandline option (enforcing
//		power-of-two so we can do: (&=SOUND_BUFFER_SIZE-1) to modulus by variable?
#define SOUND_BUFFER_SIZE 16384        // Size in bytes
#define ROOM_IN_BUFFER (SOUND_BUFFER_SIZE - buffered_bytes)
static unsigned *sound_buffer = NULL;  // Sample ring buffer
static unsigned int buf_read_pos = 0;
static unsigned int buf_write_pos = 0;
static unsigned waiting_to_feed = 0;   // Set to 1 when emu is waiting for room in output buffer

#ifdef DEBUG_FEED_RATIO
static void update_feed_ratio(void);
float cur_feed_ratio = 1.0f;
static unsigned int new_ratio_val = 0;
static unsigned int total_bytes_consumed = 0;
static unsigned int total_bytes_fed = 0;
static unsigned int dropped_bytes = 0;
static unsigned int missed_bytes = 0;
#endif

//VARS THAT ARE SHARED BETWEEN MAIN THREAD AND AUDIO-CALLBACK THREAD (fence!):
static unsigned buffered_bytes = 0;    // How many bytes are in the ring buffer

//senquack - New semaphore for atomic versions of feed/callback functions:
static SDL_sem *sound_sem = NULL;

//senquack - Old mutex vars we added to sync audio (no longer used unless
//           debugging/verifying sound with '-use_old_audio_out_mutex' switch)
static SDL_mutex *sound_mutex = NULL;
static SDL_cond *sound_cv = NULL;

////////////////////////////////////////
// BEGIN SDL AUDIO CALLBACK FUNCTIONS //
////////////////////////////////////////	

//senquack - Original PCSX-Rearmed dfsound SPU code (for reference):
//static void SOUND_FillAudio(void *unused, Uint8 *stream, int len) {
//	short *p = (short *)stream;
//
//	len /= sizeof(short);
//
//	while (iReadPos != iWritePos && len > 0) {
//		*p++ = pSndBuffer[iReadPos++];
//		if (iReadPos >= iBufSize) iReadPos = 0;
//		--len;
//	}
//
//	// Fill remaining space with zero
//	while (len > 0) {
//		*p++ = 0;
//		--len;
//	}
//}

//senquack-Newer SDL audio callback that uses GCC atomic var
static void SOUND_FillAudio_Atomic(void *unused, Uint8 *stream, int len) {
	uint8_t *out_buf = (uint8_t *)stream;
	uint8_t *in_buf = (uint8_t *)sound_buffer;

	unsigned bytes_to_copy = (len > buffered_bytes) ? buffered_bytes : len;

#ifdef DEBUG_FEED_RATIO
	missed_bytes += len - bytes_to_copy;
	total_bytes_consumed += len;
	update_feed_ratio();
#endif

	if (bytes_to_copy > 0) {
		if (buf_read_pos + bytes_to_copy <= SOUND_BUFFER_SIZE ) {
			memcpy(out_buf, in_buf + buf_read_pos, bytes_to_copy);
		} else {
			unsigned tail = SOUND_BUFFER_SIZE - buf_read_pos;
			memcpy(out_buf, in_buf + buf_read_pos, tail);
			memcpy(out_buf + tail, in_buf, bytes_to_copy - tail);
		}

		buf_read_pos = (buf_read_pos + bytes_to_copy) % SOUND_BUFFER_SIZE;

		// Atomically decrement 'buffered_bytes' by 'bytes_to_copy'
		// TODO: If ever ported to SDL2.0, its API offers portable atomics:
		__sync_fetch_and_sub(&buffered_bytes, bytes_to_copy);
	}

	// If the callback asked for more samples than we had, zero-fill remainder:
	if (len - bytes_to_copy > 0) {
		memset(out_buf + bytes_to_copy, 0, len - bytes_to_copy);
		//printf("SDL audio callback underrun by %d bytes\n", len - bytes_to_copy);
	}

	// Signal emu thread that room is available:
	if (waiting_to_feed)
		SDL_SemPost(sound_sem);
}

//senquack-Older SDL audio callback that used a mutex and condition var to
//         signal to main thread that there's more room in sound buffer.
//		   If Config.SyncAudio==0, sound_mutex ptr will remain NULL and
//         sdl_feed() will just drop samples there's no space for.
static void SOUND_FillAudio_Mutex(void *unused, Uint8 *stream, int len) {
	//senquack-adapted from dsmagin's sdl/port.cpp, but if we have less buffered
	// than requested, just write what we have and zero-fill the remainder:

	uint8_t *out_buf = (uint8_t *)stream;
	uint8_t *in_buf = (uint8_t *)sound_buffer;

	//if (!sound_running) return;

	if (sound_mutex) SDL_LockMutex(sound_mutex);

	int bytes_to_copy = (len > buffered_bytes) ? buffered_bytes : len;

#ifdef DEBUG_FEED_RATIO
	missed_bytes += len - bytes_to_copy;
	total_bytes_consumed += len;
	update_feed_ratio();
#endif

	if (buffered_bytes > 0) {
		if (buf_read_pos + bytes_to_copy <= SOUND_BUFFER_SIZE ) {
			memcpy(out_buf, in_buf + buf_read_pos, bytes_to_copy);
		} else {
			int tail = SOUND_BUFFER_SIZE - buf_read_pos;
			memcpy(out_buf, in_buf + buf_read_pos, tail);
			memcpy(out_buf + tail, in_buf, bytes_to_copy - tail);
		}
		buf_read_pos = (buf_read_pos + bytes_to_copy) % SOUND_BUFFER_SIZE;
		buffered_bytes -= bytes_to_copy;
	}

	//If the callback asked for more samples than we had, zero-fill remainder:
	if (len - bytes_to_copy > 0)
		memset(out_buf + bytes_to_copy, 0, len - bytes_to_copy);

	if (sound_mutex) {
		SDL_CondSignal(sound_cv);
		SDL_UnlockMutex(sound_mutex);
	}
}
//////////////////////////////////////
// END SDL AUDIO CALLBACK FUNCTIONS //
//////////////////////////////////////


static void InitSDL() {
	if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
		SDL_InitSubSystem(SDL_INIT_AUDIO);
	} else {
		SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
	}
}

static void DestroySDL() {
	//senquack - Added mutex based on dsmagin's sdl/port.cpp code:
	if (sound_mutex && sound_cv) {
		SDL_LockMutex(sound_mutex);
		buffered_bytes = SOUND_BUFFER_SIZE;
		SDL_CondSignal(sound_cv);
		SDL_UnlockMutex(sound_mutex);
		SDL_Delay(100);

		SDL_DestroyCond(sound_cv);
		SDL_DestroyMutex(sound_mutex);
	}

	//senquack - added sempahore:
	if (sound_sem)
		SDL_DestroySemaphore(sound_sem);

	if (SDL_WasInit(SDL_INIT_EVERYTHING & ~SDL_INIT_AUDIO)) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	} else {
		SDL_Quit();
	}
}

static int sdl_init(void) {
	//senquack
	//if (pSndBuffer != NULL) return -1;
	if (sound_buffer != NULL) return -1;

	InitSDL();

	SDL_AudioSpec				spec;

	//senquack - Added mutex based on dsmagin's port.cpp code:
	//           (Now deprecated and only left in for debug/verification of
	//            newer mutex-free atomic buffer code using sempahore)
	if (spu_config.iUseOldAudioMutex) {
		if (Config.SyncAudio) {
			sound_mutex = SDL_CreateMutex();
			sound_cv = SDL_CreateCond();
			if (sound_mutex)
				printf("Created SDL audio output mutex successfully.\n");
			else
				printf("Failed to create SDL audio output mutex, audio will not be synced.\n");
		}
		spec.callback = SOUND_FillAudio_Mutex;
	} else {
		if (Config.SyncAudio) {
			sound_sem = SDL_CreateSemaphore(0);
			if (sound_sem)
				printf("Created SDL audio output semaphore successfully.\n");
			else
				printf("Failed to create SDL audio output semaphore, audio will not be synced.\n");
		}
		spec.callback = SOUND_FillAudio_Atomic;
	}

	spec.freq = 44100;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	//senquack - TODO: PCSX_ReARMed had 512 hard-coded, but is that suitable for PCSX4ALL on low-end handhelds?
	//spec.samples = 512;
	spec.samples = 1024;	//senquack - made 1024 for now, to match port.cpp's settings ..TODO: Makefile should define

	if (SDL_OpenAudio(&spec, NULL) < 0) {
		DestroySDL();
		return -1;
	}

	//iBufSize = BUFFER_SIZE;
	//pSndBuffer = (short *)malloc(iBufSize * sizeof(short));
	//if (pSndBuffer == NULL) {
	//	SDL_CloseAudio();
	//	return -1;
	//}
	//iReadPos = 0;
	//iWritePos = 0;

	//senquack - use calloc, not malloc, to get zero-filled buffer:
	sound_buffer = (unsigned *)calloc(SOUND_BUFFER_SIZE,1);
	if (sound_buffer == NULL) {
		printf("-> ERROR: SPU plugin could not allocate %d-byte sound buffer\n", SOUND_BUFFER_SIZE);
		SDL_CloseAudio();
		return -1;
	}

	buf_read_pos = 0;
	buf_write_pos = 0;
	SDL_PauseAudio(0);
	return 0;
}

static void sdl_finish(void) {
	//senquack
	//if (pSndBuffer == NULL) return;
	if (sound_buffer == NULL) return;

	SDL_CloseAudio();
	DestroySDL();

	//senquack
	//free(pSndBuffer);
	//pSndBuffer = NULL;
	if (sound_buffer) free(sound_buffer);
	sound_buffer = NULL;
}

//senquack - When spu_config.iTempo option is set, this is used to determine
//			 when spu.c decides to fake less samples having been written in
//			 order to force more to be generated (pcsxReARMed hack for slow
//		     devices)
//senquack - Original PCSX-Rearmed dfsound SPU code (for reference):
#if 0
static int sdl_busy(void) {
	int size;
	if (pSndBuffer == NULL) return 1;
	size = iReadPos - iWritePos;
	if (size <= 0) size += iBufSize;
	if (size < iBufSize / 2) return 1;
	return 0;
}
#endif //0
static int sdl_busy(void) {
	//senquack - TODO - this might be a good place to investigate when improving audio clicking
	if ((ROOM_IN_BUFFER < SOUND_BUFFER_SIZE/2) || sound_buffer == NULL)
		return 1;

	return 0;
}

////////////////////////////////////////////////
// BEGIN EMU SPU -> SDL BUFFER FILL FUNCTIONS //
////////////////////////////////////////////////

//Feed samples from emu into intermediate buffer
//senquack - original pcsxrearmed code:
#if 0
static void sdl_feed(void *pSound, int lBytes) {
	short *p = (short *)pSound;

	if (pSndBuffer == NULL) return;

	while (lBytes > 0) {
		if (((iWritePos + 1) % iBufSize) == iReadPos) break;

		pSndBuffer[iWritePos] = *p++;

		++iWritePos;
		if (iWritePos >= iBufSize) iWritePos = 0;

		lBytes -= sizeof(short);
	}
}
#endif //0

//senquack-Newer sdl_feed() function that uses a shared atomic var to
//         coordinate with SDL audio callback function.
static void sdl_feed_atomic(void *pSound, int lBytes) {
	//sound_running = 1;

#ifdef DEBUG_FEED_RATIO
	total_bytes_fed += lBytes;
#endif

	unsigned bytes_to_copy = lBytes;

	if (sound_sem) {
		while (ROOM_IN_BUFFER < lBytes) {
			//senquack - Wait until semaphore is posted by audio callback:
			waiting_to_feed = 1;
			SDL_SemWait(sound_sem);
		}
		waiting_to_feed = 0;
	} else {
		// Just drop the samples that cannot fit:
		if (ROOM_IN_BUFFER == 0) {
#ifdef DEBUG_FEED_RATIO
			dropped_bytes += bytes_to_copy;
#endif
			return;
		}

		if (bytes_to_copy > ROOM_IN_BUFFER) bytes_to_copy = ROOM_IN_BUFFER;

#ifdef DEBUG_FEED_RATIO
		dropped_bytes += lBytes - bytes_to_copy;
#endif
	}

	uint8_t *in_buf = (uint8_t *)pSound;
	uint8_t *out_buf = (uint8_t *)sound_buffer;

	if (buf_write_pos + bytes_to_copy <= SOUND_BUFFER_SIZE ) {
		memcpy(out_buf + buf_write_pos, in_buf, bytes_to_copy);
	} else {
		int tail = SOUND_BUFFER_SIZE - buf_write_pos;
		memcpy(out_buf + buf_write_pos, in_buf, tail);
		memcpy(out_buf, in_buf + tail, bytes_to_copy - tail);
	}

	buf_write_pos = (buf_write_pos + bytes_to_copy) % SOUND_BUFFER_SIZE;

	// Atomically increment 'buffered_bytes' by 'bytes_to_copy':
	// TODO: If ever ported to SDL2.0, its API offers portable atomics:
	__sync_fetch_and_add(&buffered_bytes, bytes_to_copy);
}

//senquack-Older sdl_feed() function that used a mutex and condition var to
//         coordinate with SDL audio callback function.
//         If Config.SyncAudio==0, sound_mutex ptr will remain NULL and
//         this function will just drop samples there's no space for.
//         Primarily adapted from dsmagin's port/sdl/port.cpp sound_mix()
static void sdl_feed_mutex(void *pSound, int lBytes) {
//	sound_running = 1;

#ifdef DEBUG_FEED_RATIO
	total_bytes_fed += lBytes;
#endif

	if (sound_mutex) SDL_LockMutex(sound_mutex);

	int bytes_to_copy = lBytes;

	if (sound_mutex) {
		// Block until we have all the room we need:
		while (ROOM_IN_BUFFER < lBytes) SDL_CondWait(sound_cv, sound_mutex);
	} else {
		// Just drop the samples that cannot fit:
		if (ROOM_IN_BUFFER == 0) {
#ifdef DEBUG_FEED_RATIO
			dropped_bytes += bytes_to_copy;
#endif
			return;
		}


		if (bytes_to_copy > ROOM_IN_BUFFER) bytes_to_copy = ROOM_IN_BUFFER;

#ifdef DEBUG_FEED_RATIO
		dropped_bytes += lBytes - bytes_to_copy;
#endif
	}

	uint8_t *in_buf = (uint8_t *)pSound;
	uint8_t *out_buf = (uint8_t *)sound_buffer;

	if (buf_write_pos + bytes_to_copy <= SOUND_BUFFER_SIZE ) {
		memcpy(out_buf + buf_write_pos, in_buf, bytes_to_copy);
	} else {
		int tail = SOUND_BUFFER_SIZE - buf_write_pos;
		memcpy(out_buf + buf_write_pos, in_buf, tail);
		memcpy(out_buf, in_buf + tail, bytes_to_copy - tail);
	}
	buf_write_pos = (buf_write_pos + bytes_to_copy) % SOUND_BUFFER_SIZE;
	buffered_bytes += bytes_to_copy;

	if (sound_mutex) {
		SDL_CondSignal(sound_cv);
		SDL_UnlockMutex(sound_mutex);
	}
}


//////////////////////////////////////////////
// END EMU SPU -> SDL BUFFER FILL FUNCTIONS //
//////////////////////////////////////////////

#ifdef DEBUG_FEED_RATIO
static void update_feed_ratio(void) {
	const int calls_between_new_ratio = 5;
	static int calls_until_new_ratio = calls_between_new_ratio;
	calls_until_new_ratio--;
	if (calls_until_new_ratio > 0)
		return;

	// Avoid possible div-by-zero:
	if (total_bytes_consumed == 0)
		total_bytes_consumed = 1;

	calls_until_new_ratio = calls_between_new_ratio;
	cur_feed_ratio = (float)total_bytes_fed / (float)total_bytes_consumed;
	new_ratio_val = 1;
	total_bytes_fed = total_bytes_consumed = 0;

	printf("fr: %f   buf: %d   drop: %d   miss: %d\n", cur_feed_ratio, buffered_bytes, dropped_bytes, missed_bytes);

	dropped_bytes = missed_bytes = 0;
}
#endif //DEBUG_FEED_RATIO

void out_register_sdl(struct out_driver *drv)
{
	drv->name = "sdl";
	drv->init = sdl_init;
	drv->finish = sdl_finish;
	drv->busy = sdl_busy;
	
	//senquack - newer atomic feed function is default:
	if (spu_config.iUseOldAudioMutex)
		drv->feed = sdl_feed_mutex;
	else
		drv->feed = sdl_feed_atomic;
}
