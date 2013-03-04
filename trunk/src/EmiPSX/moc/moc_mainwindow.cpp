/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created: Mon 4. Mar 01:14:11 2013
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../mainwindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MainWindow[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
      27,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x05,
      25,   11,   11,   11, 0x05,
      40,   11,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
      56,   11,   11,   11, 0x0a,
      69,   11,   11,   11, 0x0a,
      79,   11,   11,   11, 0x0a,
      94,   92,   11,   11, 0x0a,
     116,   11,   11,   11, 0x0a,
     129,   11,   11,   11, 0x0a,
     137,   11,   11,   11, 0x0a,
     149,   11,   11,   11, 0x0a,
     160,   11,   11,   11, 0x0a,
     173,   11,   11,   11, 0x0a,
     184,   11,   11,   11, 0x0a,
     202,  196,   11,   11, 0x0a,
     220,   11,   11,   11, 0x0a,
     251,  246,   11,   11, 0x0a,
     264,   11,   11,   11, 0x0a,
     276,   11,   11,   11, 0x0a,
     288,   11,   11,   11, 0x0a,
     306,  302,   11,   11, 0x0a,
     327,  196,   11,   11, 0x0a,
     355,   11,   11,   11, 0x0a,
     373,   11,   11,   11, 0x0a,
     385,   11,   11,   11, 0x0a,
     405,  398,   11,   11, 0x0a,
     423,   92,   11,   11, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_MainWindow[] = {
    "MainWindow\0\0UpdateImgs()\0UpdateRomDir()\0"
    "UpdateSaveDir()\0loadConfig()\0exitEmu()\0"
    "saveConfig()\0,\0updateScreen(int,int)\0"
    "runEmuLoop()\0reset()\0raiseMenu()\0"
    "hideMenu()\0getRomFile()\0pauseEmu()\0"
    "resumeEmu()\0state\0setGameState(int)\0"
    "setvControlTransparency()\0zoom\0"
    "setZoom(int)\0saveState()\0loadState()\0"
    "emuContinue()\0rom\0loadRomFile(QString)\0"
    "processAudio(QAudio::State)\0"
    "saveVirtualKeys()\0getRomDir()\0"
    "getSaveDir()\0volume\0changeVolume(int)\0"
    "setKeys(int,int)\0"
};

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow,
      qt_meta_data_MainWindow, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MainWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: UpdateImgs(); break;
        case 1: UpdateRomDir(); break;
        case 2: UpdateSaveDir(); break;
        case 3: loadConfig(); break;
        case 4: exitEmu(); break;
        case 5: saveConfig(); break;
        case 6: updateScreen((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 7: runEmuLoop(); break;
        case 8: reset(); break;
        case 9: raiseMenu(); break;
        case 10: hideMenu(); break;
        case 11: getRomFile(); break;
        case 12: pauseEmu(); break;
        case 13: resumeEmu(); break;
        case 14: setGameState((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 15: setvControlTransparency(); break;
        case 16: setZoom((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 17: saveState(); break;
        case 18: loadState(); break;
        case 19: emuContinue(); break;
        case 20: loadRomFile((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 21: processAudio((*reinterpret_cast< QAudio::State(*)>(_a[1]))); break;
        case 22: saveVirtualKeys(); break;
        case 23: getRomDir(); break;
        case 24: getSaveDir(); break;
        case 25: changeVolume((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 26: setKeys((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
        _id -= 27;
    }
    return _id;
}

// SIGNAL 0
void MainWindow::UpdateImgs()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void MainWindow::UpdateRomDir()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void MainWindow::UpdateSaveDir()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}
QT_END_MOC_NAMESPACE
