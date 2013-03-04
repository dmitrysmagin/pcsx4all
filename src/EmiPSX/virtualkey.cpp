#include "virtualkey.h"


VirtualKey::VirtualKey(){
}

VirtualKey::VirtualKey(QRect _rectl, Qt::Key _key, QRect _rectp = QRect()){
    lRect = _rectl;
    pRect = _rectp;
    key = _key;
    iPressed = 0;
}

VirtualKey::~VirtualKey()
{
}

bool VirtualKey::isOver(QPoint mpoint, bool landscape)
{
    if( landscape )
        return lRect.contains( mpoint, true);
    else
        return pRect.contains( mpoint, true);
}

QImage VirtualKey::getImage()
{
    if( iPressed )
        return pressimg;
    else
        return img;
}
