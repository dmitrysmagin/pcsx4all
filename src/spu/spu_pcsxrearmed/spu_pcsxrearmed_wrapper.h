//senquack - A wrapper that allows our version of pcsx_rearmed/plugins/dfsound/*
//			 (renamed as spu/spu_pcsxrearmed/) to work with PCSX4ALL.
//			
//           Credit goes to Notaz, PCSX_ReArmed, and PCSX_Reloaded teams for vastly
//           improving the original plugin made by Pete Bernert.
//
//           NOTE: see README.txt for information about this plugin, as well as
//           https://github.com/notaz/pcsx_rearmed.git

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#ifndef SPU_PCSXREARMED_WRAPPER_H
#define SPU_PCSXREARMED_WRAPPER_H

//senquack - so we can set some sensible default config values:
#include "spu_config.h"

//senquack - Some of the ReARMed SPU functions read the "cycles" value from
//           psxRegs.cycle, and I've provided a pointer to it in r3000a.cpp:
extern const uint32_t* const psxRegs_cycle_valptr;

// SPU Functions
#ifdef __cplusplus
extern "C" {
#endif
extern long SPUinit(void);				
extern long SPUopen(void);				
extern long SPUshutdown(void);	
extern long SPUclose(void);			
extern void SPUwriteRegister(unsigned long, unsigned short, unsigned int);
extern unsigned short SPUreadRegister(unsigned long);
extern void SPUwriteDMA(unsigned short);
extern unsigned short SPUreadDMA(void);
extern void SPUwriteDMAMem(unsigned short *, int, unsigned int);
extern void SPUreadDMAMem(unsigned short *, int, unsigned int);
extern void SPUplayADPCMchannel(xa_decode_t *);
extern int  SPUplayCDDAchannel(short *, int);
extern void SPUregisterCallback(void (CALLBACK *callback)(void));
extern void SPUregisterScheduleCb(void (CALLBACK *callback)(unsigned int cycles_after));
extern long SPUconfigure(void);
extern long SPUfreeze(uint32_t, SPUFreeze_t *, uint32_t);
extern void SPUasync(uint32_t, uint32_t);
#ifdef __cplusplus
}
#endif

// senquack - unused by PCSX4ALL:
//extern void SPUplaySample(unsigned char);   
//extern long SPUtest(void);
//extern void SPUabout(void);

static inline long SPU_init(void)
{
    //senquack - TODO: provide a way to alter default/current configuration:
	// ORIGINAL PCSX_ReARMed spu defaults:
#if 0
    // SOME SENSIBLE DEFAULTS:
	spu_config.iUseReverb = 1;
	spu_config.iUseInterpolation = 1;
	spu_config.iXAPitch = 0;
	spu_config.iVolume = 768;
	spu_config.iTempo = 0;
	spu_config.iUseThread = 1; // no effect if only 1 core is detected
    // LOW-END DEVICE:
#ifdef HAVE_PRE_ARMV7 /* XXX GPH hack */
	spu_config.iUseReverb = 0;
	spu_config.iUseInterpolation = 0;
	spu_config.iTempo = 1;
#endif
#endif //0

    // PCSX4ALL defaults:
	spu_config.iUseReverb = 1;
	spu_config.iUseInterpolation = 1;
	spu_config.iXAPitch = 0;
	spu_config.iVolume = 768;
	spu_config.iUseThread = 1; // no effect if only 1 core is detected
	spu_config.iUseFixedUpdates = 1;    // This is always set to 1 in libretro's pcsxReARMed
#if defined(WIZ) || defined(CAANOO) || defined(GCW_ZERO) || defined(HAVE_PRE_ARMV7)
	spu_config.iUseReverb = 0;
	spu_config.iUseInterpolation = 0;
	spu_config.iTempo = 1;     // see note below
#endif

	//senquack - NOTE REGARDING iTempo config var above
    // From thread https://pyra-handheld.com/boards/threads/pcsx-rearmed-r22-now-using-the-dsp.75388/
    // Notaz says that setting iTempo=1 restores pcsxreARMed SPU's old behavior, which allows slow emulation
    // to not introduce audio dropouts (at least I *think* he's referring to iTempo config setting)
    // "Probably the main change is SPU emulation, there were issues in some games where effects were wrong,
    //  mostly Final Fantasy series, it should be better now. There were also sound sync issues where game would
    //  occasionally lock up (like Valkyrie Profile), it should be stable now.
    //  Changed sync has a side effect however - if the emulator is not fast enough (may happen with double 
    //  resolution mode or while underclocking), sound will stutter more instead of slowing down the music itself.
    //  There is a new option in SPU plugin config to restore old inaccurate behavior if anyone wants it." -Notaz


    int ret = -1;

    ret = SPUinit();    //senquack - Should be called before SPU_open()
	if (ret < 0) { printf ("Error initializing SPU plugin spu_pcsxrearmed, SPUInit() returned: %d\n", ret); return -1; }

	ret = SPUopen();    //senquack - This initialized the low-level audio backend driver, i.e. OSS,ALSA,SDL,PulseAudio etc
	if (ret < 0) { printf ("Error opening audio backend for SPU spu_pcsxrearmed, SPUOpen() returned: %d\n", ret); return -1; }

    printf("-> SPU plugin spu_pcsxrearmed initialized successfully.\n");
    return 0;
}

static inline long SPU_shutdown()
{
    return SPUshutdown();       //senquack - calls SPUclose() automatically
}

static inline void SPU_writeRegister(unsigned long reg, unsigned short val)
{
    //senquack - PCSX_ReArmed version takes additional 3rd parameter, cycles:
    SPUwriteRegister(reg, val, *psxRegs_cycle_valptr);
}

static inline unsigned short SPU_readRegister(unsigned long reg)
{
    return SPUreadRegister(reg);
}

static inline void SPU_writeDMA(unsigned short val)
{
    SPUwriteDMA(val);
}

static inline unsigned short SPU_readDMA()
{
    return SPU_readDMA();
}

static inline void SPU_writeDMAMem(unsigned short *pusPSXMem, int iSize)
{
    //senquack - PCSX_ReArmed version takes additional 3rd parameter, cycles:
    SPUwriteDMAMem(pusPSXMem, iSize, *psxRegs_cycle_valptr);
}

static inline void SPU_readDMAMem(unsigned short *pusPSXMem, int iSize)
{
    //senquack - PCSX_ReArmed version takes additional 3rd parameter, cycles:
    SPUreadDMAMem(pusPSXMem, iSize, *psxRegs_cycle_valptr);
}

static inline void SPU_playADPCMchannel(xa_decode_t *xap)
{
    SPUplayADPCMchannel(xap);
}

//senquack - note how SPU_playCDDAchannel now returns a feedback value
//			 to cdriso.cpp:
static inline int SPU_playCDDAchannel(short *pcm, int bytes)
{
    return SPUplayCDDAchannel(pcm, bytes);
}

static inline long SPU_freeze(uint32_t ulFreezeMode, SPUFreeze_t * pF)
{
    return SPUfreeze(ulFreezeMode, pF, *psxRegs_cycle_valptr);
}

//senquack - my first attempt, forcing flag 2nd param to 1:
static inline void SPU_async(unsigned int cycle, unsigned int flags)
{
    //senquack - PCSX_ReARMed version takes two parameters, first one is psxRegs.cycle,
    //           second one is what amounts to a bool flag.
    //SPUasync(unsigned int cycle, unsigned int flags)

    //senquack force flags to 1 for now:
    SPUasync(*psxRegs_cycle_valptr, flags);
}

#endif //SPU_PCSXREARMED_WRAPPER_H
