#include "localfileimageprovider.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QImageReader>

//const int LocalFileImageProvider::MAX_CACHE_COST = 10;
//const QString LocalFileImageProvider::DEFAULT_CACHE_PATH = "LocalFileImageProvider";

LocalFileImageProvider::LocalFileImageProvider() : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
}

QString LocalFileImageProvider::getFileFormat(const QString &fileName) {
    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,4})$");
    rx.indexIn(fileName);
//    for(int i=0; i<=rx.captureCount(); i++) {
//        qDebug() << "i=" << i << " rx.cap=" << rx.cap(i);
//    }

    QString format = rx.cap(3).toUpper();
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

    if (requestedSize.width() <= 0 || requestedSize.height() <= 0) {
        qDebug() << "LocalFileImageProvider::requestImage requestSize is invalid. " << requestedSize;
        return QImage();
    }

    QFile file(id);
    QByteArray format = getFileFormat(id).toLatin1();
    QImage image;
    QImageReader ir(&file, format);
    ir.setAutoDetectImageFormat(false);

    if (ir.canRead()) {
        // Calculate new thumbnail size with KeepAspectRatio.
        if (ir.size().width() > requestedSize.width() || ir.size().height() > requestedSize.height()) {
            QSize newSize = ir.size();
            newSize.scale(requestedSize, Qt::KeepAspectRatio);
            qDebug() << "LocalFileImageProvider::requestImage scale requestedSize" << requestedSize << "newSize" << newSize;
            ir.setScaledSize(newSize);
        }

        // Read into image.
        image = ir.read();

        // Return scaled size as requested.
        size->setWidth(image.size().width());
        size->setHeight(image.size().height());

        if (ir.error() != 0) {
            qDebug() << "LocalFileImageProvider::requestImage read err " << ir.error() << " " << ir.errorString();
        }
    } else {
        qDebug() << "LocalFileImageProvider::requestImage can't read from file. Invalid file content.";
    }

    qDebug() << "LocalFileImageProvider::requestImage reply id " << id
             << " size " << size->width() << "," << size->height()
             << " requestedSize " << requestedSize
             << " image size " << image.size();
    return image;
}
