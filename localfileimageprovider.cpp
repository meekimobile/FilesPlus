#include "localfileimageprovider.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QImageReader>

//const int LocalFileImageProvider::MAX_CACHE_COST = 10;
//const QString LocalFileImageProvider::DEFAULT_CACHE_PATH = "C:/LocalFileImageProvider";

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
    // No cache. Let QML(UI) grid manage image cache.
    qDebug() << "LocalFileImageProvider::requestImage request id " << id
             << " size " << size->width() << "," << size->height()
             << " requestedSize " << requestedSize;

    if (id == "") {
        qDebug() << "LocalFileImageProvider::requestImage id is empty.";
        return QImage();
    }

    QImageReader ir(id);
    QImage image = ir.read();
    if (ir.error() != 0) {
        qDebug() << "LocalFileImageProvider::requestImage read err " << ir.error() << " " << ir.errorString();
    } else {
        // Return actual size.
        size->setWidth(image.size().width());
        size->setHeight(image.size().height());

        // Create thumbnail and return.
        if (size->width() > requestedSize.width() || size->height() > requestedSize.height()) {
            image = image.scaled(requestedSize, Qt::KeepAspectRatio);
        }
    }

    qDebug() << "LocalFileImageProvider::requestImage reply id " << id
             << " size " << size->width() << "," << size->height()
             << " requestedSize " << requestedSize
             << " image size " << image.size();
    return image;
}
