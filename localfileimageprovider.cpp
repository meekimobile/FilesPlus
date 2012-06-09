#include "localfileimageprovider.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QImageReader>

LocalFileImageProvider::LocalFileImageProvider() : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
}

const char * LocalFileImageProvider::getFileFormat(const QString &fileName) {
    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,4})$");
    rx.indexIn(fileName);
    for(int i=0; i<=rx.captureCount(); i++) {
        qDebug() << "i=" << i << " rx.cap=" << rx.cap(i);
    }

    const char * format = rx.cap(3).toLower().toStdString().c_str();
    return format;
}

QImage LocalFileImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    // TODO Implement thumbnail cache.

    qDebug() << "LocalFileImageProvider::requestImage request id " << id << " size " << size->width() << "," << size->height() << " requestedSize " << requestedSize;

    QImage image;
    if (id != "") {
        QImageReader ir(id);
        image = ir.read();
        if (ir.error() != 0) {
            qDebug() << "LocalFileImageProvider::requestImage read err " << ir.error() << " " << ir.errorString();
        } else {
            size->setWidth(image.size().width());
            size->setHeight(image.size().height());
        }
    }

    qDebug() << "LocalFileImageProvider::requestImage reply id " << id << " size " << size->width() << "," << size->height() << " requestedSize " << requestedSize;

    return image;
}
