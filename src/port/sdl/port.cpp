
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

/* FPS showing */
extern char msg[36];
extern bool show_fps;

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

#ifdef GCW_ZERO
				case SDLK_LSHIFT: pad1 &= ~(1 << DKEY_SQUARE); break;
				case SDLK_LCTRL: pad1 &= ~(1 << DKEY_CIRCLE); break;
				case SDLK_SPACE: pad1 &= ~(1 << DKEY_TRIANGLE); break;
				case SDLK_LALT: pad1 &= ~(1 << DKEY_CROSS); break;

				case SDLK_TAB: pad1 &= ~(1 << DKEY_L1); break;
				case SDLK_BACKSPACE: pad1 &= ~(1 << DKEY_R1); break;
#else
				case SDLK_a: pad1 &= ~(1 << DKEY_SQUARE); break;
				case SDLK_x: pad1 &= ~(1 << DKEY_CIRCLE); break;
				case SDLK_y:
				case SDLK_s: pad1 &= ~(1 << DKEY_TRIANGLE); break;
				case SDLK_b:
				case SDLK_z: pad1 &= ~(1 << DKEY_CROSS); break;

				case SDLK_w: pad1 &= ~(1 << DKEY_L1); break;
				case SDLK_e: pad1 &= ~(1 << DKEY_R1); break;
#endif
				case SDLK_RETURN: pad1 &= ~(1 << DKEY_START); break;
#ifndef GCW_ZERO
				case SDLK_BACKSPACE: pad1 &= ~(1 << DKEY_SELECT); break;

				case 0:
#endif
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
				case SDLK_v: { show_fps=!show_fps; } break;
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
#ifdef GCW_ZERO
				case SDLK_LSHIFT: pad1 |= (1 << DKEY_SQUARE); break;
				case SDLK_LCTRL: pad1 |= (1 << DKEY_CIRCLE); break;
				case SDLK_SPACE: pad1 |= (1 << DKEY_TRIANGLE); break;
				case SDLK_LALT: pad1 |= (1 << DKEY_CROSS); break;

				case SDLK_TAB: pad1 |= (1 << DKEY_L1); break;
				case SDLK_BACKSPACE: pad1 |= (1 << DKEY_R1); break;
#else
				case SDLK_a: pad1 |= (1 << DKEY_SQUARE); break;
				case SDLK_x: pad1 |= (1 << DKEY_CIRCLE); break;
				case SDLK_s: pad1 |= (1 << DKEY_TRIANGLE); break;
				case SDLK_z: pad1 |= (1 << DKEY_CROSS); break;

				case SDLK_w: pad1 |= (1 << DKEY_L1); break;
				case SDLK_e: pad1 |= (1 << DKEY_R1); break;
#endif
				case SDLK_RETURN: pad1 |= (1 << DKEY_START); break;
#ifndef GCW_ZERO
				case SDLK_BACKSPACE: pad1 |= (1 << DKEY_SELECT); break;
#endif
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

//senquack - spu_pcsxrearmed has its own sound backends
#if !defined(spu_pcsxrearmed)

#ifdef spu_franxis
#define SOUND_BUFFER_SIZE (1024 * 3 * 2)
#else
#define SOUND_BUFFER_SIZE (1024 * 4 * 4)
#endif
static unsigned *sound_buffer = NULL;
static unsigned int buf_read_pos = 0;
static unsigned int buf_write_pos = 0;
static unsigned buffered_bytes = 0;
static int sound_initted = 0;
static int sound_running = 0;

SDL_mutex *sound_mutex;
SDL_cond *sound_cv;
static unsigned int mutex = 0; // 0 - don't use mutex; 1 - use it

static void sound_mix(void *unused, Uint8 *stream, int len)
{
	u8 *data = (u8 *)stream;
	u8 *buffer = (u8 *)sound_buffer;

	if (!sound_running)
		return;

	if (mutex)
		SDL_LockMutex(sound_mutex);

	if (buffered_bytes >= len) {
		if (buf_read_pos + len <= SOUND_BUFFER_SIZE ) {
			memcpy(data, buffer + buf_read_pos, len);
		} else {
			int tail = SOUND_BUFFER_SIZE - buf_read_pos;
			memcpy(data, buffer + buf_read_pos, tail);
			memcpy(data + tail, buffer, len - tail);
		}
		buf_read_pos = (buf_read_pos + len) % SOUND_BUFFER_SIZE;
		buffered_bytes -= len;
	}

	if (mutex) {
		SDL_CondSignal(sound_cv);
		SDL_UnlockMutex(sound_mutex);
	}
}

void sound_init(void)
{
#ifndef spu_null
	SDL_AudioSpec fmt;

	if (sound_initted)
		return;
	sound_initted = 1;

	fmt.format = AUDIO_S16;
#ifdef spu_franxis
	fmt.channels = 1;
	fmt.freq = 22050;
	fmt.samples = 1024;
#else
	fmt.channels = 2;
	fmt.freq = 44100;
	fmt.samples = 1024;
#endif
	fmt.callback = sound_mix;
	if (!sound_buffer)
		sound_buffer = (unsigned *)calloc(SOUND_BUFFER_SIZE,1);

	SDL_OpenAudio(&fmt, NULL);
#endif
	if (mutex) {
		sound_mutex = SDL_CreateMutex();
		sound_cv = SDL_CreateCond();
	}

	sound_running = 0;
	SDL_PauseAudio(0);
}

void sound_close(void) {
#ifndef spu_null
	sound_running = 0;

	if (mutex) {
		SDL_LockMutex(sound_mutex);
		buffered_bytes = SOUND_BUFFER_SIZE;
		SDL_CondSignal(sound_cv);
		SDL_UnlockMutex(sound_mutex);
		SDL_Delay(100);

		SDL_DestroyCond(sound_cv);
		SDL_DestroyMutex(sound_mutex);
	}

	if (!sound_initted)
		return;

	sound_initted = 0;
	SDL_CloseAudio();
	if (sound_buffer)
		free(sound_buffer);
	sound_buffer = NULL;
#endif
}

unsigned long sound_get(void) {
	#ifndef spu_null
	return 0; // return 0 to get more bytes
	#else
	return 24193; // no more bytes
	#endif
}

void sound_set(unsigned char *pSound, long lBytes)
{
	u8 *data = (u8 *)pSound;
	u8 *buffer = (u8 *)sound_buffer;

	sound_running = 1;

	if (mutex)
		SDL_LockMutex(sound_mutex);

	for (int i = 0; i < lBytes; i += 4) {
		if (mutex) {
			while (buffered_bytes == SOUND_BUFFER_SIZE)
				SDL_CondWait(sound_cv, sound_mutex);
		} else {
			if (buffered_bytes == SOUND_BUFFER_SIZE)
				return; // just drop samples
		}

		*(int*)((char*)(buffer + buf_write_pos)) = *(int*)((char*)(data + i));
		//memcpy(buffer + buf_write_pos, data + i, 4);
		buf_write_pos = (buf_write_pos + 4) % SOUND_BUFFER_SIZE;
		buffered_bytes += 4;
	}

	if (mutex) {
		SDL_CondSignal(sound_cv);
		SDL_UnlockMutex(sound_mutex);
	}
}

#endif //spu_pcsxrearmed


void video_flip(void)
{
	int i,j;
	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
	unsigned *src=(unsigned *)SCREEN,*dst=(unsigned *)screen->pixels;
#ifndef ANDROID
	const unsigned suma=(screen->pitch/2)-320;
//	const unsigned suma=(1024)-160;
#endif
	if (show_fps)
		port_printf(5,5,msg);
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
#if defined(ANDROID) || defined (PSXREC)
	Config.Cpu=0; /* 0=recompiler, 1=interpreter */
#else
	Config.Cpu=1; /* 0=recompiler, 1=interpreter */
#endif
	Config.RCntFix=0; /* 1=Parasite Eve 2, Vandal Hearts 1/2 Fix */
	Config.VSyncWA=0; /* 1=InuYasha Sengoku Battle Fix */

	// spu_dfxsound
	#ifdef spu_dfxsound
	{
	extern int iDisStereo; iDisStereo=0; // 0=stereo, 1=mono
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
		if (strcmp(argv[i],"-showfps")==0) { show_fps=true; } // show FPS
		if (strcmp(argv[i],"-framelimit")==0) { extern bool frameLimit; frameLimit=true; } // frame limit
		if (strcmp(argv[i],"-skip")==0) { extern int skipCount; skipCount=atoi(argv[i+1]); } // frame skip (0,1,2,3...)
		if (strcmp(argv[i],"-interlace")==0) { extern int linesInterlace_user; linesInterlace_user=1; } // interlace
		if (strcmp(argv[i],"-progressive")==0) { extern bool progressInterlace; progressInterlace=true; } // progressive interlace
	#endif

		// SPU
	#ifndef spu_null
		if (strcmp(argv[i],"-silent")==0) { extern bool nullspu; nullspu=true; } // No sound

	#ifndef spu_pcsxrearmed
		if (strcmp(argv[i],"-mutex")==0) { mutex = 1; } // use mutex
	#endif

        //senquack - Added audio syncronization option; if audio buffer full, main thread blocks
		if (strcmp(argv[i],"-syncaudio")==0) Config.SyncAudio=true; 
	#endif


		// WIZ
#if 0
		if (strcmp(argv[i],"-ramtweaks")==0) { extern int wiz_ram_tweaks; wiz_ram_tweaks=1; } // RAM tweaks
		if (strcmp(argv[i],"-clock")==0) { wiz_clock=atoi(argv[i+1]); } // WIZ clock
		if (strcmp(argv[i],"-frontend")==0) { strcpy(frontend,argv[i+1]); } // Frontend
#endif
	}
	

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);

#if !defined(spu_pcsxrearmed)		//spu_pcsxrearmed handles its own audio backends
	SDL_Init( SDL_INIT_AUDIO );
#endif

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
	static const unsigned char fontdata8x8[] =
	{
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x3C,0x42,0x99,0xBD,0xBD,0x99,0x42,0x3C,0x3C,0x42,0x81,0x81,0x81,0x81,0x42,0x3C,
		0xFE,0x82,0x8A,0xD2,0xA2,0x82,0xFE,0x00,0xFE,0x82,0x82,0x82,0x82,0x82,0xFE,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
		0x80,0xC0,0xF0,0xFC,0xF0,0xC0,0x80,0x00,0x01,0x03,0x0F,0x3F,0x0F,0x03,0x01,0x00,
		0x18,0x3C,0x7E,0x18,0x7E,0x3C,0x18,0x00,0xEE,0xEE,0xEE,0xCC,0x00,0xCC,0xCC,0x00,
		0x00,0x00,0x30,0x68,0x78,0x30,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
		0x3C,0x66,0x7A,0x7A,0x7E,0x7E,0x3C,0x00,0x0E,0x3E,0x3A,0x22,0x26,0x6E,0xE4,0x40,
		0x18,0x3C,0x7E,0x3C,0x3C,0x3C,0x3C,0x00,0x3C,0x3C,0x3C,0x3C,0x7E,0x3C,0x18,0x00,
		0x08,0x7C,0x7E,0x7E,0x7C,0x08,0x00,0x00,0x10,0x3E,0x7E,0x7E,0x3E,0x10,0x00,0x00,
		0x58,0x2A,0xDC,0xC8,0xDC,0x2A,0x58,0x00,0x24,0x66,0xFF,0xFF,0x66,0x24,0x00,0x00,
		0x00,0x10,0x10,0x38,0x38,0x7C,0xFE,0x00,0xFE,0x7C,0x38,0x38,0x10,0x10,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x1C,0x1C,0x18,0x00,0x18,0x18,0x00,
		0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x28,0x7C,0x28,0x7C,0x28,0x00,0x00,
		0x10,0x38,0x60,0x38,0x0C,0x78,0x10,0x00,0x40,0xA4,0x48,0x10,0x24,0x4A,0x04,0x00,
		0x18,0x34,0x18,0x3A,0x6C,0x66,0x3A,0x00,0x18,0x18,0x20,0x00,0x00,0x00,0x00,0x00,
		0x30,0x60,0x60,0x60,0x60,0x60,0x30,0x00,0x0C,0x06,0x06,0x06,0x06,0x06,0x0C,0x00,
		0x10,0x54,0x38,0x7C,0x38,0x54,0x10,0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,
		0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x04,0x08,0x10,0x20,0x40,0x00,0x00,
		0x38,0x4C,0xC6,0xC6,0xC6,0x64,0x38,0x00,0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00,
		0x7C,0xC6,0x0E,0x3C,0x78,0xE0,0xFE,0x00,0x7E,0x0C,0x18,0x3C,0x06,0xC6,0x7C,0x00,
		0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00,0xFC,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00,
		0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00,0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00,
		0x78,0xC4,0xE4,0x78,0x86,0x86,0x7C,0x00,0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00,
		0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30,
		0x1C,0x38,0x70,0xE0,0x70,0x38,0x1C,0x00,0x00,0x7C,0x00,0x00,0x7C,0x00,0x00,0x00,
		0x70,0x38,0x1C,0x0E,0x1C,0x38,0x70,0x00,0x7C,0xC6,0xC6,0x1C,0x18,0x00,0x18,0x00,
		0x3C,0x42,0x99,0xA1,0xA5,0x99,0x42,0x3C,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00,
		0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00,0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00,
		0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00,0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xFE,0x00,
		0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xC0,0x00,0x3E,0x60,0xC0,0xCE,0xC6,0x66,0x3E,0x00,
		0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,
		0x06,0x06,0x06,0x06,0xC6,0xC6,0x7C,0x00,0xC6,0xCC,0xD8,0xF0,0xF8,0xDC,0xCE,0x00,
		0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00,
		0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
		0xFC,0xC6,0xC6,0xC6,0xFC,0xC0,0xC0,0x00,0x7C,0xC6,0xC6,0xC6,0xDE,0xCC,0x7A,0x00,
		0xFC,0xC6,0xC6,0xCE,0xF8,0xDC,0xCE,0x00,0x78,0xCC,0xC0,0x7C,0x06,0xC6,0x7C,0x00,
		0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
		0xC6,0xC6,0xC6,0xEE,0x7C,0x38,0x10,0x00,0xC6,0xC6,0xD6,0xFE,0xFE,0xEE,0xC6,0x00,
		0xC6,0xEE,0x3C,0x38,0x7C,0xEE,0xC6,0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00,
		0xFE,0x0E,0x1C,0x38,0x70,0xE0,0xFE,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,
		0x60,0x60,0x30,0x18,0x0C,0x06,0x06,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,
		0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
		0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x06,0x3E,0x66,0x66,0x3C,0x00,
		0x60,0x7C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x3C,0x66,0x60,0x60,0x66,0x3C,0x00,
		0x06,0x3E,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x3C,0x00,
		0x1C,0x30,0x78,0x30,0x30,0x30,0x30,0x00,0x00,0x3E,0x66,0x66,0x66,0x3E,0x06,0x3C,
		0x60,0x7C,0x76,0x66,0x66,0x66,0x66,0x00,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x00,
		0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x38,0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00,
		0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0xEC,0xFE,0xFE,0xFE,0xD6,0xC6,0x00,
		0x00,0x7C,0x76,0x66,0x66,0x66,0x66,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x3C,0x00,
		0x00,0x7C,0x66,0x66,0x66,0x7C,0x60,0x60,0x00,0x3E,0x66,0x66,0x66,0x3E,0x06,0x06,
		0x00,0x7E,0x70,0x60,0x60,0x60,0x60,0x00,0x00,0x3C,0x60,0x3C,0x06,0x66,0x3C,0x00,
		0x30,0x78,0x30,0x30,0x30,0x30,0x1C,0x00,0x00,0x66,0x66,0x66,0x66,0x6E,0x3E,0x00,
		0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x7C,0x6C,0x00,
		0x00,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x06,0x3C,
		0x00,0x7E,0x0C,0x18,0x30,0x60,0x7E,0x00,0x0E,0x18,0x0C,0x38,0x0C,0x18,0x0E,0x00,
		0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,0x70,0x18,0x30,0x1C,0x30,0x18,0x70,0x00,
		0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00,0x10,0x28,0x10,0x54,0xAA,0x44,0x00,0x00,
	};
	unsigned short *screen=(SCREEN+x+y*320);
	for (int i=0;i<strlen(text);i++) {

		for (int l=0;l<8;l++) {
			screen[l*320+0]=(fontdata8x8[((text[i])*8)+l]&0x80)?0xffff:0x0000;
			screen[l*320+1]=(fontdata8x8[((text[i])*8)+l]&0x40)?0xffff:0x0000;
			screen[l*320+2]=(fontdata8x8[((text[i])*8)+l]&0x20)?0xffff:0x0000;
			screen[l*320+3]=(fontdata8x8[((text[i])*8)+l]&0x10)?0xffff:0x0000;
			screen[l*320+4]=(fontdata8x8[((text[i])*8)+l]&0x08)?0xffff:0x0000;
			screen[l*320+5]=(fontdata8x8[((text[i])*8)+l]&0x04)?0xffff:0x0000;
			screen[l*320+6]=(fontdata8x8[((text[i])*8)+l]&0x02)?0xffff:0x0000;
			screen[l*320+7]=(fontdata8x8[((text[i])*8)+l]&0x01)?0xffff:0x0000;
		}
		screen+=8;
	}
}

void port_sync(void)
{
	//sync();
}

void port_mute(void)
{
	//wiz_sound_thread_mute();
}
