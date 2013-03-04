#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include "polluxregs.h"
#include "time.h"

#ifndef __WIZ_LIB_H__
#define __WIZ_LIB_H__

extern unsigned char *fb0_8bit, *fb1_8bit; /* current buffers (8 bit) */
extern unsigned short *fb0_16bit, *fb1_16bit; /* current buffers (16 bit) */

#define wiz_video_color8(C,R,G,B) (wiz_video_RGB_palette[(C)].color=((R)>>3)<<11|((G)>>2)<<5|((B)>>3),wiz_video_RGB_palette[(C)].dirty=1)
#define wiz_video_color16(R,G,B)	(((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3)))
#define wiz_video_getr16(C) (((C)>>8)&0xF8)
#define wiz_video_getg16(C) (((C)>>3)&0xFC)
#define wiz_video_getb16(C) (((C)<<3)&0xF8)

extern int wiz_init(int bpp, int rate, int bits, int stereo);
extern void wiz_deinit(void);

#define BIT(number) (1<<(number))
enum {
	WIZ_A = BIT(20), //by simon
	WIZ_B = BIT(21),
	WIZ_X = BIT(22),
	WIZ_Y = BIT(23),
	WIZ_L = BIT(18),
	WIZ_R = BIT(19),
	WIZ_MENU   = BIT(16), // Help II (Start)
	WIZ_SELECT = BIT(17), // Help I  (Coin)
	WIZ_HOME   = BIT(11), // Home
	WIZ_HOLD   = BIT(10), // Hold
	WIZ_LEFT = BIT(12),
	WIZ_RIGHT = BIT(13),
	WIZ_UP = BIT(14),
	WIZ_DOWN = BIT(15),
	WIZ_VOLUP = BIT(04),
	WIZ_VOLDOWN = BIT(05)
};

#ifdef WIZ_TIMER
    #define clock_type clock_t
#else
    #define clock_type unsigned int
#endif

extern void wiz_set_clock(int speed); /* 200, 533, 650 */

extern unsigned int wiz_joystick_read(int n);
extern void wiz_video_flip(void);
extern void wiz_video_flip_single(void);

typedef struct wiz_palette { unsigned short color; unsigned short dirty; } wiz_palette;
extern wiz_palette wiz_video_RGB_palette[256];
extern void wiz_video_setpalette(void);

extern int wiz_sound_rate;
extern int wiz_sound_stereo;
extern int wiz_clock;

extern void wiz_timer_delay(clock_type ticks);
extern void wiz_sound_play(void *buff, int len);
extern unsigned int wiz_joystick_press (int n);
extern void wiz_sound_thread_mute(void);
extern void wiz_sound_thread_start(void);
extern void wiz_sound_thread_stop(void);
extern void wiz_sound_set_rate(int rate);
extern void wiz_sound_set_stereo(int stereo);
extern clock_type wiz_timer_read(void);
extern void wiz_timer_profile(void);

extern void wiz_set_video_mode(int bpp,int width,int height);

extern void wiz_printf(char* fmt, ...);
extern void wiz_printf_init(void);
extern void wiz_gamelist_text_out(int x, int y, char *eltexto);
extern void wiz_gamelist_text_out_fmt(int x, int y, char* fmt, ...);

extern void wiz_video_wait_vsync(void);

#ifdef MMUHACK
#include "warm.h"
#endif
#include "pollux_set.h"
#include "sys_cacheflush.h"

#endif
