#ifndef __COMMON_H__
#define __COMMON_H__

#include "../../psxcommon.h"
#include "../../psxhle.h"
#include "../../psxmem.h"
#include "../../r3000a.h"
#include "gte.h"

// From old psxmem.h
extern u32 psxRecLUT[0x010000];

#define PC_REC(x)	(psxRecLUT[(x) >> 16] + ((x) & 0xffff))
#define PC_REC32(x) (*(u32*)PC_REC(x))

#define psxMemReadS8 psxMemRead8
#define psxMemReadS16 psxMemRead16

#define BIAS_CYCLE_INC BIAS

#undef INLINE
#define INLINE inline

#define DEBUGF(aa) 

#if 0
#include <zlib.h>
#include <sys/types.h>

#include "psxTypes.h"

#ifdef __WIN32__
//#define INLINE __forceinline
  #define strncasecmp strnicmp
#else
  #define strnicmp strncasecmp
#endif


#ifndef INLINE
  #define INLINE inline
#endif

#define ClosePlugins()


#include "r3000a_new.h"

extern s32 Log;

void __Log(s8 *fmt, ...);

typedef struct {
	s8	Mcd1[256];
	s8	Mcd2[256];
	s8	Bios[256];
	s8	BiosDir[256];
	s32 Xa;
	s32 Sio;
	s32 Mdec;
	s32 PsxAuto;
	s32 PsxType; // ntsc - 0 | pal - 1
	s32 QKeys;
	s32 Cdda;
	s32 Cpu;
	s32 PsxOut;
	s32 SpuIrq;
	s32 RCntFix;
	s32 VSyncWA;
	s32 HLE;
} PsxConfig;

extern PsxConfig Config;

extern s32 LoadCdBios;
extern s32 StatesC;
extern s32 cdOpenCase;
extern s32 NetOpened;
extern int scale_height;
extern unsigned long mcdst;

#define gzfreeze(ptr, size) \
	if (Mode == 1) gzwrite(f, ptr, size); \
	if (Mode == 0) gzread(f, ptr, size);

#define gzfreezel(ptr) gzfreeze(ptr, sizeof(ptr))

  #define BIAS_SIO			(200) 				/* REQUIRED FOR REGULAR INTERPRETERS! */
  #define BIAS			      (1)			      /* Optimization: Adds weight to the root counters balance. Higher values increase speed.							*/
//#define	CDROM_CYCLES		(PSXCLK/75)   /* Optimization: This is the number of cycles needed for the cdrom interrupt to begin. Lower values for more speed.	*/
  #define BIAS_CYCLE_INC	(PsxCycleMult)		/* Optimization: Increments the cycle counter by this value.														*/

  //  Esto puede parecer raro pero no lo es tanto.
  static  const u32 uPsxClockBase   = 338688;   //  33.8688mhz de reloj por defecto, divididos por 2 ciclos por instrucción de media. Así no evitamos incrementos raros.
  static  const u32 uDefaultPsxMult = 100;         //  % de cpu emulada, repercute en cada cuantos ciclos se ejecutan las interrupciones.
  extern        u32 PsxClockMult;                 //  Por defecto uDefaultPsxMult;
  extern        u32 PsxClock;                     //  Por defecto uPsxClockBase*uDefaultPsxMult;
  extern        u32 PsxPALClock;                  //  Por defecto PsxClock/50;
  extern        u32 PsxNTSCClock;                 //  Por defecto PsxClock/60;
  extern        u32 PsxCDClock;                   //  Por defecto PsxClock/75;

  INLINE  void  UpdateClock (u32 uClockMult = uDefaultPsxMult)
  {
    PsxClockMult = uClockMult;
    PsxClock     = uPsxClockBase * PsxClockMult;
    PsxPALClock  = PsxClock / 50;
    PsxNTSCClock = PsxClock / 60;
    PsxCDClock   = PsxClock / 75;
  }

/* TO BE PLACED IN A HEADER FOR THE OFFICIAL MENU SYSTEM */
extern s8 iphonefile[256];
s32 SelectGame();

#include "minimal.h"
#include "system.h"
#include "counters_new.h"
#include "gte_pcsx.h"
#include "gte_mame.h"
#include "gpuAPI/gpuAPI.h"
#include "hw.h"
#include "mem.h"
#include "bios.h"
#include "dma.h"
#include "hle.h"
#include "mdec.h"
#include "cdrom.h"
#include "sio.h"
#include "spu.h"
#include "components.h"
#include "decode_xa.h"
#include "misc.h"
#include "debug.h"
#include "profiler.h"
#endif

#endif /* __COMMON_H__ */
