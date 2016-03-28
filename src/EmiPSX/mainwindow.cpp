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
 
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "emuinterface.h"
#include "symbian_memory_handler.h"
#include "relocutils.h"
#include <QEvent>
#include <QMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QMessageBox>
#include <QIODevice>
#include <QInputDialog>
#include <QDeclarativeView>
#include <QDebug>
#ifdef Q_OS_SYMBIAN
#include <SoundDevice.h>
#endif

extern QByteArray *emuAudioBuffer;
extern QImage *emuFrameBuffer;

QString sRomFile;
QString sSavePath;
void getChanges();
void update_audio(int len);
extern QDeclarativeView *menuView;
extern QDeclarativeView *menuViewp;
int emu_frameskip = 0;
int emu_framerate = 0;
int emu_sound_enable = 1;
int emu_sound_rate = 0;
int emu_volume = 50;
int emu_state = 0;
extern int emu_psx_bias;
extern int emu_audio_xa; /* 0=XA enabled, 1=XA disabled */
extern int emu_video_mdec; /* 0=Black&White Mdecs Only Disabled, 1=Black&White Mdecs Only Enabled */
extern int emu_audio_cd; /* 0=Enable Cd audio, 1=Disable Cd audio */
extern int emu_enable_bios; /* 0=BIOS, 1=HLE */
extern int emu_cpu_type;
extern int emu_skip_gpu; /* skip GPU primitives */
extern int emu_frame_limit; /* frames to wait */
extern int emu_video_light; /* lighting */
extern int emu_video_blend; /* blending */
int emu_clock;
extern float emu_clock_multiplier;
extern int emu_screen_width, emu_screen_height;
int snesKeys[12];

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tframeRate = new QString;
    tText = new QString;
    timeCounter = new QTime();
    loopTimer = new QTimer();
    secondTimer = new QTimer();
    loopTimer->start(0);
    secondTimer->start(1000);
    timeCounter->start();
    ishowText = 0;
    iCounter = 0;
    icurrRate = 0;
    iRomLoaded = 0;
    iGameState = 0;
    iSkipBlit = 0;
    iZoom = 0;
    audioOut = NULL;
    bEmulating = false;
    iVCTransparency = 5;
    iBuffer_Size = 512;
    bLandscape = true;
    audioBuffer = new QByteArray(4096*16,0);
    connect(loopTimer, SIGNAL(timeout()), this,  SLOT(runEmuLoop()));
    setAttribute(Qt::WA_AcceptTouchEvents);
    audioDevice = NULL;
    emuInited = false;
    bSmoothImage = false;
    sRomDir = "e:/data";
    sSaveDir = sRomDir;
    sSavePath = sSaveDir;
    iRefresh = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
    exitEmu();
}

int MainWindow::init()
{
    QGraphicsObject *vobject = menuView->rootObject();
    connect(vobject ,SIGNAL(getRomFile()), this,  SLOT(getRomFile()));
    connect(vobject ,SIGNAL(exitEmu()), this,  SLOT(exitEmu()));
    connect(vobject ,SIGNAL(resetEmu()), this,  SLOT(reset()));
    connect(vobject ,SIGNAL(saveState()), this,  SLOT(saveState()));
    connect(vobject ,SIGNAL(loadState()), this,  SLOT(loadState()));
    connect(vobject ,SIGNAL(emuContinue()), this,  SLOT(emuContinue()));
    connect(vobject ,SIGNAL(saveConfig()), this,  SLOT(saveConfig()));
    connect(vobject,SIGNAL(stChanged(int)), this,  SLOT(setGameState(int)));
    connect(vobject,SIGNAL(zoomChanged(int)), this,  SLOT(setZoom(int)));
    connect(vobject,SIGNAL(saveVControl()), this,  SLOT(saveVirtualKeys()));
    connect(vobject,SIGNAL(getRomDir()), this,  SLOT(getRomDir()));
    connect(vobject,SIGNAL(getSaveDir()), this,  SLOT(getSaveDir()));
    connect(vobject,SIGNAL(setKey(int,int)), this,  SLOT(setKeys(int,int)));
    connect(this,SIGNAL(UpdateImgs()), vobject,  SLOT(updateimgs()));
    connect(this,SIGNAL(UpdateRomDir()), vobject,  SLOT(upRomDir()));
    connect(this,SIGNAL(UpdateSaveDir()), vobject,  SLOT(upSaveDir()));
    vobject = menuViewp->rootObject();
    connect(vobject ,SIGNAL(getRomFile()), this,  SLOT(getRomFile()));
    connect(vobject ,SIGNAL(exitEmu()), this,  SLOT(exitEmu()));
    connect(vobject ,SIGNAL(resetEmu()), this,  SLOT(reset()));
    connect(vobject ,SIGNAL(saveState()), this,  SLOT(saveState()));
    connect(vobject ,SIGNAL(loadState()), this,  SLOT(loadState()));
    connect(vobject ,SIGNAL(emuContinue()), this,  SLOT(emuContinue()));
    connect(vobject ,SIGNAL(saveConfig()), this,  SLOT(saveConfig()));
    connect(vobject,SIGNAL(stChanged(int)), this,  SLOT(setGameState(int)));
    connect(vobject,SIGNAL(zoomChanged(int)), this,  SLOT(setZoom(int)));
    connect(vobject,SIGNAL(saveVControl()), this,  SLOT(saveVirtualKeys()));
    connect(vobject,SIGNAL(getRomDir()), this,  SLOT(getRomDir()));
    connect(vobject,SIGNAL(getSaveDir()), this,  SLOT(getSaveDir()));
    connect(vobject,SIGNAL(setKey(int,int)), this,  SLOT(setKeys(int,int)));
    connect(this,SIGNAL(UpdateImgs()), vobject,  SLOT(updateimgs()));
    connect(this,SIGNAL(UpdateRomDir()), vobject,  SLOT(upRomDir()));
    connect(this,SIGNAL(UpdateSaveDir()), vobject,  SLOT(upSaveDir()));
    if(menuView) menuView->hide();
    if(menuViewp) menuViewp->hide();
    loadVirtualKeys();
//    if(create_all_translation_caches()){
//        qDebug() << "Translation cache creation failed";
//    }
//    SymbianPackHeap();
//    modify_function_pointers_symbian( relocated_ro_offset );
    return 0;
}

void MainWindow::exitEmu()
{
//    if( iRomLoaded ){
//        emu_state = 6;
//        emuSaveState();
//    }
    saveConfig();
    if( emuInited )emuClose();
    QApplication::quit();
}
void MainWindow::getRomDir(){
    QString temp = QFileDialog::getExistingDirectory(this, tr("Select Rom Directory"),
                                                     "e:/",
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

    if(temp.isEmpty())
        return;
    else
        sRomDir = temp;
    QObject *tmp = menuView->rootObject();
    if (tmp){
        tmp->setProperty("romdir", sRomDir);
    }
    tmp = menuViewp->rootObject();
    if (tmp){
        tmp->setProperty("romdir", sRomDir);
    }
    emit UpdateRomDir();
}

void MainWindow::getSaveDir(){
    QString temp = QFileDialog::getExistingDirectory(this, tr("Select Save Directory"),
                                                     "e:/",
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);


    if(temp.isEmpty())
        return;
    else
        sSaveDir = temp;
    sSavePath = sSaveDir;
    QObject *tmp = menuView->rootObject();
    if (tmp){
        tmp->setProperty("savedir", sSaveDir);
    }
    tmp = menuViewp->rootObject();
    if (tmp){
        tmp->setProperty("savedir", sSaveDir);
    }
    emit UpdateSaveDir();
}

void MainWindow::loadVirtualKeys()
{
    QSettings config("./config.ini", QSettings::IniFormat);
    QString path;
    int x,y,w,h,xp,yp;
    path = config.value("buttons/Up/img",":/app/data/setaUP.png").toString();
    x = config.value( "buttons/Up/x",5).toInt();
    y = config.value( "buttons/Up/y",100).toInt();
    w = config.value( "buttons/Up/w",180).toInt();
    h = config.value( "buttons/Up/h",60).toInt();
    xp = config.value( "buttons/Up/xp",10).toInt();
    yp = config.value( "buttons/Up/yp",460).toInt();
    iKeys[Up]= new VirtualKey(QRect(x,y,w,h), Qt::Key_Up, QRect(xp,yp,w,h));
    iKeys[Up]->img = QImage(path).scaled(iKeys[Up]->lRect.width()/3, iKeys[Up]->lRect.height());
    iKeys[Up]->file = path;
    path = config.value("buttons/Left/img",":/app/data/setaLEFT.png").toString();
    x = config.value( "buttons/Left/x",5).toInt();
    y = config.value( "buttons/Left/y",100).toInt();
    w = config.value( "buttons/Left/w",60).toInt();
    h = config.value( "buttons/Left/h",180).toInt();
    xp = config.value( "buttons/Left/xp",10).toInt();
    yp = config.value( "buttons/Left/yp",460).toInt();
    iKeys[Left]= new VirtualKey(QRect(x,y,w,h), Qt::Key_Left, QRect(xp,yp,w,h));
    iKeys[Left]->img = QImage(path).scaled(iKeys[Left]->lRect.width(), iKeys[Left]->lRect.height()/3);
    iKeys[Left]->file = path;

    path = config.value("buttons/Right/img",":/app/data/setaRIGHT.png").toString();
    x = config.value( "buttons/Right/x",125).toInt();
    y = config.value( "buttons/Right/y",100).toInt();
    w = config.value( "buttons/Right/w",60).toInt();
    h = config.value( "buttons/Right/h",180).toInt();
    xp = config.value( "buttons/Right/xp",130).toInt();
    yp = config.value( "buttons/Right/yp",460).toInt();
    iKeys[Right]= new VirtualKey(QRect(x,y,w,h), Qt::Key_Right, QRect(xp,yp,w,h));
    iKeys[Right]->img = QImage(path).scaled(iKeys[Right]->lRect.width(), iKeys[Right]->lRect.height()/3);
    iKeys[Right]->file = path;
    path = config.value("buttons/Down/img",":/app/data/setaDOWN.png").toString();
    x = config.value( "buttons/Down/x",5).toInt();
    y = config.value( "buttons/Down/y",220).toInt();
    w = config.value( "buttons/Down/w",180).toInt();
    h = config.value( "buttons/Down/h",60).toInt();
    xp = config.value( "buttons/Down/xp",10).toInt();
    yp = config.value( "buttons/Down/yp",580).toInt();
    iKeys[Down]= new VirtualKey(QRect(x,y,w,h), Qt::Key_Down, QRect(xp,yp,w,h));
    iKeys[Down]->img = QImage(path).scaled(iKeys[Down]->lRect.width()/3, iKeys[Down]->lRect.height());
    iKeys[Down]->file = path;
    path = config.value("buttons/C/img",":/app/data/buttonC.png").toString();
    x = config.value( "buttons/C/x",570).toInt();
    y = config.value( "buttons/C/y",240).toInt();
    w = config.value( "buttons/C/w",60).toInt();
    h = config.value( "buttons/C/h",60).toInt();
    xp = config.value( "buttons/C/xp",290).toInt();
    yp = config.value( "buttons/C/yp",500).toInt();
    iKeys[A]= new VirtualKey(QRect(x,y,w,h), Qt::Key_A, QRect(xp,yp,w,h));
    iKeys[A]->img = QImage(path).scaled(iKeys[Down]->lRect.width()/3, iKeys[Down]->lRect.height());
    iKeys[A]->file = path;
    path = config.value("buttons/X/img",":/app/data/buttonX.png").toString();
    x = config.value( "buttons/X/x",510).toInt();
    y = config.value( "buttons/X/y",290).toInt();
    w = config.value( "buttons/X/w",60).toInt();
    h = config.value( "buttons/X/h",60).toInt();
    xp = config.value( "buttons/X/xp",270).toInt();
    yp = config.value( "buttons/X/yp",580).toInt();
    iKeys[B]= new VirtualKey(QRect(x,y,w,h), Qt::Key_B, QRect(xp,yp,w,h));
    iKeys[B]->img = QImage(path).scaled(iKeys[Down]->lRect.width()/3, iKeys[Down]->lRect.height());
    iKeys[B]->file = path;
    path = config.value("buttons/T/img",":/app/data/buttonT.png").toString();
    x = config.value( "buttons/T/x",510).toInt();
    y = config.value( "buttons/T/y",190).toInt();
    w = config.value( "buttons/T/w",60).toInt();
    h = config.value( "buttons/T/h",60).toInt();
    xp = config.value( "buttons/T/xp",240).toInt();
    yp = config.value( "buttons/T/yp",450).toInt();
    iKeys[X]= new VirtualKey(QRect(x,y,w,h), Qt::Key_X, QRect(xp,yp,w,h));
    iKeys[X]->img = QImage(path).scaled(iKeys[Down]->lRect.width()/3, iKeys[Down]->lRect.height());
    iKeys[X]->file = path;
    path = config.value("buttons/S/img",":/app/data/buttonS.png").toString();
    x = config.value( "buttons/S/x",450).toInt();
    y = config.value( "buttons/S/y",240).toInt();
    w = config.value( "buttons/S/w",60).toInt();
    h = config.value( "buttons/S/h",60).toInt();
    xp = config.value( "buttons/S/xp",220).toInt();
    yp = config.value( "buttons/S/yp",530).toInt();
    iKeys[Y]= new VirtualKey(QRect(x,y,w,h), Qt::Key_Y, QRect(xp,yp,w,h));
    iKeys[Y]->img = QImage(path).scaled(iKeys[Down]->lRect.width()/3, iKeys[Down]->lRect.height());
    iKeys[Y]->file = path;
    path = config.value("buttons/Start/img",":/app/data/stbutton.png").toString();
    x = config.value( "buttons/Start/x",320).toInt();
    y = config.value( "buttons/Start/y",10).toInt();
    w = config.value( "buttons/Start/w",70).toInt();
    h = config.value( "buttons/Start/h",30).toInt();
    xp = config.value( "buttons/Start/xp",120).toInt();
    yp = config.value( "buttons/Start/yp",10).toInt();
    iKeys[Start]= new VirtualKey(QRect(x,y,w,h), Qt::Key_Return, QRect(xp,yp,w,h));
    iKeys[Start]->img = QImage(path).scaled(iKeys[Start]->lRect.width(), iKeys[Start]->lRect.height());
    iKeys[Start]->file = path;
    path = config.value("buttons/Select/img",":/app/data/slbutton.png").toString();
    x = config.value( "buttons/Select/x",230).toInt();
    y = config.value( "buttons/Select/y",10).toInt();
    w = config.value( "buttons/Select/w",70).toInt();
    h = config.value( "buttons/Select/h",30).toInt();
    xp = config.value( "buttons/Select/xp",190).toInt();
    yp = config.value( "buttons/Select/yp",10).toInt();
    iKeys[Select]= new VirtualKey(QRect(x,y,w,h), Qt::Key_Shift, QRect(xp,yp,w,h));
    iKeys[Select]->img = QImage(path).scaled(iKeys[Select]->lRect.width(), iKeys[Select]->lRect.height());
    iKeys[Select]->file = path;
    path = config.value("buttons/TL/img",":/app/data/lbutton.png").toString();
    x = config.value( "buttons/TL/x",10).toInt();
    y = config.value( "buttons/TL/y",10).toInt();
    w = config.value( "buttons/TL/w",70).toInt();
    h = config.value( "buttons/TL/h",30).toInt();
    xp = config.value( "buttons/TL/xp",5).toInt();
    yp = config.value( "buttons/TL/yp",10).toInt();
    iKeys[TL]= new VirtualKey(QRect(x,y,w,h), Qt::Key_L, QRect(xp,yp,w,h));
    iKeys[TL]->img = QImage(path).scaled(iKeys[TL]->lRect.width(), iKeys[TL]->lRect.height());
    iKeys[TL]->file = path;
    path = config.value("buttons/TR/img",":/app/data/rbutton.png").toString();
    x = config.value( "buttons/TR/x",550).toInt();
    y = config.value( "buttons/TR/y",10).toInt();
    w = config.value( "buttons/TR/w",70).toInt();
    h = config.value( "buttons/TR/h",30).toInt();
    xp = config.value( "buttons/TR/xp",285).toInt();
    yp = config.value( "buttons/TR/yp",10).toInt();
    iKeys[TR]= new VirtualKey(QRect(x,y,w,h), Qt::Key_R, QRect(xp,yp,w,h));
    iKeys[TR]->img = QImage(path).scaled(iKeys[TR]->lRect.width(), iKeys[TR]->lRect.height());
    iKeys[TR]->file = path;
    path = config.value("buttons/Menu/img",":/app/data/mbutton.png").toString();
    x = config.value( "buttons/Menu/x",10).toInt();
    y = config.value( "buttons/Menu/y",312).toInt();
    w = config.value( "buttons/Menu/w",80).toInt();
    h = config.value( "buttons/Menu/h",40).toInt();
    xp = config.value( "buttons/Menu/xp",5).toInt();
    yp = config.value( "buttons/Menu/yp",50).toInt();
    iKeys[Menu]= new VirtualKey(QRect(x,y,w,h), Qt::Key_Escape, QRect(xp,yp,w,h));
    iKeys[Menu]->img = QImage(path).scaled(iKeys[Menu]->lRect.width(), iKeys[Menu]->lRect.height());
    iKeys[Menu]->file = path;
    /****************LOAD KEYMAP**************************/
    iKeys[Up]->key = (Qt::Key)config.value("keymap/UP",Qt::Key_Up).toInt();
    snesKeys[0] = iKeys[Up]->key;
    iKeys[Down]->key = (Qt::Key)config.value("keymap/DOWN",Qt::Key_Down).toInt();
    snesKeys[1] = iKeys[Down]->key;
    iKeys[Left]->key = (Qt::Key)config.value("keymap/LEFT",Qt::Key_Left).toInt();
    snesKeys[2] = iKeys[Left]->key;
    iKeys[Right]->key = (Qt::Key)config.value("keymap/RIGH",Qt::Key_Right).toInt();
    snesKeys[3] = iKeys[Right]->key;
    iKeys[A]->key = (Qt::Key)config.value("keymap/A",Qt::Key_A).toInt();
    snesKeys[4] = iKeys[A]->key;
    iKeys[B]->key = (Qt::Key)config.value("keymap/B",Qt::Key_B).toInt();
    snesKeys[5] = iKeys[B]->key;
    iKeys[X]->key = (Qt::Key)config.value("keymap/X",Qt::Key_X).toInt();
    snesKeys[6] = iKeys[X]->key;
    iKeys[Y]->key = (Qt::Key)config.value("keymap/Y",Qt::Key_Y).toInt();
    snesKeys[7] = iKeys[Y]->key;
    iKeys[TL]->key = (Qt::Key)config.value("keymap/L",Qt::Key_L).toInt();
    snesKeys[8] = iKeys[TL]->key;
    iKeys[TR]->key = (Qt::Key)config.value("keymap/R",Qt::Key_R).toInt();
    snesKeys[9] = iKeys[TR]->key;
    iKeys[Start]->key = (Qt::Key)config.value("keymap/START",Qt::Key_Return).toInt();
    snesKeys[10] = iKeys[Start]->key;
    iKeys[Select]->key = (Qt::Key)config.value("keymap/SELECT",Qt::Key_Shift).toInt();
    snesKeys[11] = iKeys[Select]->key;
    setvControlTransparency();
}

void MainWindow::saveVirtualKeys()
{
    QSettings config("./config.ini", QSettings::IniFormat);
    QObject *rmenuView, *rmenuViewp, *tmp, *tmp2, *tmp3, *tmp4;
    rmenuView = menuView->rootObject();
    rmenuViewp = menuViewp->rootObject();
    tmp = rmenuView->findChild<QObject*>("dpadbtn");
    tmp2 = rmenuView->findChild<QObject*>("upbtn");
    tmp3 = rmenuViewp->findChild<QObject*>("upbtn");
    tmp4 = rmenuViewp->findChild<QObject*>("dpadbtn");
    if (tmp && tmp2 && tmp3 && tmp4){
        config.setValue("buttons/Up/x", tmp->property("x").toInt()+ tmp2->property("x").toInt());
        config.setValue("buttons/Up/xp", tmp4->property("x").toInt()+ tmp3->property("x").toInt());
        config.setValue("buttons/Up/y", tmp->property("y").toInt()+ tmp2->property("y").toInt());
        config.setValue("buttons/Up/yp", tmp4->property("y").toInt()+ tmp3->property("y").toInt());
        config.setValue("buttons/Up/w", tmp2->property("width").toInt());
        config.setValue("buttons/Up/h", tmp2->property("height").toInt());
        config.setValue("buttons/Up/img", iKeys[Up]->file);
    }
    tmp2 = rmenuView->findChild<QObject*>("downbtn");
    tmp3 = rmenuViewp->findChild<QObject*>("downbtn");
    if (tmp && tmp2 && tmp3 && tmp4){
        config.setValue("buttons/Down/x", tmp->property("x").toInt()+ tmp2->property("x").toInt());
        config.setValue("buttons/Down/y", tmp->property("y").toInt()+ tmp2->property("y").toInt());
        config.setValue("buttons/Down/xp", tmp4->property("x").toInt()+ tmp3->property("x").toInt());
        config.setValue("buttons/Down/yp", tmp4->property("y").toInt()+ tmp3->property("y").toInt());
        config.setValue("buttons/Down/w", tmp2->property("width").toInt());
        config.setValue("buttons/Down/h", tmp2->property("height").toInt());
        config.setValue("buttons/Down/img", iKeys[Down]->file);
    }
    tmp2 = rmenuView->findChild<QObject*>("leftbtn");
    tmp3 = rmenuViewp->findChild<QObject*>("leftbtn");
    if (tmp && tmp2 && tmp3 && tmp4){
        config.setValue("buttons/Left/x", tmp->property("x").toInt()+ tmp2->property("x").toInt());
        config.setValue("buttons/Left/y", tmp->property("y").toInt()+ tmp2->property("y").toInt());
        config.setValue("buttons/Left/xp", tmp4->property("x").toInt()+ tmp3->property("x").toInt());
        config.setValue("buttons/Left/yp", tmp4->property("y").toInt()+ tmp3->property("y").toInt());
        config.setValue("buttons/Left/w", tmp2->property("width").toInt());
        config.setValue("buttons/Left/h", tmp2->property("height").toInt());
        config.setValue("buttons/Left/img", iKeys[Left]->file);
    }
    tmp2 = rmenuView->findChild<QObject*>("rightbtn");
    tmp3 = rmenuViewp->findChild<QObject*>("rightbtn");
    if (tmp && tmp2 && tmp3 && tmp4){
        config.setValue("buttons/Right/x", tmp->property("x").toInt()+ tmp2->property("x").toInt());
        config.setValue("buttons/Right/y", tmp->property("y").toInt()+ tmp2->property("y").toInt());
        config.setValue("buttons/Right/xp", tmp4->property("x").toInt()+ tmp3->property("x").toInt());
        config.setValue("buttons/Right/yp", tmp4->property("y").toInt()+ tmp3->property("y").toInt());
        config.setValue("buttons/Right/w", tmp2->property("width").toInt());
        config.setValue("buttons/Right/h", tmp2->property("height").toInt());
        config.setValue("buttons/Right/img", iKeys[Right]->file);
    }
    tmp = rmenuView->findChild<QObject*>("abtn");
    tmp2 = rmenuViewp->findChild<QObject*>("abtn");
    if (tmp && tmp2){
        config.setValue("buttons/C/x", tmp->property("x").toInt());
        config.setValue("buttons/C/y", tmp->property("y").toInt());
        config.setValue("buttons/C/xp", tmp2->property("x").toInt());
        config.setValue("buttons/C/yp", tmp2->property("y").toInt());
        config.setValue("buttons/C/w", tmp->property("width").toInt());
        config.setValue("buttons/C/h", tmp->property("height").toInt());
        config.setValue("buttons/C/img", iKeys[A]->file);
    }
    tmp = rmenuView->findChild<QObject*>("bbtn");
    tmp2 = rmenuViewp->findChild<QObject*>("bbtn");
    if (tmp && tmp2){
        config.setValue("buttons/X/x", tmp->property("x").toInt());
        config.setValue("buttons/X/y", tmp->property("y").toInt());
        config.setValue("buttons/X/xp", tmp2->property("x").toInt());
        config.setValue("buttons/X/yp", tmp2->property("y").toInt());
        config.setValue("buttons/X/w", tmp->property("width").toInt());
        config.setValue("buttons/X/h", tmp->property("height").toInt());
        config.setValue("buttons/X/img", iKeys[B]->file);
    }
    tmp = rmenuView->findChild<QObject*>("xbtn");
    tmp2 = rmenuViewp->findChild<QObject*>("xbtn");
    if (tmp && tmp2){
        config.setValue("buttons/T/x", tmp->property("x").toInt());
        config.setValue("buttons/T/y", tmp->property("y").toInt());
        config.setValue("buttons/T/xp", tmp2->property("x").toInt());
        config.setValue("buttons/T/yp", tmp2->property("y").toInt());
        config.setValue("buttons/T/w", tmp->property("width").toInt());
        config.setValue("buttons/T/h", tmp->property("height").toInt());
        config.setValue("buttons/T/img", iKeys[X]->file);
    }
    tmp = rmenuView->findChild<QObject*>("ybtn");
    tmp2 = rmenuViewp->findChild<QObject*>("ybtn");
    if (tmp && tmp2){
        config.setValue("buttons/S/x", tmp->property("x").toInt());
        config.setValue("buttons/S/y", tmp->property("y").toInt());
        config.setValue("buttons/S/xp", tmp2->property("x").toInt());
        config.setValue("buttons/S/yp", tmp2->property("y").toInt());
        config.setValue("buttons/S/w", tmp->property("width").toInt());
        config.setValue("buttons/S/h", tmp->property("height").toInt());
        config.setValue("buttons/S/img", iKeys[Y]->file);
    }
    tmp = rmenuView->findChild<QObject*>("lbtn");
    tmp2 = rmenuViewp->findChild<QObject*>("lbtn");
    if (tmp && tmp2){
        config.setValue("buttons/TL/x", tmp->property("x").toInt());
        config.setValue("buttons/TL/y", tmp->property("y").toInt());
        config.setValue("buttons/TL/xp", tmp2->property("x").toInt());
        config.setValue("buttons/TL/yp", tmp2->property("y").toInt());
        config.setValue("buttons/TL/w", tmp->property("width").toInt());
        config.setValue("buttons/TL/h", tmp->property("height").toInt());
        config.setValue("buttons/TL/img", iKeys[TL]->file);
    }
    tmp = rmenuView->findChild<QObject*>("rbtn");
    tmp2 = rmenuViewp->findChild<QObject*>("rbtn");
    if (tmp && tmp2){
        config.setValue("buttons/TR/x", tmp->property("x").toInt());
        config.setValue("buttons/TR/y", tmp->property("y").toInt());
        config.setValue("buttons/TR/xp", tmp2->property("x").toInt());
        config.setValue("buttons/TR/yp", tmp2->property("y").toInt());
        config.setValue("buttons/TR/w", tmp->property("width").toInt());
        config.setValue("buttons/TR/h", tmp->property("height").toInt());
        config.setValue("buttons/TR/img", iKeys[TR]->file);
    }
    tmp = rmenuView->findChild<QObject*>("stbtn");
    tmp2 = rmenuViewp->findChild<QObject*>("stbtn");
    if (tmp && tmp2){
        config.setValue("buttons/Start/x", tmp->property("x").toInt());
        config.setValue("buttons/Start/y", tmp->property("y").toInt());
        config.setValue("buttons/Start/xp", tmp2->property("x").toInt());
        config.setValue("buttons/Start/yp", tmp2->property("y").toInt());
        config.setValue("buttons/Start/w", tmp->property("width").toInt());
        config.setValue("buttons/Start/h", tmp->property("height").toInt());
        config.setValue("buttons/Start/img", iKeys[Start]->file);
    }
    tmp = rmenuView->findChild<QObject*>("slbtn");
    tmp2 = rmenuViewp->findChild<QObject*>("slbtn");
    if (tmp && tmp2){
        config.setValue("buttons/Select/x", tmp->property("x").toInt());
        config.setValue("buttons/Select/y", tmp->property("y").toInt());
        config.setValue("buttons/Select/xp", tmp2->property("x").toInt());
        config.setValue("buttons/Select/yp", tmp2->property("y").toInt());
        config.setValue("buttons/Select/w", tmp->property("width").toInt());
        config.setValue("buttons/Select/h", tmp->property("height").toInt());
        config.setValue("buttons/Select/img", iKeys[Select]->file);
    }
    tmp = rmenuView->findChild<QObject*>("menubtn");
    tmp2 = rmenuViewp->findChild<QObject*>("menubtn");
    if (tmp && tmp2){
        config.setValue("buttons/Menu/x", tmp->property("x").toInt());
        config.setValue("buttons/Menu/y", tmp->property("y").toInt());
        config.setValue("buttons/Menu/xp", tmp2->property("x").toInt());
        config.setValue("buttons/Menu/yp", tmp2->property("y").toInt());
        config.setValue("buttons/Menu/w", tmp->property("width").toInt());
        config.setValue("buttons/Menu/h", tmp->property("height").toInt());
        config.setValue("buttons/Menu/img", iKeys[Menu]->file);
    }
    config.sync();
    loadVirtualKeys();
    if( iRomLoaded){
        hideMenu();
    }
}

void MainWindow::loadConfig()
{
    QSettings config("./config.ini", QSettings::IniFormat);
    emu_frameskip = config.value("video/frameSkip", 0).toInt();
    emu_psx_bias = config.value("misc/bias", 2).toInt();;
    emu_audio_xa = config.value("audio/xa", 0).toInt();
    emu_video_mdec = config.value("video/mdec", 0).toInt();
    emu_audio_cd = config.value("audio/cdda", 0).toInt();
    emu_enable_bios = config.value("misc/bios", 1).toInt();
    emu_cpu_type = config.value("misc/cpu", 1).toInt();;
    emu_skip_gpu = config.value("video/gpuskip", 0).toInt();
    emu_frame_limit = config.value("video/limitframes", 1).toInt();
    emu_video_light = config.value("video/light", 1).toInt();
    emu_video_blend = config.value("video/blend", 1).toInt();
    emu_clock = config.value("misc/clock", 2).toInt();
    emu_clock_multiplier = 1.0f / (float)(emu_clock + 1);
    emu_framerate = config.value("video/framerate", 0).toInt();
    emu_sound_enable = config.value("audio/enable", 1).toInt();
    emu_sound_rate = config.value("audio/rate", 0).toInt();
    iVCTransparency = config.value("misc/vctransparency", 5).toInt();
    emu_volume = config.value("audio/volume", 50).toInt();
    iSkipBlit = config.value("video/blitSkip", 0).toInt();
    iZoom = config.value("video/zoom", 0).toInt();
    bSmoothImage = config.value("video/smooth", false).toBool();
    sRomFile = config.value("misc/lastRom").toString();
    iOrientation = config.value("misc/orientation").toInt();
    sRomDir = config.value("misc/romdir", "E:/data").toString();
    sSaveDir = config.value("misc/savedir", "E:/data").toString();
    sSavePath = sSaveDir;
    if( !iOrientation){
        setAttribute( Qt::WA_AutoOrientation);
    }else if( iOrientation == 1){
        setAttribute( Qt::WA_LockLandscapeOrientation);
        bLandscape = true;
    }else{
        setAttribute( Qt::WA_LockPortraitOrientation);
        bLandscape = false;
    }
    switch(emu_sound_rate){
    case 0: iBuffer_Size = 512; break;
    case 1: iBuffer_Size = 1024; break;
    case 2: iBuffer_Size = 2048; break;
    case 3: iBuffer_Size = 4096; break;
    default: iBuffer_Size = 512; break;
    }
    QObject *rmenuView, *tmp;
    for(int i=0; i< 2; i++){
        if( !i )
            rmenuView = menuView->rootObject();
        else
            rmenuView = menuViewp->rootObject();
        tmp = rmenuView->findChild<QObject*>("senaBox");
        if (tmp) tmp->setProperty("checked", emu_sound_enable);
        tmp = rmenuView->findChild<QObject*>("lightBox");
        if (tmp) tmp->setProperty("checked", emu_video_light);
        tmp = rmenuView->findChild<QObject*>("blendBox");
        if (tmp) tmp->setProperty("checked", emu_video_blend);
        tmp = rmenuView->findChild<QObject*>("sgpuBox");
        if (tmp) tmp->setProperty("checked", emu_skip_gpu);
        tmp = rmenuView->findChild<QObject*>("biosBox");
        if (tmp) tmp->setProperty("checked", emu_enable_bios);
        tmp = rmenuView->findChild<QObject*>("mdecBox");
        if (tmp) tmp->setProperty("checked", emu_video_mdec);
        tmp = rmenuView->findChild<QObject*>("frameBox");
        if (tmp) tmp->setProperty("checked", emu_framerate);
        tmp = rmenuView->findChild<QObject*>("xaBox");
        if (tmp) tmp->setProperty("checked", emu_audio_xa);
        tmp = rmenuView->findChild<QObject*>("cdaBox");
        if (tmp) tmp->setProperty("checked", emu_audio_cd);
        tmp = rmenuView->findChild<QObject*>("flimBox");
        if (tmp) tmp->setProperty("checked", emu_frame_limit);
        tmp = rmenuView->findChild<QObject*>("smoothBox");
        if (tmp) tmp->setProperty("checked", bSmoothImage);
        tmp = rmenuView->findChild<QObject*>("sblitBox");
        if (tmp) tmp->setProperty("checked", iSkipBlit);
        tmp = rmenuView->findChild<QObject*>("orientdial");
        if (tmp) tmp->setProperty("selectedIndex", iOrientation);
        tmp = rmenuView->findChild<QObject*>("cpuDial");
        if (tmp) tmp->setProperty("selectedIndex", emu_cpu_type);
        tmp = rmenuView->findChild<QObject*>("biasDial");
        if (tmp) tmp->setProperty("selectedIndex", emu_psx_bias);
        tmp = rmenuView->findChild<QObject*>("fskipdial");
        if (tmp) tmp->setProperty("selectedIndex", emu_frameskip);
        tmp = rmenuView->findChild<QObject*>("sratedial");
        if (tmp) tmp->setProperty("selectedIndex", emu_sound_rate);
        tmp = rmenuView->findChild<QObject*>("isizedial");
        if (tmp) tmp->setProperty("selectedIndex", iZoom);
        tmp = rmenuView->findChild<QObject*>("clockDial");
        if (tmp) tmp->setProperty("selectedIndex",(emu_clock));
        tmp = rmenuView->findChild<QObject*>("volumeSlider");
        if (tmp) tmp->setProperty("value", emu_volume);
        tmp = rmenuView->findChild<QObject*>("vctranslider");
        if (tmp) tmp->setProperty("value", iVCTransparency);
        rmenuView->setProperty("romdir", sRomDir);
        rmenuView->setProperty("savedir", sSaveDir);
    }
    emuChangeConfig();
    setvControlTransparency();
    changeVolume(emu_volume);
    if( !config.contains("video/frameSkip")) saveConfig();
    raiseMenu();
}

void MainWindow::saveConfig()
{
    QObject *rmenuView;
    if( bLandscape )
        rmenuView = menuView->rootObject();
    else
        rmenuView = menuViewp->rootObject();

    QObject *tmp = rmenuView->findChild<QObject*>("senaBox");
    if (tmp) emu_sound_enable = tmp->property("checked").toBool();
    tmp = rmenuView->findChild<QObject*>("lightBox");
    if (tmp) emu_video_light = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("blendBox");
    if (tmp) emu_video_blend = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("sgpuBox");
    if (tmp) emu_skip_gpu = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("biosBox");
    if (tmp) emu_enable_bios = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("mdecBox");
    if (tmp) emu_video_mdec = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("frameBox");
    if (tmp) emu_framerate = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("xaBox");
    if (tmp) emu_audio_xa = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("cdaBox");
    if (tmp) emu_audio_cd = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("flimBox");
    if (tmp) emu_frame_limit = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("frameBox");
    if (tmp) emu_framerate = tmp->property("checked").toInt();
    tmp = rmenuView->findChild<QObject*>("sblitBox");
    if (tmp) iSkipBlit = tmp->property("checked").toBool();
    tmp = rmenuView->findChild<QObject*>("smoothBox");
    if (tmp) bSmoothImage = tmp->property("checked").toBool();
    tmp = rmenuView->findChild<QObject*>("vctranslider");
    if (tmp) iVCTransparency = tmp->property("value").toInt();
    tmp = rmenuView->findChild<QObject*>("cpuDial");
    if (tmp) emu_cpu_type = tmp->property("selectedIndex").toInt();
    tmp = rmenuView->findChild<QObject*>("biasDial");
    if (tmp) emu_psx_bias = tmp->property("selectedIndex").toInt();
    tmp = rmenuView->findChild<QObject*>("orientdial");
    if (tmp) iOrientation = tmp->property("selectedIndex").toInt();
    tmp = rmenuView->findChild<QObject*>("fskipdial");
    if (tmp) emu_frameskip = tmp->property("selectedIndex").toInt();
    tmp = rmenuView->findChild<QObject*>("clockDial");
    if (tmp) emu_clock = tmp->property("selectedIndex").toInt();
    tmp = rmenuView->findChild<QObject*>("sratedial");
    if (tmp) emu_sound_rate = tmp->property("selectedIndex").toInt();
    tmp = rmenuView->findChild<QObject*>("isizedial");
    if (tmp) iZoom = tmp->property("selectedIndex").toInt();
    tmp = rmenuView->findChild<QObject*>("volumeSlider");
    if (tmp) emu_volume = tmp->property("value").toInt();
    switch(emu_sound_rate){
    case 0: iBuffer_Size = 512; break;
    case 1: iBuffer_Size = 1024; break;
    case 2: iBuffer_Size = 2048; break;
    case 3: iBuffer_Size = 4096; break;
    default: iBuffer_Size = 512; break;
    }
    QSettings config("./config.ini", QSettings::IniFormat);
    config.setValue("video/frameSkip", emu_frameskip);
    config.setValue("video/framerate", emu_framerate);
    config.setValue("audio/enable", emu_sound_enable);
    config.setValue("audio/rate", emu_sound_rate);
    config.setValue("audio/volume", emu_volume);
    config.setValue("misc/bias",emu_psx_bias);
    config.setValue("audio/xa",emu_audio_xa);
    config.setValue("video/mdec",emu_video_mdec);
    config.setValue("audio/cdda",emu_audio_cd);
    config.setValue("misc/bios",emu_enable_bios);
    config.setValue("misc/cpu",emu_cpu_type);
    config.setValue("video/gpuskip",emu_skip_gpu);
    config.setValue("video/limitframes",emu_frame_limit);
    config.setValue("video/light",emu_video_light);
    config.setValue("video/blend",emu_video_blend);
    config.setValue("misc/clock",emu_clock);
    config.setValue("video/blitSkip", iSkipBlit);
    config.setValue("video/smooth", bSmoothImage);
    config.setValue("video/zoom", iZoom);
    config.setValue("misc/orientation", iOrientation);
    config.setValue("misc/romdir", sRomDir);
    config.setValue("misc/savedir", sSaveDir);
    if( iRomLoaded )
        config.setValue("misc/lastRom", sRomFile);
    config.sync();
    loadConfig();
    if( iRomLoaded){
        hideMenu();
    }
}

void MainWindow::focusInEvent( QFocusEvent * ){
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QSize winSize = event->size();
    bLandscape = winSize.width() > winSize.height();
    if( menuView->isVisible() || menuViewp->isVisible()){
        if( bLandscape){
            if(menuView) menuView->showFullScreen();
            if(menuViewp) menuViewp->hide();
        }else{
            if(menuView) menuView->hide();
            if(menuViewp) menuViewp->showFullScreen();
        }
        pauseEmu();
    }
    ui->centralWidget->resize(winSize);
}

void MainWindow::resumeEmu(){
    if( audioOut ) audioOut->resume();
    if( loopTimer ) loopTimer->start(0);
    if( iRomLoaded ) bEmulating = true;
}

void MainWindow::setvControlTransparency(){
    QColor color;
    QMap<Buttons, VirtualKey*>::iterator i;
    for (i = iKeys.begin(); i != iKeys.end(); ++i){
        for ( int x = 0; x < i.value()->img.width(); x++ )	{
            for ( int y = 0; y < i.value()->img.height(); y++ ) {
                color.setRgba(i.value()->img.pixel( x, y ));
                if(color.alpha()!=0) color.setAlpha((iVCTransparency + 1) * 25);
                i.value()->img.setPixel( x, y, color.rgba() );
            }
        }
        i.value()->pix = QPixmap::fromImage(i.value()->img);
    }
}

void MainWindow::focusOutEvent( QFocusEvent * ){
    pauseEmu();
}

void MainWindow::closeEvent( QCloseEvent * ){
    exitEmu();
}

void MainWindow::emuContinue(){
    if (!iRomLoaded){
        loadRomFile(sRomFile);
        loadConfig();
    }
    if( iRomLoaded){
        hideMenu();
    }
}

void MainWindow::pauseEmu(){
    bEmulating = false;
    if( loopTimer ) loopTimer->stop();
    if( audioOut ) audioOut->suspend();
}

void MainWindow::setGameState(int state){
    iGameState = state;
    emu_state = iGameState;
    QObject *root = menuView->rootObject();
    QObject *tmp = root->findChild<QObject*>("savePathView");
    if (tmp){
        tmp->setProperty("currentIndex", iGameState);
    }
    root = menuViewp->rootObject();
    tmp = root->findChild<QObject*>("savePathView");
    if (tmp){
        tmp->setProperty("currentIndex", iGameState);
    }
    if(state != 6 ) loadSnapImg();
}

void MainWindow::setZoom(int zoom){
    iZoom = zoom;
}

void MainWindow::raiseMenu(){
    pauseEmu();
#if defined(Q_WS_S60) or defined(Q_WS_SIMULATOR)
    //emuMenu->showFullScreen();
    if( bLandscape )
        menuView->showFullScreen();
    else
        menuViewp->showFullScreen();

#else
    QObject *root = menuView->rootObject();
    if (root)
        root->setProperty("visible", "true");
    //emuMenu->show();
    menuView->show();
#endif
}

void MainWindow::hideMenu(){
    if(menuView) menuView->hide();
    if(menuViewp) menuViewp->hide();
    resumeEmu();
}

void MainWindow::loadState(){
    if(iRomLoaded){
        emu_state = iGameState;
        emuLoadState();
        loadSnapImg();
        hideMenu();
    }
}

void MainWindow::saveState(){
    if(iRomLoaded){
        emu_state = iGameState;
        emuSaveState();
        saveSnapImg();
        hideMenu();
    }
}

void MainWindow::saveSnapImg(){
    if(iRomLoaded){
        QFileInfo fi(sRomFile);
        QString path(sSaveDir);
        path.append("/");
        path += fi.fileName();
        path.append(".");
        QString temp;
        path.append(temp.setNum(iGameState));
        path.append(".jpg");
        QImage out = emuFrameBuffer->copy(0, 0, isWidth, isHeight);
        out.save(path);
        loadSnapImg();
    }
}

void MainWindow::loadSnapImg(){
    emit UpdateImgs();
}

void MainWindow::setDevice()
{
    connect(audioOut, SIGNAL(stateChanged (QAudio::State)), this,  SLOT(processAudio(QAudio::State)));
    if(audioOut) audioDevice = audioOut->start();
#ifdef Q_OS_SYMBIAN
    unsigned int *pointer_to_abstract_audio = (unsigned int*)( (unsigned char*)audioOut + 8 );
    unsigned int *dev_sound_wrapper = (unsigned int*)(*pointer_to_abstract_audio) + 13;
    unsigned int *temp = ((unsigned int*)(*dev_sound_wrapper) + 6);
    CMMFDevSound *dev_sound = (CMMFDevSound*)(*temp);
    int volume = (dev_sound->MaxVolume()/100)*emu_volume;
    dev_sound->SetVolume(volume);
#endif
}

void MainWindow::changeVolume(int i)
{
#ifdef Q_OS_SYMBIAN
    if( audioOut ){
        unsigned int *pointer_to_abstract_audio = (unsigned int*)( (unsigned char*)audioOut + 8 );
        unsigned int *dev_sound_wrapper = (unsigned int*)(*pointer_to_abstract_audio) + 13;
        unsigned int *temp = ((unsigned int*)(*dev_sound_wrapper) + 6);
        CMMFDevSound *dev_sound = (CMMFDevSound*)(*temp);
        int volume = (dev_sound->MaxVolume()/100)*i;
        dev_sound->SetVolume(volume);
    }
#endif
    return;
}

void MainWindow::unsetDevice()
{
    if(audioOut)audioOut->stop();
}

void MainWindow::runEmuLoop()
{
    if( bEmulating && iRomLoaded){
        if( iSampleCounter >= iBuffer_Size ) processAudio(QAudio::ActiveState);
        while( !iRefresh ){
            emuLoop();
        }
        iRefresh = 0;
    }
}

void MainWindow::refresh(int type)
{
    switch(type){
    case 1: break;
    case 2:{ processAudio(QAudio::ActiveState); break;}
    case 3:{ updateScreen( emu_screen_width, emu_screen_height); break;} //video refress
    default: break;
    }
    iRefresh = type;
}
void MainWindow::reset()
{
    if( !iRomLoaded ) return;
    emuReset();
    hideMenu();
}

void MainWindow::setKeys(int w, int codeint)
{
    snesKeys[w] = codeint;
    Qt::Key code = (Qt::Key)codeint;
    switch(w){
    case 0: iKeys[Up]->key = code; break;
    case 1: iKeys[Down]->key = code; break;
    case 2: iKeys[Left]->key = code; break;
    case 3: iKeys[Right]->key = code; break;
    case 4: iKeys[A]->key = code; break;
    case 5: iKeys[B]->key = code; break;
    case 6: iKeys[X]->key = code; break;
    case 7: iKeys[Y]->key = code; break;
    case 8: iKeys[TL]->key = code; break;
    case 9: iKeys[TR]->key = code; break;
    case 10: iKeys[Start]->key = code; break;
    case 11: {
        iKeys[Select]->key = code;
        QSettings config("./config.ini", QSettings::IniFormat);
        config.setValue("keymap/UP", iKeys[Up]->key);
        config.setValue("keymap/DOWN", iKeys[Down]->key);
        config.setValue("keymap/LEFT", iKeys[Left]->key);
        config.setValue("keymap/RIGHT", iKeys[Right]->key);
        config.setValue("keymap/A", iKeys[A]->key);
        config.setValue("keymap/B", iKeys[B]->key);
        config.setValue("keymap/X", iKeys[X]->key);
        config.setValue("keymap/Y", iKeys[Y]->key);
        config.setValue("keymap/L", iKeys[TL]->key);
        config.setValue("keymap/R", iKeys[TR]->key);
        config.setValue("keymap/SELECT", iKeys[Select]->key);
        config.setValue("keymap/START", iKeys[Start]->key);
        config.sync();
        hideMenu();
        break;
    }
    default: break;
    }

}

void MainWindow::processAudio( QAudio::State state )
{
    if (bEmulating && iRomLoaded && emu_sound_enable){
        if(audioDevice && emuAudioBuffer){
            audioDevice->write( *emuAudioBuffer , iSampleCounter);
            iSampleCounter = 0;
        }
    }else{
        iSampleCounter = 0;
    }
}

void MainWindow::getRomFile(){
    pauseEmu();
    loadConfig();
    loadRomFile(QFileDialog::getOpenFileName(this,
                                             tr("Open Rom"), sRomDir, tr("Rom Files (*.smc *.sfc *.zip *.bin)")));
    if( iRomLoaded ){
        QSettings config("./config.ini", QSettings::IniFormat);
        config.setValue("misc/lastRom", sRomFile);
        config.sync();
        hideMenu();
    }else
        raiseMenu();
}

void MainWindow::loadRomFile( QString romfile){
    if(romfile.isEmpty()) return;
    if(iRomLoaded && romfile == sRomFile) return;
    int current_state = emu_state;
    emu_state = 6;
    if(!emuInited){
        emuInit(PSX);
        emuInited = true;
    }else{
        emuClose();
        emuInit(PSX);
    }
    if(iRomLoaded){
        emuSaveState();
    }
    sRomFile = romfile;
    if(emuLoadRom(&sRomFile)){
        iRomLoaded = 1;
        //emuLoadState();
        loadSnapImg();
    }
    emu_state = current_state;
}

void MainWindow::softEventCreator( QPoint bPoint, QEvent::Type type, int press, bool end){
    QMap<Buttons, VirtualKey*>::iterator i;
    for (i = iKeys.begin(); i != iKeys.end(); ++i){
        if( i.value()->isOver(bPoint, bLandscape) ){
            if( i.value()->iPressed == 0 || type == QEvent::KeyRelease){ //se estava solto ou é evento de soltar botão
                QApplication::sendEvent(this, new QKeyEvent (type, i.value()->key, Qt::NoModifier));
                if( type == QEvent::KeyRelease)
                    i.value()->iPressed = 0;
                else
                    i.value()->iPressed = 1 ;
            }else{ // se estava marcado para liberar
                i.value()->iPressed = 1 ;
            }
        }else if(i.value()->iPressed == 1 && press == 0 && !end){
            i.value()->iPressed = -1;
        }else if(i.value()->iPressed == 1 && press == 0 && end){
            QApplication::sendEvent(this, new QKeyEvent(QEvent::KeyRelease, i.value()->key, Qt::NoModifier));
            i.value()->iPressed = 0;
        }else if(i.value()->iPressed == -1 && end){
            QApplication::sendEvent(this, new QKeyEvent(QEvent::KeyRelease, i.value()->key, Qt::NoModifier));
            i.value()->iPressed = 0;
        }
    }
}

bool MainWindow::event(QEvent *event) //Thanks Summeli
{
    QPoint bPoint;
#ifndef __SYMBIAN32__
    QMouseEvent * _event;
#endif
    switch (event->type())
    {
#ifdef __SYMBIAN32__
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    {
        QList<QTouchEvent::TouchPoint> touchPoints = (static_cast<QTouchEvent*>(event))->touchPoints();
        bool last = false;
        for ( int i = 0; i < touchPoints.length(); i++ )
        {
            QTouchEvent::TouchPoint tp = touchPoints[i];

            if( i == (touchPoints.length() - 1) ) last = true;

            if ( tp.state() == Qt::TouchPointPressed || tp.state() == Qt::TouchPointMoved || tp.state() == Qt::TouchPointStationary)
            {
                bPoint = QPoint( tp.screenPos().x(), tp.screenPos().y());
                softEventCreator( bPoint, QEvent::KeyPress, i, last);
            }else if(tp.state() == Qt::TouchPointReleased ){
                bPoint = QPoint( tp.screenPos().x(), tp.screenPos().y());
                softEventCreator( bPoint, QEvent::KeyRelease, i, last);
            }
        }
        event->accept();
        return true;
    }
    case QEvent::TouchEnd:
    {
        QList<QTouchEvent::TouchPoint> touchPoints = (static_cast<QTouchEvent*>(event))->touchPoints();
        bool last = false;
        for ( int i = 0; i < touchPoints.length(); i++ )
        {
            if( i == (touchPoints.length() - 1)) last = 1;
            QTouchEvent::TouchPoint tp = touchPoints[i];
            bPoint = QPoint( tp.screenPos().x(), tp.screenPos().y());
            softEventCreator( bPoint, QEvent::KeyRelease, i, last);
        }
        break;
    }
#else
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
        _event = (static_cast<QMouseEvent*>(event));
        bPoint = QPoint( _event->x(), _event->y());
        softEventCreator( bPoint, QEvent::KeyPress, 0, true);
        break;
    case QEvent::MouseButtonRelease:
        _event = (static_cast<QMouseEvent*>(event));
        bPoint = QPoint( _event->x(), _event->y());
        softEventCreator( bPoint, QEvent::KeyRelease, 0, true);
        break;

#endif
    default:
        break;
    }
    return QWidget::event(event);
}


void MainWindow::keyPressEvent( QKeyEvent *event){
    if(event->key() == Qt::Key_Escape) raiseMenu();
    emuProcessKeys( event->key(), true);
}

void MainWindow::keyReleaseEvent( QKeyEvent *event){
    QMap<Buttons, int >::iterator i;
    emuProcessKeys( event->key(), false);
}

void MainWindow::sleep(int msecs){
#ifdef __SYMBIAN32__
    User::AfterHighRes(msecs);
#endif
}

void MainWindow::setMessage(char* message, int x, int y){
    sInfo = message;
    infoX = x;
    infoY = y;
}

int framecount = 0;
int framerate = 0;
void MainWindow::updateScreen(int width, int height){
    static int oldtime = 0;
    if( oldtime == 0 ) oldtime = timeCounter->elapsed();
    if( timeCounter->elapsed() >= oldtime + 1000 ){
        oldtime = timeCounter->elapsed();
        framerate = framecount;
        framecount = 0;
    }
    isWidth = width;
    isHeight = height;
    static int fcounter = 0;
    if( fcounter && iSkipBlit){
        fcounter = 0;
        framecount++;
        return;
    }
    fcounter++;
    repaint();
}

void MainWindow::paintEvent( QPaintEvent * event){
    QPainter painter;
    QRectF target, source;
    QString fpsStr;
    painter.begin(this);
    if( bSmoothImage )painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if( iRomLoaded && bEmulating ){
        source = QRectF(0.0, 0.0, isWidth, isHeight);
        if(iZoom){
            target = QRectF(0.0, 0.0, ui->centralWidget->size().width(), ui->centralWidget->size().height());
        }else{
            if( bLandscape)
                target = QRectF(80.0, 0.0, 480, 360);
            else
                target = QRectF(0.0, 100.0, 360, 320);
        }
        painter.drawImage(target, *emuFrameBuffer, source);
        framecount++;
        if( !sInfo.isNull()) painter.drawText( infoX, infoY + 10, sInfo);
        if( emu_framerate){
            fpsStr = "FPS: ";
            fpsStr += QString().setNum(framerate);
            painter.drawText(10,50,fpsStr);
        }
    }
    QMap<Buttons, VirtualKey*>::iterator i;
    if( iVCTransparency ){
        for (i = iKeys.begin(); i != iKeys.end(); ++i){
            if( bLandscape )
                painter.drawPixmap(i.value()->lRect.x()-(i.value()->pix.width()/2)+(i.value()->lRect.width()/2), i.value()->lRect.y()-(i.value()->pix.height()/2)+(i.value()->lRect.height()/2), i.value()->pix);
            else
                painter.drawPixmap(i.value()->pRect.x()-(i.value()->pix.width()/2)+(i.value()->pRect.width()/2), i.value()->pRect.y()-(i.value()->pix.height()/2)+(i.value()->pRect.height()/2), i.value()->pix);
        }
    }
    painter.end();
}
