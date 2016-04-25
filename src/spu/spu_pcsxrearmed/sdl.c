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

//senquack - in sdl/port.cpp, dsmagin was using SOUND_BUFFER_SIZE = 1024*4*4 = 16384, 
//		and I've used that here too as it seems to work well and taking modulus
//		is fast bitwise-op. Perhaps make size a commandline option?
#define SOUND_BUFFER_SIZE 16384
#define ROOM_IN_BUFFER (SOUND_BUFFER_SIZE - buffered_bytes)
static unsigned *sound_buffer = NULL;
static unsigned int buf_read_pos = 0;
static unsigned int buf_write_pos = 0;
static unsigned buffered_bytes = 0;
static unsigned sync_audio = 0;         // Does emu yield when audio output buffer is full?

//senquack - Old mutex vars we added to sync audio (no longer used unless
//           debugging/verifying sound with '-use_old_audio_out_mutex' switch)
SDL_mutex *sound_mutex = NULL;
SDL_cond *sound_cv = NULL;

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
static void SOUND_FillAudio(void *unused, Uint8 *stream, int len) {
	//senquack-adapted from dsmagin's sdl/port.cpp, but if we have less buffered
	// than requested, just write what we have and zero-fill the remainder:

	uint8_t *out_buf = (uint8_t *)stream;
	uint8_t *in_buf = (uint8_t *)sound_buffer;

	//if (!sound_running) return;

	if (sound_mutex) SDL_LockMutex(sound_mutex);

	int bytes_to_copy = (len > buffered_bytes) ? buffered_bytes : len;

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

	if (SDL_WasInit(SDL_INIT_EVERYTHING & ~SDL_INIT_AUDIO)) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	} else {
		SDL_Quit();
	}
}

static int sdl_init(void) {
	SDL_AudioSpec				spec;

	//senquack
	//if (pSndBuffer != NULL) return -1;
	if (sound_buffer != NULL) return -1;

	InitSDL();

	spec.freq = 44100;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	//senquack - TODO: PCSX_ReARMed had 512 hard-coded, but is that suitable for PCSX4ALL on low-end handhelds?
	//spec.samples = 512;
	spec.samples = 1024;	//senquack - made 1024 for now, to match port.cpp's settings ..TODO: Makefile should define
	spec.callback = SOUND_FillAudio;

	if (SDL_OpenAudio(&spec, NULL) < 0) {
		DestroySDL();
		return -1;
	}

	//senquack - Adapted from dsmagin's sdl/port.cpp:
	//senquack - use calloc, not malloc, to get zero-filled buffer:
	//iBufSize = BUFFER_SIZE;
	//pSndBuffer = (short *)malloc(iBufSize * sizeof(short));
	//if (pSndBuffer == NULL) {
	//	SDL_CloseAudio();
	//	return -1;
	//}
	//iReadPos = 0;
	//iWritePos = 0;

	sound_buffer = (unsigned *)calloc(SOUND_BUFFER_SIZE,1);
	if (sound_buffer == NULL) {
		SDL_CloseAudio();
		return -1;
	}
	buf_read_pos = 0;
	buf_write_pos = 0;

	sync_audio = Config.SyncAudio;

	//senquack - Added mutex based on dsmagin's port.cpp code. (Now deprecated
	//           and left in for debug/verification of newer mutex-free buffer code)
	if (sync_audio) {
		sound_mutex = SDL_CreateMutex();
		sound_cv = SDL_CreateCond();
		printf("SPU using SDL audio output mutex\n");
	}

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

//senquack - Adapted from dsmagin's sdl/port.cpp sound_mix() function, but
//           waits until there is room in buffer for full copy
static void sdl_feed(void *pSound, int lBytes) {
//	sound_running = 1;

	if (sound_mutex)
		SDL_LockMutex(sound_mutex);

	int bytes_to_copy = lBytes;

	if (sound_mutex) {
		//senquack - Block until we have all the room we need:
		while (ROOM_IN_BUFFER < lBytes)
			SDL_CondWait(sound_cv, sound_mutex);
	} else {
		// Just drop the samples that cannot fit:
		if (ROOM_IN_BUFFER == 0) return;
		if (bytes_to_copy > ROOM_IN_BUFFER)
			bytes_to_copy = ROOM_IN_BUFFER;
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

void out_register_sdl(struct out_driver *drv)
{
	drv->name = "sdl";
	drv->init = sdl_init;
	drv->finish = sdl_finish;
	drv->busy = sdl_busy;
	drv->feed = sdl_feed;
}
