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
 
#ifndef EMUINTERFACE_H
#define EMUINTERFACE_H
#include <QString>
#include <QImage>
#include <QByteArray>

enum EmuType{ SNES, NES, GENESIS, GAMEBOY, PSX };

int emuLoadRom( QString * romFile );
int emuInit( EmuType type);
int emuClose();
int emuLoop();
int emuReset();
int InitAudio();
void emuSaveState();
void emuLoadState();
int emuProcessKeys(int key, bool pressed);
int emuChangeConfig();

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
#endif // EMUINTERFACE_H
