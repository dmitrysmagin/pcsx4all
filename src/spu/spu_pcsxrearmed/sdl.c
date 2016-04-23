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

//senquack - to get access to emu config:
#include "psxcommon.h"

//senquack - Original pcsxrearmed code:
//#define BUFFER_SIZE		22050
//short			*pSndBuffer = NULL;
//int				iBufSize = 0;
//volatile int	iReadPos = 0, iWritePos = 0;

//senquack - in sdl/port.cpp, dsmagin was using SOUND_BUFFER_SIZE = 1024*4*4 = 16384, 
//		and I've used that here too as it seems to work well and taking modulus
//		is fast bitwise-op. Perhaps make size a commandline option?
#define SOUND_BUFFER_SIZE 16384
static unsigned *sound_buffer = NULL;
static unsigned int buf_read_pos = 0;
static unsigned int buf_write_pos = 0;
static unsigned buffered_bytes = 0;
SDL_mutex *sound_mutex = NULL;
SDL_cond *sound_cv = NULL;


//senquack - Original pcsxrearmed code:
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
	//senquack-same code as dsmagin's sdl/port.cpp, but if we have less buffered
	// than requested, just write what we have and zero-fill the remainder:

	uint8_t *data = (uint8_t *)stream;
	uint8_t *buffer = (uint8_t *)sound_buffer;

	//if (!sound_running) return;

	if (sound_mutex) SDL_LockMutex(sound_mutex);

	int bytes_to_write = (len > buffered_bytes) ? buffered_bytes : len;

	if (buffered_bytes > 0) {
		if (buf_read_pos + bytes_to_write <= SOUND_BUFFER_SIZE ) {
			memcpy(data, buffer + buf_read_pos, bytes_to_write);
		} else {
			int tail = SOUND_BUFFER_SIZE - buf_read_pos;
			memcpy(data, buffer + buf_read_pos, tail);
			memcpy(data + tail, buffer, bytes_to_write - tail);
		}
		buf_read_pos = (buf_read_pos + bytes_to_write) % SOUND_BUFFER_SIZE;
		buffered_bytes -= bytes_to_write;
	}

	//If the callback asked for more samples than we had, zero-fill remainder:
	if (len - bytes_to_write > 0)
		memset(data + bytes_to_write, 0, len - bytes_to_write);

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

	//senquack - Added mutex based on dsmagin's port.cpp code:
	if (Config.SyncAudio) {
		sound_mutex = SDL_CreateMutex();
		sound_cv = SDL_CreateCond();
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
static int sdl_busy(void) {
	//senquack - original pcsxrearmed code:
	//int size;
	//if (pSndBuffer == NULL) return 1;
	//size = iReadPos - iWritePos;
	//if (size <= 0) size += iBufSize;
	//if (size < iBufSize / 2) return 1;

	//senquack - TODO - this might be a good place to investigate when improving audio clicking
	if (sound_buffer == NULL) return 1;
	int size = buf_read_pos - buf_write_pos;
	if (size <= 0) size += SOUND_BUFFER_SIZE;
	if (size < SOUND_BUFFER_SIZE / 2) return 1;

	return 0;
}

static void sdl_feed(void *pSound, int lBytes) {
	//senquack - original pcsxrearmed code:
#if 0
	short *p = (short *)pSound;

	if (pSndBuffer == NULL) return;

	while (lBytes > 0) {
		if (((iWritePos + 1) % iBufSize) == iReadPos) break;

		pSndBuffer[iWritePos] = *p++;

		++iWritePos;
		if (iWritePos >= iBufSize) iWritePos = 0;

		lBytes -= sizeof(short);
	}
#endif

	//senquack - Adapted from dsmagin's sdl/port.cpp (known to work, but I
	//			 made one more similar to his sound_mix() function that
	//			 uses memcpy
#if 0
	uint8_t *data = (uint8_t *)pSound;
	uint8_t *buffer = (uint8_t *)sound_buffer;

	//sound_running = 1;

	if (sound_mutex)
		SDL_LockMutex(sound_mutex);

	int i;
	for (i = 0; i < lBytes; i += 4) {
		if (sound_mutex) {
			//Block until there is room in the output buffer
			// ORIGINAL DSMAGIN CODE:
			//while (buffered_bytes == SOUND_BUFFER_SIZE)
			//	SDL_CondWait(sound_cv, sound_mutex);

			//senquack - we should wait until we're sure we have room
			//			 for all the bytes we are to write:
			while (SOUND_BUFFER_SIZE - buffered_bytes < lBytes)
				SDL_CondWait(sound_cv, sound_mutex);
		} else {
			if (buffered_bytes == SOUND_BUFFER_SIZE)
				return; // just drop samples
		}

		*(int*)((uint8_t*)(buffer + buf_write_pos)) = *(int*)((uint8_t*)(data + i));
		//memcpy(buffer + buf_write_pos, data + i, 4);
		buf_write_pos = (buf_write_pos + 4) % SOUND_BUFFER_SIZE;
		buffered_bytes += 4;
	}

	if (sound_mutex) {
		SDL_CondSignal(sound_cv);
		SDL_UnlockMutex(sound_mutex);
	}
#endif //0

//	sound_running = 1;

	if (sound_mutex)
		SDL_LockMutex(sound_mutex);

	int bytes_to_write = lBytes;

	if (sound_mutex) {
		//senquack - Block until we have all the room we need:
		while (SOUND_BUFFER_SIZE - buffered_bytes < lBytes)
			SDL_CondWait(sound_cv, sound_mutex);
	} else {
		// Just drop the samples that cannot fit:
		int room_in_buffer = SOUND_BUFFER_SIZE - buffered_bytes;
		if (room_in_buffer == 0) return;
		if (bytes_to_write > room_in_buffer)
			bytes_to_write = room_in_buffer;
	}

	uint8_t *data = (uint8_t *)pSound;
	uint8_t *buffer = (uint8_t *)sound_buffer;

	if (buf_write_pos + bytes_to_write <= SOUND_BUFFER_SIZE ) {
		memcpy(buffer + buf_write_pos, data, bytes_to_write);
	} else {
		int tail = SOUND_BUFFER_SIZE - buf_write_pos;
		memcpy(buffer + buf_write_pos, data, tail);
		memcpy(buffer, data + tail, bytes_to_write - tail);
	}
	buf_write_pos = (buf_write_pos + bytes_to_write) % SOUND_BUFFER_SIZE;
	buffered_bytes += bytes_to_write;

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
