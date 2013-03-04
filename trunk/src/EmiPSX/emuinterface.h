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
