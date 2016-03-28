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
 
#include <QtGui/QApplication>
#include <QDebug>
#if defined(Q_WS_S60) or defined(Q_WS_SIMULATOR)
#include <QSystemScreenSaver>
QTM_USE_NAMESPACE
#endif
#include "mainwindow.h"
#include "relocutils.h"
#include "symbian_memory_handler.h"
#include <QDebug>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include "CImageProvider.cpp"
MainWindow* emuWindow;
QDeclarativeView *menuView;
QDeclarativeView *menuViewp;

int main(int argc, char *argv[])
{
    //BEGIN_RELOCATED_CODE(0x10000000);
    QApplication a(argc, argv);
    emuWindow = new MainWindow;
    menuView = new QDeclarativeView(emuWindow);
    if(menuView){
        menuView->engine()->addImageProvider("saves", new ImageProvider);
        menuView->setSource(QUrl("qrc:/app/data/emuui.qml"));
        if( menuView->status() != 1)
            qDebug()<< menuView->errors();
    }
    menuViewp = new QDeclarativeView(emuWindow);
    if(menuViewp){
        menuViewp->engine()->addImageProvider("saves", new ImageProvider);
        menuViewp->setSource(QUrl("qrc:/app/data/emuuip.qml"));
        if( menuViewp->status() != 1)
            qDebug()<< menuViewp->errors();
    }
    emuWindow->init();
#if defined(Q_WS_S60) or defined(Q_WS_SIMULATOR)
    emuWindow->showFullScreen();
    emuWindow->loadConfig();
#ifndef RELEASE
    QSystemScreenSaver *sss=new QSystemScreenSaver(emuWindow);
    sss->setScreenSaverInhibit();
#endif
#else
    emuWindow->show();
    emuWindow->loadConfig();
#endif
    return a.exec();
    //END_RELOCATED_CODE();
}
