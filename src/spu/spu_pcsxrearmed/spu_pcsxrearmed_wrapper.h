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

#include "spu_config.h"	//To access SPU configuration settings
#include "psxcommon.h"  //To access emu configuration settings

//senquack - Some of the ReARMed SPU functions read the "cycles" value from
//           psxRegs.cycle, and I've provided a pointer to it in r3000a.cpp.
//           Not having to include "r3000a.h" avoids circuluar dependency hell.
extern const uint32_t* const psxRegs_cycle_valptr;

// SPU Functions
#ifdef __cplusplus
extern "C" {
#endif
extern long CALLBACK SPUinit(void);
extern long CALLBACK SPUopen(void);
extern long CALLBACK SPUshutdown(void);
extern long CALLBACK SPUclose(void);
extern void CALLBACK SPUwriteRegister(unsigned long, unsigned short, unsigned int);
extern unsigned short CALLBACK SPUreadRegister(unsigned long);
extern void CALLBACK SPUwriteDMA(unsigned short);
extern unsigned short CALLBACK SPUreadDMA(void);
extern void CALLBACK SPUwriteDMAMem(unsigned short *, int, unsigned int);
extern void CALLBACK SPUreadDMAMem(unsigned short *, int, unsigned int);
extern void CALLBACK SPUplayADPCMchannel(xa_decode_t *);
extern unsigned int CALLBACK SPUgetADPCMBufferRoom(); //senquack - added function
extern int  CALLBACK SPUplayCDDAchannel(short *, int);
extern void CALLBACK SPUregisterCallback(void (CALLBACK *callback)(void));
extern void CALLBACK SPUregisterScheduleCb(void (CALLBACK *callback)(unsigned int));
extern long CALLBACK SPUconfigure(void);
extern long CALLBACK SPUfreeze(uint32_t, SPUFreeze_t *, uint32_t);
extern void CALLBACK SPUasync(uint32_t, uint32_t);
#ifdef __cplusplus
}
#endif

// senquack - unused by PCSX4ALL:
//extern void SPUplaySample(unsigned char);   
//extern long SPUtest(void);
//extern void SPUabout(void);

static inline long SPU_init(void)
{
	//senquack - added new SPUConfig member to indicate when no configuration has been set:
	if (spu_config.iHaveConfiguration == 0) {
		printf("ERROR: SPU plugin 'spu_pcsxrearmed' configuration settings not set, aborting.\n");
		return -1;
	}

	int ret = -1;
	ret = SPUinit();    //senquack - Should be called before SPU_open()
	if (ret < 0) { printf ("ERROR initializing SPU plugin 'spu_pcsxrearmed', SPUInit() returned: %d\n", ret); return -1; }

	ret = SPUopen();    //senquack - This initialized the low-level audio backend driver, i.e. OSS,ALSA,SDL,PulseAudio etc
	if (ret < 0) { printf ("ERROR opening audio backend for SPU 'spu_pcsxrearmed', SPUOpen() returned: %d\n", ret); return -1; }

	printf("-> SPU plugin 'spu_pcsxrearmed' initialized successfully.\n");

	const char* interpol_str[4] = { "none", "simple", "gaussian", "cubic" };
	assert(spu_config.iUseInterpolation >= 0 && spu_config.iUseInterpolation <= 3);
	printf("-> SPU plugin using configuration settings:\n"
		   "    Volume:             %d\n"
		   "    Disabled (-silent): %d\n"
		   "    XAPitch:            %d\n"
		   "    UseReverb:          %d\n"
		   "    UseInterpolation:   %d (%s)\n"
		   "    Tempo:              %d\n"
		   "    UseThread:          %d\n"
		   "    UseFixedUpdates:    %d\n"
		   "    UseOldAudioMutex:   %d\n"
		   "    SyncAudio:          %d\n",
		   spu_config.iVolume, spu_config.iDisabled, spu_config.iXAPitch, spu_config.iUseReverb,
		   spu_config.iUseInterpolation, interpol_str[spu_config.iUseInterpolation],
		   spu_config.iTempo, spu_config.iUseThread, spu_config.iUseFixedUpdates,
		   spu_config.iUseOldAudioMutex, Config.SyncAudio);

	//senquack TODO: allow nullspu backend driver of spu_pcsxrearmed to provide simulated
	//               audio sync?
	if (Config.SyncAudio == 1 && spu_config.iDisabled == true)
		printf("-> WARNING: '-silent' option in effect; nullspu cannot sync emu to audio (yet).\n");

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

static inline void SPU_writeDMAMem(unsigned short *pusPSXMem, int iSize, unsigned int cycles)
{
    SPUwriteDMAMem(pusPSXMem, iSize, cycles);
}

static inline void SPU_readDMAMem(unsigned short *pusPSXMem, int iSize, unsigned int cycles)
{
    SPUreadDMAMem(pusPSXMem, iSize, cycles);
}

static inline void SPU_playADPCMchannel(xa_decode_t *xap)
{
    SPUplayADPCMchannel(xap);
}

// senquack - I added this function when fixing XA audio dropouts on
//            slower devices/games. (used in cdrom.cpp, see notes
//            for cdrReadInterrupt() there or UpdateXABufferRoom() and
//            FeedXA() in xa.c
static inline unsigned int SPU_getADPCMBufferRoom()
{
	return SPUgetADPCMBufferRoom();
}

//senquack - note how Notaz's SPU_playCDDAchannel() now returns a feedback value
//			 to caller thread in cdriso.cpp:
static inline int SPU_playCDDAchannel(short *pcm, int bytes)
{
    return SPUplayCDDAchannel(pcm, bytes);
}

//senquack - IRQ callback (implemented via AcknowledgeSPUIRQ() in PCSX4ALL plugins.cpp)
static inline void SPU_registerCallback(void (CALLBACK *callback)(void))
{
	SPUregisterCallback(callback);
}

//senquack - Schedule SPU update callback (implemented via ScheduleSPUUpdate()
// in PCSX4ALL plugins.cpp)
static inline void SPU_registerScheduleCb(void (CALLBACK *callback)(unsigned int))
{
	SPUregisterScheduleCb(callback);
}

static inline long SPU_freeze(uint32_t ulFreezeMode, SPUFreeze_t * pF)
{
    return SPUfreeze(ulFreezeMode, pF, *psxRegs_cycle_valptr);
}

//senquack - PCSX_ReARMed version takes two parameters, first one is psxRegs.cycle,
//           second one used as a simple bool flag.
static inline void SPU_async(unsigned int cycle, unsigned int flags)
{
    SPUasync(*psxRegs_cycle_valptr, flags);
}

#endif //SPU_PCSXREARMED_WRAPPER_H
