
#include "port.h"
#include "r3000a.h"
#include "plugins.h"
#include "profiler.h"
#include <SDL.h>

enum {
	DKEY_SELECT = 0,
	DKEY_L3,
	DKEY_R3,
	DKEY_START,
	DKEY_UP,
	DKEY_RIGHT,
	DKEY_DOWN,
	DKEY_LEFT,
	DKEY_L2,
	DKEY_R2,
	DKEY_L1,
	DKEY_R1,
	DKEY_TRIANGLE,
	DKEY_CIRCLE,
	DKEY_CROSS,
	DKEY_SQUARE,

	DKEY_TOTAL
};

SDL_Surface	*screen=NULL;
unsigned short *SCREEN=NULL;

#ifdef DEBUG_FAST_MEMORY
unsigned long long totalreads=0;
unsigned long long totalreads_ok=0;
unsigned long long totalreads_hw=0;
unsigned long long totalreads_mh=0;
unsigned long long totalwrites=0;
unsigned long long totalwrites_ok=0;
unsigned long long totalwrites_hw=0;
unsigned long long totalwrites_mh=0;
#endif
#ifdef DEBUG_IO_CYCLE_COUNTER
unsigned iocycle_saltado=0;
unsigned iocycle_ok=0;
#endif
void pcsx4all_exit(void)
{
	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
#ifdef DEBUG_FAST_MEMORY
	printf("READS TOTAL = %iK, OK=%iK (%.2f%%), HW=%iK (%.2f%%), MH=%iK (%.2f%%)\n",(unsigned)(totalreads/1000LL),(unsigned)(totalreads_ok/1000LL),(((double)totalreads_ok)*100.0)/((double)totalreads),(unsigned)(totalreads_hw/1000LL),(((double)totalreads_hw)*100.0)/((double)totalreads),(unsigned)(totalreads_mh/1000LL),(((double)totalreads_mh)*100.0)/((double)totalreads));
	printf("WRITES TOTAL = %iK, OK=%iK (%.2f%%), HW=%iK (%.2f%%), MH=%iK (%.2f%%)\n",(unsigned)(totalwrites/1000LL),(unsigned)(totalwrites_ok/1000LL),(((double)totalwrites_ok)*100.0)/((double)totalwrites),(unsigned)(totalwrites_hw/1000LL),(((double)totalwrites_hw)*100.0)/((double)totalwrites),(unsigned)(totalwrites_mh/1000LL),(((double)totalwrites_mh)*100.0)/((double)totalwrites));
#endif
#ifdef DEBUG_IO_CYCLE_COUNTER
	printf("IOCYCLE: %i/%i (%.2%%f)\n",iocycle_ok+iocycle_saltado,iocycle_ok, (((double)iocycle_ok)*100.0)/((double)(iocycle_ok+iocycle_saltado)));
#endif
	pcsx4all_prof_show();
	dbg_print_analysis();
	ReleasePlugins();
	psxShutdown();
	SDL_Quit();
	exit(0);
}

static unsigned short pad1=0xffff;
static unsigned short pad2=0xffff;

static int autosavestate=0;

void pad_update(void)
{
  SDL_Event event;

  while(SDL_PollEvent(&event))
  {
	  switch(event.type)
	  {
// CHUI: Salida por SDL_QUIT por comodidad...
		  case SDL_QUIT:
			if (autosavestate) {	
				toExit=1;
				toSaveState=1;
			}else
				pcsx4all_exit();
			break;
		  case SDL_KEYDOWN:
			switch(event.key.keysym.sym)
			{
				case SDLK_UP:    pad1 &= ~(1 << DKEY_UP);    break;
				case SDLK_DOWN:  pad1 &= ~(1 << DKEY_DOWN);  break;
				case SDLK_LEFT:  pad1 &= ~(1 << DKEY_LEFT);  break;
				case SDLK_RIGHT: pad1 &= ~(1 << DKEY_RIGHT); break;

				case SDLK_a: pad1 &= ~(1 << DKEY_SQUARE); break;
				case SDLK_x: pad1 &= ~(1 << DKEY_CIRCLE); break;
				case SDLK_y:
				case SDLK_s: pad1 &= ~(1 << DKEY_TRIANGLE); break;
				case SDLK_b:
				case SDLK_z: pad1 &= ~(1 << DKEY_CROSS); break;

				case SDLK_w: pad1 &= ~(1 << DKEY_L1); break;
				case SDLK_e: pad1 &= ~(1 << DKEY_R1); break;
				
				case SDLK_RETURN: pad1 &= ~(1 << DKEY_START); break;
				case SDLK_BACKSPACE: pad1 &= ~(1 << DKEY_SELECT); break;
				
				case 0:
				case SDLK_ESCAPE:
					if (autosavestate) {	
						toExit=1;
						toSaveState=1;
					}else
						pcsx4all_exit();
					break;
#ifdef DEBUG_FRAME
				case SDLK_F12: dbg_enable_frame(); break;
#endif
				case SDLK_F1: 
						   if ((!toLoadState)&&(!toSaveState))
							  toLoadState=1;
						   break;
				case SDLK_F2: 
						   if ((!toLoadState)&&(!toSaveState))
							  toSaveState=1;
						   break;

#ifdef gpu_unai
				case SDLK_v: { extern bool show_fps; show_fps=!show_fps; } break;
#endif
						   
				default: break;
			}
			break;
		case SDL_KEYUP:
			switch(event.key.keysym.sym)
			{
				case SDLK_UP:    pad1 |= (1 << DKEY_UP);    break;
				case SDLK_DOWN:  pad1 |= (1 << DKEY_DOWN);  break;
				case SDLK_LEFT:  pad1 |= (1 << DKEY_LEFT);  break;
				case SDLK_RIGHT: pad1 |= (1 << DKEY_RIGHT); break;

				case SDLK_a: pad1 |= (1 << DKEY_SQUARE); break;
				case SDLK_x: pad1 |= (1 << DKEY_CIRCLE); break;
				case SDLK_s: pad1 |= (1 << DKEY_TRIANGLE); break;
				case SDLK_z: pad1 |= (1 << DKEY_CROSS); break;

				case SDLK_w: pad1 |= (1 << DKEY_L1); break;
				case SDLK_e: pad1 |= (1 << DKEY_R1); break;

				case SDLK_RETURN: pad1 |= (1 << DKEY_START); break;
				case SDLK_BACKSPACE: pad1 |= (1 << DKEY_SELECT); break;

				default: break;
			}
			break;
		default: break;
	  }
  }
}

unsigned short pad_read(int num)
{
	if (num==0) return pad1; else return pad2;
}

#define SOUND_BUFFER_SIZE (64*1024)
static unsigned *sound_buffer=NULL;
static unsigned sound_wptr=0;
static unsigned sound_rptr=0;
static int sound_initted=0;
static SDL_sem * sound_data_available_sem=NULL;
static SDL_sem * sound_callback_done_sem=NULL;
static int sound_running=0;

static void sound_mix(void *unused, Uint8 *stream, int len) {
	if (!sound_running) return;
	unsigned *dest=(unsigned *)stream;
	if (sound_wptr!=sound_rptr)
		SDL_SemWait(sound_data_available_sem);
	for(len/=4;(sound_wptr!=sound_rptr)&&len;len--)
		*dest++=sound_buffer[(sound_wptr++)&(SOUND_BUFFER_SIZE-1)];
	sound_wptr&=(SOUND_BUFFER_SIZE-1);
	SDL_SemPost(sound_callback_done_sem);
}

void sound_init(void) {
#ifndef spu_null
	if (sound_initted) return;
	sound_initted=1;
	SDL_AudioSpec fmt;
	fmt.format = AUDIO_S16;
	fmt.channels = 1;
#ifdef spu_franxis
	fmt.freq = 22050;
	fmt.samples = 1080/2;
#else
	fmt.freq = 44100;
	fmt.samples = 1620/2;
#endif
	fmt.callback = sound_mix;
	if (!sound_buffer)
		sound_buffer = (unsigned *)calloc(SOUND_BUFFER_SIZE,4);
	SDL_OpenAudio(&fmt, NULL);
#endif
	if (!sound_callback_done_sem)
		sound_callback_done_sem=SDL_CreateSemaphore(0);
	if (!sound_data_available_sem)
		sound_data_available_sem=SDL_CreateSemaphore(0);
	sound_running=0;
	SDL_PauseAudio(0);
}

void sound_close(void) {
#ifndef spu_null
	sound_running=0;
	if (sound_data_available_sem)
		SDL_SemPost(sound_data_available_sem);
	SDL_Delay(333);
	if (!sound_initted) return;
	sound_initted=0;
	SDL_CloseAudio();
	if (sound_buffer)
		free(sound_buffer);
	sound_buffer=NULL;
	if (sound_callback_done_sem)
		SDL_DestroySemaphore(sound_callback_done_sem); 
	sound_callback_done_sem=NULL;
	if (sound_data_available_sem)
		SDL_DestroySemaphore(sound_data_available_sem); 
	sound_data_available_sem=NULL;
#endif
}

unsigned long sound_get(void) {
	#ifndef spu_null
	return 0; // return 0 to get more bytes
	#else
	return 24193; // no more bytes
	#endif
}

void sound_set(unsigned char* pSound,long lBytes) {
	#ifndef spu_null
	unsigned *orig=(unsigned *)pSound;
	sound_running=1;
	SDL_SemWait(sound_callback_done_sem);
//	SDL_LockAudio();
	for(lBytes/=4;lBytes;lBytes--)
		sound_buffer[(sound_rptr++)&(SOUND_BUFFER_SIZE-1)]=*orig++;
	sound_rptr&=(SOUND_BUFFER_SIZE-1);
//	SDL_UnlockAudio();
	SDL_SemPost(sound_data_available_sem);
	#endif
}


void video_flip(void)
{
	int i,j;
	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
	unsigned *src=(unsigned *)SCREEN,*dst=(unsigned *)screen->pixels;
#ifndef ANDROID
	const unsigned suma=(screen->pitch/2)-320;
//	const unsigned suma=(1024)-160;
#endif
	for(j=0;j<240;j++) {
		for(i=0;i<(320/(2*8));i++) {
			*dst++=*src++;
			*dst++=*src++;
			*dst++=*src++;
			*dst++=*src++;
			*dst++=*src++;
			*dst++=*src++;
			*dst++=*src++;
			*dst++=*src++;
		}
#ifndef ANDROID
		dst+=suma;
#endif
	}
	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}

void video_set(unsigned short* pVideo,unsigned int width,unsigned int height)
{
	int y;
	unsigned short *ptr=SCREEN;
	int w=(width>320?320:width);
	int h=(height>240?240:height);

	for (y=0;y<h;y++)
	{
		memcpy (ptr,pVideo,w*2);
		ptr+=320;
		pVideo+=width;
	}
	
	video_flip();
}

void video_clear(void)
{
	memset(screen->pixels, 0, screen->pitch*screen->h);
}

static char savename[256];

int main (int argc, char **argv)
{
	char filename[256];
	const char *cdrfilename=GetIsoFile();

// CHUI: Y esto???? Peta en Linux al salir	
//	*stdout = *stderr;
	filename[0] = '\0'; /* Executable file name */
	savename[0] = '\0'; /* SaveState file name */
	
	// PCSX
	Config.Xa=1; /* 0=XA enabled, 1=XA disabled */
	Config.Mdec=0; /* 0=Black&White Mdecs Only Disabled, 1=Black&White Mdecs Only Enabled */
	Config.PsxAuto=1; /* 1=autodetect system (pal or ntsc) */
	Config.PsxType=0; /* PSX_TYPE_NTSC=ntsc, PSX_TYPE_PAL=pal */
	Config.Cdda=1; /* 0=Enable Cd audio, 1=Disable Cd audio */
	Config.HLE=1; /* 0=BIOS, 1=HLE */
#ifdef ANDROID
	Config.Cpu=0; /* 0=recompiler, 1=interpreter */
#else
	Config.Cpu=1; /* 0=recompiler, 1=interpreter */
#endif
	Config.RCntFix=0; /* 1=Parasite Eve 2, Vandal Hearts 1/2 Fix */
	Config.VSyncWA=0; /* 1=InuYasha Sengoku Battle Fix */

	// spu_dfxsound
	#ifdef spu_dfxsound
	{
	extern int iDisStereo; iDisStereo=1; // 0=stereo, 1=mono
	extern int iUseInterpolation; iUseInterpolation=0; // 0=disabled, 1=enabled
	extern int iUseReverb; iUseReverb=0; // 0=disabled, 1=enabled
	}
	#endif
	
	// gpu_dfxvideo
	#ifdef gpu_dfxvideo
	extern int UseFrameLimit; UseFrameLimit=1; // limit fps 1=on, 0=off
	extern int UseFrameSkip; UseFrameSkip=1; // frame skip 1=on, 0=off
	extern int iFrameLimit; iFrameLimit=2; // fps limit 2=auto 1=fFrameRate, 0=off
	extern float fFrameRate; fFrameRate=200.0f; // fps
	extern int iUseDither; iUseDither=0; // 0=off, 1=game dependant, 2=always
	extern int iUseFixes; iUseFixes=0; // use game fixes
	extern uint32_t dwCfgFixes; dwCfgFixes=0; // game fixes
	/*
	 1=odd/even hack (Chrono Cross)
	 2=expand screen width (Capcom fighting games)
	 4=ignore brightness color (black screens in Lunar)
	 8=disable coordinate check (compatibility mode)
	 16=disable cpu saving (for precise framerate)
	 32=PC fps calculation (better fps limit in some games)
	 64=lazy screen update (Pandemonium 2)
	 128=old frame skipping (skip every second frame)
	 256=repeated flat tex triangles (Dark Forces)
	 512=draw quads with triangles (better g-colors, worse textures)
	*/
	#endif

	// gpu_drhell
	#ifdef gpu_drhell
	extern unsigned int autoFrameSkip; autoFrameSkip=1; /* auto frameskip */
	extern signed int framesToSkip; framesToSkip=0; /* frames to skip */
	#endif
	
	// gpu_unai
	#ifdef gpu_unai
	extern int skipCount; skipCount=0; /* frame skip (0,1,2,3...) */
	extern int linesInterlace_user; linesInterlace_user=0; /* interlace */
	#endif
	
	SetIsoFile("/tmp/wipe.bin");
	//SetIsoFile("../isos/bubble.bin");
	//SetIsoFile("../isos/bustmove.bin");
	//SetIsoFile("../isos/castle.bin");
	//SetIsoFile("../isos/cotton.iso");
	//SetIsoFile("../isos/ridge.bin");
	//SetIsoFile("../isos/TEKKEN3.BIN");
	//SetIsoFile("../isos/yoyo.bin");

	// command line options
	for (int i=1;i<argc;i++)
	{
		// PCSX
		if (strcmp(argv[i],"-xa")==0) Config.Xa=0; // XA enabled
		if (strcmp(argv[i],"-bwmdec")==0) Config.Mdec=1; // Black & White MDEC
		if (strcmp(argv[i],"-pal")==0) { Config.PsxAuto=0; Config.PsxType=1; } // Force PAL system
		if (strcmp(argv[i],"-ntsc")==0) { Config.PsxAuto=0; Config.PsxType=0; } // Force NTSC system
		if (strcmp(argv[i],"-cdda")==0) Config.Cdda=0; // CD audio enabled
		if (strcmp(argv[i],"-bios")==0) Config.HLE=0; // BIOS enabled
		if (strcmp(argv[i],"-interpreter")==0) Config.Cpu=1; // Interpreter enabled
		if (strcmp(argv[i],"-rcntfix")==0) Config.RCntFix=1; // Parasite Eve 2, Vandal Hearts 1/2 Fix
		if (strcmp(argv[i],"-vsyncwa")==0) Config.VSyncWA=1; // InuYasha Sengoku Battle Fix
		if (strcmp(argv[i],"-iso")==0) SetIsoFile(argv[i+1]); // Set ISO file
		if (strcmp(argv[i],"-file")==0) strcpy(filename,argv[i+1]); // Set executable file
		if (strcmp(argv[i],"-savestate")==0) strcpy(savename,argv[i+1]); // Set executable file
		if (strcmp(argv[i],"-autosavestate")==0) autosavestate=1; // Autosavestate
		if (strcmp(argv[i],"-bias")==0) {
			BIAS=atoi(argv[i+1]); // Set BIAS
			if (((int)BIAS)<1) {
				autobias=1;
				BIAS=2;
			} else {
				autobias=0;
			}
		}
		if (strcmp(argv[i],"-adjust")==0) { PSXCLK=(u32)((double)PSXCLK*atof(argv[i+1])); }
		// GPU
	#ifdef gpu_unai
		if (strcmp(argv[i],"-framelimit")==0) { extern bool frameLimit; frameLimit=true; } // frame limit
		if (strcmp(argv[i],"-skip")==0) { extern int skipCount; skipCount=atoi(argv[i+1]); } // frame skip (0,1,2,3...)
		if (strcmp(argv[i],"-interlace")==0) { extern int linesInterlace_user; linesInterlace_user=1; } // interlace
		if (strcmp(argv[i],"-progressive")==0) { extern bool progressInterlace; progressInterlace=true; } // progressive interlace
	#endif
		// SPU
	#ifndef spu_null
		if (strcmp(argv[i],"-silent")==0) { extern bool nullspu; nullspu=true; } // No sound
	#endif
		// WIZ
#if 0
		if (strcmp(argv[i],"-ramtweaks")==0) { extern int wiz_ram_tweaks; wiz_ram_tweaks=1; } // RAM tweaks
		if (strcmp(argv[i],"-clock")==0) { wiz_clock=atoi(argv[i+1]); } // WIZ clock
		if (strcmp(argv[i],"-frontend")==0) { strcpy(frontend,argv[i+1]); } // Frontend
#endif
	}
	

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
	atexit(SDL_Quit);
	screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE);
	if(screen == NULL) { puts("NO Set VideoMode 320x240x16"); exit(0); }
	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
	SDL_WM_SetCaption("pcsx4all - SDL Version", "pcsx4all");
	SCREEN=(unsigned short*)calloc(320*240,2);

	if (psxInit() == -1) {
		printf("PSX emulator couldn't be initialized.\n");
		pcsx4all_exit();
	}

	if (LoadPlugins() == -1) {
		printf("Failed loading plugins.\n");
		pcsx4all_exit();
	}

	psxReset();	

	if (cdrfilename[0]!='\0') { if (CheckCdrom() == -1) { printf("Failed checking ISO image.\n"); SetIsoFile(NULL); }
	else { if (LoadCdrom() == -1) { printf("Failed loading ISO image.\n"); SetIsoFile(NULL); } } }
	if (filename[0]!='\0') { if (Load(filename) == -1) { printf("Failed loading executable.\n"); filename[0]='\0'; } }
//	if (cdrfilename[0]!='\0') { printf("Running ISO image: %s.\n",cdrfilename); }
	if (filename[0]!='\0') { printf("Running executable: %s.\n",filename); }
	if ((cdrfilename[0]=='\0') && (filename[0]=='\0') && (Config.HLE==0)) { printf("Running BIOS.\n"); }

	if ((cdrfilename[0]!='\0') || (filename[0]!='\0') || (Config.HLE==0))
	{
		if (savename[0]) SaveState_filename=(char*)&savename;
#ifdef DEBUG_PCSX4ALL
		if (savename[0]) LoadState(savename); // Load save-state
#else
		if ((autosavestate) && (savename[0])) LoadState(savename); // Load save-state
#endif
		pcsx4all_prof_start(PCSX4ALL_PROF_CPU);
		psxCpu->Execute();
		pcsx4all_prof_end(PCSX4ALL_PROF_CPU);
	}

	pcsx4all_exit(); // exit
	
	return 0;
}

unsigned get_ticks(void)
{
#ifdef TIME_IN_MSEC
	return SDL_GetTicks();
#else
	return ((((unsigned long long)clock())*1000000ULL)/((unsigned long long)CLOCKS_PER_SEC));
#endif
}

void wait_ticks(unsigned s)
{
#ifdef TIME_IN_MSEC
	SDL_Delay(s);
#else
	SDL_Delay(s/1000);
#endif
}

void port_printf(int x,int y,char *text)
{
	puts(text);
}

void port_sync(void)
{
	//sync();
}

void port_mute(void)
{
	//wiz_sound_thread_mute();
}
