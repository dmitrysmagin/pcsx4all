
#include "port.h"
#include "r3000a.h"
#include "plugins.h"
#include "profiler.h"
#include <QImage>
#include "mainwindow.h"


extern MainWindow *mW;

#ifdef ANDROID
#include "div.h"
#else
#define UDIV(n,d) ((n)/(d))
#define SDIV(n,d) ((n)/(d))
#endif

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

QImage	*screen=NULL;
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
	ReleasePlugins();
	psxShutdown();
	exit(0);
}

static unsigned short pad1=0xffff;
static unsigned short pad2=0xffff;

static int autosavestate=0;

void pad_update(void)
{
/*  SDL_Event event;

  while(SDL_PollEvent(&event))
  {
	  switch(event.type)
	  {

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
  }*/
}

unsigned short pad_read(int num)
{
    mW->refresh(1);
	if (num==0) return pad1; else return pad2;
}

#define SOUND_BUFFER_SIZE (64*1024)
static unsigned *sound_buffer=NULL;
static unsigned sound_wptr=0;
static unsigned sound_rptr=0;
static int sound_initted=0;
static int sound_running=0;


void sound_init(void) {
/*#ifndef spu_null
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
    SDL_PauseAudio(0);*/
}

void sound_close(void) {
#ifdef SOUND
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
    return 24193; // no more bytes
	#ifndef spu_null
	return 0; // return 0 to get more bytes
	#else
	return 24193; // no more bytes
	#endif
}

void sound_set(unsigned char* pSound,long lBytes) {
    mW->refresh(1);
    #ifdef SOUND
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
    //memcpy( screen->bits(), SCREEN, 320*240*2);
    mW->refresh(2);
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
    screen->fill(0);
}

static char savename[256];

int emu_init (char* isofile)
{
	char filename[256];
	const char *cdrfilename=GetIsoFile();

// CHUI: Y esto???? Peta en Linux al salir	
//	*stdout = *stderr;
	filename[0] = '\0'; /* Executable file name */
	savename[0] = '\0'; /* SaveState file name */
	
	// PCSX
	Config.Xa=1; /* 0=XA enabled, 1=XA disabled */
    //Config.Sio=0; /* 1=Sio Irq Always Enabled */
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
    //Config.SpuIrq=0; /* 1=Spu Irq Always Enabled */
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
    extern int UseFrameSkip; UseFrameSkip=0; // frame skip 1=on, 0=off
    extern int iFrameLimit; iFrameLimit=0; // fps limit 2=auto 1=fFrameRate, 0=off
	extern float fFrameRate; fFrameRate=200.0f; // fps
	extern int iUseDither; iUseDither=0; // 0=off, 1=game dependant, 2=always
	extern int iUseFixes; iUseFixes=0; // use game fixes
    extern uint32_t dwCfgFixes; dwCfgFixes=1; // game fixes
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
    //extern bool enableAbbeyHack; enableAbbeyHack=false; /* Abe's Odyssey hack */
	extern int linesInterlace_user; linesInterlace_user=0; /* interlace */
	#endif
	
    SetIsoFile(isofile);

    // command line options
    autobias=0;
    PSXCLK=(u32)((double)PSXCLK*0.5f);

    screen = new QImage(320, 240, QImage::Format_RGB16);
	if(screen == NULL) { puts("NO Set VideoMode 320x240x16"); exit(0); }
    SCREEN = (unsigned short*)screen->bits();//calloc(320*240,2);

	if (psxInit() == -1) {
		printf("PSX emulator couldn't be initialized.\n");
		pcsx4all_exit();
	}

	if (LoadPlugins() == -1) {
		printf("Failed loading plugins.\n");
		pcsx4all_exit();
	}

	psxReset();	

	
	return 0;
}

void emu_execute(){
    psxCpu->Execute();
}

unsigned get_ticks(void)
{
    return mW->timeCounter.elapsed();
}

void wait_ticks(unsigned s)
{
    //SDL_Delay(s/1000000);
}

void port_printf(int x,int y,char *text)
{
	printf(text);
	printf("\n");
}

void port_sync(void)
{
	//sync();
}

void port_mute(void)
{
	//wiz_sound_thread_mute();
}
