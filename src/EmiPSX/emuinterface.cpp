/***********************************************************
 Copyright (C) 2013 AndreBotelho(andrebotelhomail@gmail.com)
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the
 Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.
 ***********************************************************/
 
#include "emuinterface.h"
#include "mainwindow.h"
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QDebug>

#include "port.h"
#include "r3000a.h"
#include "plugins.h"
#include "profiler.h"
#define SOUND_BUFFER_SIZE (64*1024)
unsigned short *SCREEN=NULL;
extern QString sRomFile;
extern QString sSavePath;
QByteArray *emuAudioBuffer;
QImage *emuFrameBuffer;
int emu_psx_bias = 2;
int emu_audio_xa = 1; /* 0=XA enabled, 1=XA disabled */
int emu_video_mdec = 1; /* 0=Black&White Mdecs Only Disabled, 1=Black&White Mdecs Only Enabled */
int emu_audio_cd = 1; /* 0=Enable Cd audio, 1=Disable Cd audio */
int emu_enable_bios = 1; /* 0=BIOS, 1=HLE */
int emu_cpu_type = 1;
int emu_skip_gpu = 0; /* skip GPU primitives */
int emu_frame_limit = 1; /* frames to wait */
int emu_video_light = 1; /* lighting */
int emu_video_blend = 1; /* blending */
extern int emu_sound_enable;
extern int emu_state;
extern int emu_framerate;
extern int emu_frameskip;
float emu_clock_multiplier = 0.33f;
int emu_screen_width, emu_screen_height;
extern void S9xLoadState();
extern void S9xSaveState();
EmuType currentEmu;
extern MainWindow *emuWindow;
static unsigned short pad1=0xffff;


int emuInit( EmuType type){
    currentEmu = type;
    emuFrameBuffer = new QImage(320,240,QImage::Format_RGB16);
    emuAudioBuffer = new QByteArray(SOUND_BUFFER_SIZE,0);
    if( emu_psx_bias ){
        autobias=0;
        BIAS = emu_psx_bias;
    }else{
        autobias=1;
    }
    PSXCLK=(u32)((double)PSXCLK*emu_clock_multiplier);
    Config.Xa=emu_audio_xa; /* 0=XA enabled, 1=XA disabled */
    Config.Mdec=emu_video_mdec; /* 0=Black&White Mdecs Only Disabled, 1=Black&White Mdecs Only Enabled */
    Config.Cdda=emu_audio_cd; /* 0=Enable Cd audio, 1=Disable Cd audio */
    Config.HLE=emu_enable_bios; /* 0=BIOS, 1=HLE */
    Config.Cpu=emu_cpu_type; /* 0=recompiler, 1=interpreter */
    Config.PsxAuto=1; /* 1=autodetect system (pal or ntsc) */
    Config.PsxType=0; /* PSX_TYPE_NTSC=ntsc, PSX_TYPE_PAL=pal */
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
    extern unsigned int autoFrameSkip; autoFrameSkip=emu_frameskip; /* auto frameskip */
    extern signed int framesToSkip; framesToSkip=emu_frameskip; /* frames to skip */
#endif
    // gpu_unai
#ifdef gpu_unai
    extern int skipCount; skipCount=emu_frameskip; /* frame skip (0,1,2,3...) */
    //extern bool enableAbbeyHack; enableAbbeyHack=false; /* Abe's Odyssey hack */
    extern int linesInterlace_user; linesInterlace_user=0; /* interlace */
    extern bool skipGPU; skipGPU = emu_skip_gpu; /* skip GPU primitives */
    extern bool frameLimit; frameLimit = emu_frame_limit; /* frames to wait */
    extern bool light; light = emu_video_light; /* lighting */
    extern bool blend; blend = emu_video_blend; /* blending */
    extern bool show_fps; show_fps = 0;//emu_framerate; /* blending */
#endif
#ifdef spu_franxis
    extern bool nullspu; nullspu = !emu_sound_enable; /* lighting */
#endif
    SCREEN = (unsigned short*)emuFrameBuffer->bits();
    InitAudio();
}

int emuChangeConfig(){
    if( emu_psx_bias ){
        autobias=0;
        BIAS = emu_psx_bias;
    }else{
        autobias=1;
    }
    PSXCLK=(u32)((double)PSXCLK*emu_clock_multiplier);
    Config.Xa=!emu_audio_xa; /* 0=XA enabled, 1=XA disabled */
    Config.Mdec=emu_video_mdec; /* 0=Black&White Mdecs Only Disabled, 1=Black&White Mdecs Only Enabled */
    Config.Cdda=!emu_audio_cd; /* 0=Enable Cd audio, 1=Disable Cd audio */
    Config.HLE=emu_enable_bios; /* 0=BIOS, 1=HLE */
    Config.Cpu=emu_cpu_type; /* 0=recompiler, 1=interpreter */
#ifdef gpu_unai
    extern int skipCount; skipCount=emu_frameskip; /* frame skip (0,1,2,3...) */
    extern bool skipGPU; skipGPU = emu_skip_gpu; /* skip GPU primitives */
    extern bool frameLimit; frameLimit = emu_frame_limit; /* frames to wait */
    extern bool light; light = emu_video_light; /* lighting */
    extern bool blend; blend = emu_video_blend; /* blending */
    extern bool show_fps; show_fps = 0;//emu_framerate; /* blending */
#endif
#ifdef gpu_new
    //  Tweaks and Hacks
    extern  int  skipCount;skipCount=emu_frameskip;
    extern  int  skipRate;
    extern  bool autoSkip;autoSkip = emu_frameskip;
    extern  bool enableFrameLimit;enableFrameLimit = emu_frame_limit;
    extern  bool enableAbbeyHack;
    extern  bool displayFrameInfo;
    extern  bool displayGpuStats;displayGpuStats = emu_framerate;
    extern  bool displayVideoMemory;
    extern  bool primitiveDebugMode;
    extern  bool activeNullGPU;
    extern  bool activeGPULog;
#endif

#ifdef spu_franxis
    extern bool nullspu; nullspu = !emu_sound_enable; /* sound */
#endif
    emuWindow->iSampleCounter = 0;
}

int InitAudio(){
    QAudioFormat format;
    format.setFrequency(22050);
    format.setChannels(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

    emuWindow->audioOut = new QAudioOutput(info, format, emuWindow);
    if(emuWindow->audioOut){
        emuWindow->setDevice();
        return 1;
    }else{
        return 0;
    }
}

void sound_init(void) {
}

void sound_close(void) {
}

void sound_set(unsigned char* pSound,long lBytes) {
    memcpy( emuAudioBuffer->data()+ emuWindow->iSampleCounter, pSound, lBytes);
    emuWindow->iSampleCounter += lBytes;
    emuWindow->refresh(1);
}

void video_flip(void)
{
    emuWindow->refresh(3);
}

void video_clear(void)
{
    emuFrameBuffer->fill(0);
}


unsigned long sound_get(void) {
    return 0; // return 0 to get more bytes
}

int emuLoadRom( QString * romFile ){
    if( romFile->isEmpty()) return 0;
    SetIsoFile(romFile->toLatin1().data());
    if (psxInit() == -1) {
        printf("PSX emulator couldn't be initialized.\n");
        emuClose();
        return 0;
    }
    if (LoadPlugins() == -1) {
        printf("Failed loading plugins.\n");
        emuClose();
        return 0;
    }
    psxReset();
    return 1;
}

int emuClose(){
    ReleasePlugins();
    psxShutdown();
}

void pad_update(void){
    emuWindow->refresh(1);
}

unsigned short pad_read(int num)
{
    emuWindow->refresh(1);
    if (num==0) return pad1; else return 0xffff;
}

int emuLoop(){
    psxCpu->Execute();
}

int emuReset(){
    psxReset();
}


void emuSaveState(){
    QFileInfo fi(sRomFile);
    QString path(sSavePath);
    path.append("/");
    path += fi.fileName();
    QString temp;
    path.append(temp.setNum(emu_state+1));
    path.append(".sav");
    int error =SaveState(path.toLatin1().data());
    if(error!=0){
        qDebug()<<"state save error: "<<error;
        emuReset();
    }
}


void emuLoadState(){
    QFileInfo fi(sRomFile);
    QString path(sSavePath);
    path.append("/");
    path += fi.fileName();
    QString temp;
    path.append(temp.setNum(emu_state+1));
    path.append(".sav");
    int error =LoadState(path.toLatin1().data());
    if(error!=0){
        qDebug()<<"state load error: "<< error;
        emuReset();
    }
}

int emuProcessKeys( int key, bool press){
    if( press ){
        switch( key ){
        case Qt::Key_Up:    pad1 &= ~(1 << DKEY_UP);    break;
        case Qt::Key_Down:  pad1 &= ~(1 << DKEY_DOWN);  break;
        case Qt::Key_Left:  pad1 &= ~(1 << DKEY_LEFT);  break;
        case Qt::Key_Right: pad1 &= ~(1 << DKEY_RIGHT); break;

        case Qt::Key_A: pad1 &= ~(1 << DKEY_SQUARE); break;
        case Qt::Key_X: pad1 &= ~(1 << DKEY_CIRCLE); break;
        case Qt::Key_Y: pad1 &= ~(1 << DKEY_TRIANGLE); break;
        case Qt::Key_B: pad1 &= ~(1 << DKEY_CROSS); break;

        case Qt::Key_L: pad1 &= ~(1 << DKEY_L1); break;
        case Qt::Key_R: pad1 &= ~(1 << DKEY_R1); break;

        case Qt::Key_Return: pad1 &= ~(1 << DKEY_START); break;
        case Qt::Key_Shift: pad1 &= ~(1 << DKEY_SELECT); break;
        default: break;
        }
    }else{
        switch( key ){
        case Qt::Key_Up:    pad1 |= (1 << DKEY_UP);    break;
        case Qt::Key_Down:  pad1 |= (1 << DKEY_DOWN);  break;
        case Qt::Key_Left:  pad1 |= (1 << DKEY_LEFT);  break;
        case Qt::Key_Right: pad1 |= (1 << DKEY_RIGHT); break;

        case Qt::Key_A: pad1 |= (1 << DKEY_SQUARE); break;
        case Qt::Key_X: pad1 |= (1 << DKEY_CIRCLE); break;
        case Qt::Key_Y: pad1 |= (1 << DKEY_TRIANGLE); break;
        case Qt::Key_B: pad1 |= (1 << DKEY_CROSS); break;

        case Qt::Key_L: pad1 |= (1 << DKEY_L1); break;
        case Qt::Key_R: pad1 |= (1 << DKEY_R1); break;

        case Qt::Key_Return: pad1 |= (1 << DKEY_START); break;
        case Qt::Key_Shift: pad1 |= (1 << DKEY_SELECT); break;
        default: break;
        }
    }
}

unsigned get_ticks(void)
{
    return emuWindow->timeCounter->elapsed();
}
int iWaitTime;
int iWaitSince;
void wait_ticks(unsigned s)
{
    emuWindow->sleep(s);
}

void port_printf(int x,int y,char *text)
{
    if(x == 0) qDebug() << text;
    emuWindow->setMessage( text, x, y);
}

void port_sync(void)
{
    qDebug() << "sync";
}

void port_mute(void)
{
    qDebug() << "mute";
}

void pcsx4all_exit(void)
{
    emuClose();
}
