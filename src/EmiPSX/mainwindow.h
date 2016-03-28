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
 
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QGraphicsScene>
#include <QTimer>
#include <QTime>
#include <QPixmap>
#include <QImage>
#include <QTextItem>
#include <QRect>
#include <QEvent>
#include <QFileDialog>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QSettings>
#include "virtualkey.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    static uint* GetKeyState(int);
    QKeyEvent* pollEvents();
    int init();
    void setDevice();
    void unsetDevice();
    void saveSnapImg();
    void loadSnapImg();
    void loadVirtualKeys();
    void refresh(int type);
    void sleep(int msecs);
    ~MainWindow();
    QTime* timeCounter;
    QAudioOutput * audioOut;
    QByteArray *audioBuffer;
    QString sSaveDir;
    int iSampleCounter;
    void setMessage(char* text, int x, int y);


public slots:
    void loadConfig();
    void exitEmu();
    void saveConfig();
    void updateScreen(int, int);
    void runEmuLoop();
    void reset();
    void raiseMenu();
    void hideMenu();
    void getRomFile();
    void pauseEmu();
    void resumeEmu();
    void setGameState(int state);
    void setvControlTransparency();
    void setZoom(int zoom);
    void saveState();
    void loadState();
    void emuContinue();
    void loadRomFile(QString rom);
    void processAudio( QAudio::State state);
    void saveVirtualKeys();
    void getRomDir();
    void getSaveDir();
    void changeVolume(int volume);
    void setKeys(int, int);

signals:
    void UpdateImgs(void);
    void UpdateRomDir(void);
    void UpdateSaveDir(void);

protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void closeEvent(QCloseEvent *);
    virtual void focusOutEvent(QFocusEvent * );
    virtual void keyPressEvent( QKeyEvent *event);
    virtual void keyReleaseEvent( QKeyEvent *event);
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    bool event(QEvent *event);
    void softEventCreator( QPoint bPoint, QEvent::Type type, int press, bool end);
    enum Buttons { Up, Down, Left, Right, Start, Select,
                    TR, TL, A, B, X, Y, Menu};
    QMap<Buttons, VirtualKey* > iKeys;
    QMap<Buttons, int > iKeymap;
    Ui::MainWindow *ui;
    QTimer* loopTimer;
    QTimer* secondTimer;
    QImage* subScreen;
    int iCounter;
    int ishowText;
    int icurrRate;
    int iRomLoaded;
    int iGameState;
    int iSkipBlit;
    QString* tframeRate;
    QString* tText;
    int isWidth;
    int isHeight;
    QIODevice * audioDevice;
    bool bEmulating;
    int iZoom;
    QImage *controlImg;
    int iVCTransparency;
    int iBuffer_Size;
    bool bLandscape;
    int iMaxSaveStates;
    bool emuInited;
    QString sRomDir;
    bool bSmoothImage;
    int iOrientation;
    int iRefresh;
    QString sInfo;
    int infoX, infoY;
};

#endif // MAINWINDOW_H
