#include "localfileimageprovider.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QImageReader>

const int LocalFileImageProvider::MAX_CACHE_COST = 10;

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

    qDebug() << "LocalFileImageProvider::requestImage request id " << id
             << " size " << size->width() << "," << size->height()
             << " requestedSize " << requestedSize;

    if (id == "") {
        qDebug() << "LocalFileImageProvider::requestImage id is empty.";
        return QImage();
    }

    bool isCached;
    QImage image;

    if (m_thumbnailHash.contains(id)) {
        isCached = true;
        image = m_thumbnailHash[id];
    } else {
        isCached = false;
        QImageReader ir(id);
        QImage loadedImage = ir.read();
        if (ir.error() != 0) {
            qDebug() << "LocalFileImageProvider::requestImage read err " << ir.error() << " " << ir.errorString();
        } else {
            size->setWidth(loadedImage.size().width());
            size->setHeight(loadedImage.size().height());

            // Create thumbnail.
            image = loadedImage.scaled(requestedSize, Qt::KeepAspectRatio);
            m_thumbnailHash.insert(id, image);
        }
    }

    qDebug() << "LocalFileImageProvider::requestImage reply id " << id
             << " size " << size->width() << "," << size->height()
             << " requestedSize " << requestedSize
             << " image size " << image.size()
             << " isCached " << isCached;
    return image;
}
