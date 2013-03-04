/**** QML image provider from QML Examples *****/
 #include <qdeclarativeengine.h>
 #include <qdeclarative.h>
 #include <qdeclarativeitem.h>
 #include <qdeclarativeimageprovider.h>
 #include <qdeclarativeview.h>
 #include <QImage>
 #include <QPainter>
#include <QString>

extern QString sRomFile;
extern QString sSavePath;

 class ImageProvider : public QDeclarativeImageProvider
 {
 public:
     ImageProvider()
         : QDeclarativeImageProvider(QDeclarativeImageProvider::Pixmap)
     {
     }

     QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
     {
         int width = 180;
         int height = 180;

         QString path;
         if( !sRomFile.isEmpty()){
             QFileInfo fi(sRomFile);
             path = sSavePath;
             path.append("/");
             path += fi.fileName();
             path.append(".");
             QString temp;
             path.append(temp.setNum(id.toInt()));
             path.append(".jpg");
         }
         if (size)
             *size = QSize(width, height);
         QPixmap pix(*size);
         if( !pix.load(path)){
             // write no save loaded
             pix.fill();
             QPainter painter(&pix);
             QFont f = painter.font();
             f.setPixelSize(20);
             painter.setFont(f);
             painter.setPen(Qt::black);
             painter.drawText(QRectF(0, 0, width, height), Qt::AlignCenter, "No Save Image");
             painter.end();
         }
         return pix;
     }
 };
