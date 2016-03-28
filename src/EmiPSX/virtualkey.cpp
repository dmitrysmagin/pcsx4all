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
