#ifndef VIRTUALKEY_H
#define VIRTUALKEY_H
#include <Qt>
#include <QRect>
#include <QImage>
#include <QPixmap>

class VirtualKey{
public:
    QRect pRect;
    QRect lRect;
    Qt::Key key;
    int iPressed;
    QImage img;
    QImage pressimg;
    QString file;
    QPixmap pix;
    VirtualKey(QRect _rectl, Qt::Key _key, QRect _rectp);
    VirtualKey();
    ~VirtualKey();
    bool isOver( QPoint mpoint, bool landscape);
    QImage getImage();
};

#endif // VIRTUALKEY_H
