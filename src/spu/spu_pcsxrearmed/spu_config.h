#ifndef SPU_CONFIG_H
#define SPU_CONFIG_H
// user settings

typedef struct
{
 int        iVolume;
 int        iXAPitch;
 int        iUseReverb;
 int        iUseInterpolation;
 int        iTempo;
 int        iUseThread;
 int        iUseFixedUpdates;  // output fixed number of samples/frame

 // status
 int        iThreadAvail;

 //senquack - added to disable audio (presumably from command line)
 int		iDisabled;

 //senquack - use older code in sdl.c that uses mutex to syncronize
 //           (for debugging / verification that newer atomic code works)
 int        iUseOldAudioMutex;

 //senquack - added to detect when configuration has been set
 int		iHaveConfiguration;
} SPUConfig;

extern SPUConfig spu_config;
#endif //SPU_CONFIG_H
