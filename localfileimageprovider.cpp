#include "localfileimageprovider.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QImageReader>

//const int LocalFileImageProvider::MAX_CACHE_COST = 10;
//const QString LocalFileImageProvider::DEFAULT_CACHE_PATH = "LocalFileImageProvider";
#if defined(Q_WS_HARMATTAN)
const int LocalFileImageProvider::DEFAULT_CACHE_IMAGE_SIZE = 480;
#else
const int LocalFileImageProvider::DEFAULT_CACHE_IMAGE_SIZE = 360;
#endif

LocalFileImageProvider::LocalFileImageProvider(QString cachePath) : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
    m_cachePath = cachePath;
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

QString LocalFileImageProvider::getCachedPath(const QString &id, const QSize &requestedSize)
{
    QFileInfo fi(id);
    QString cachedImagePath;
    if (requestedSize.isValid()) {
        cachedImagePath = QString("%1/%2_%3_%4x%5.png").arg(m_cachePath).arg(fi.baseName()).arg(fi.completeSuffix()).arg(requestedSize.width()).arg(requestedSize.height());
    } else {
        cachedImagePath = QString("%1/%2_%3.png").arg(m_cachePath).arg(fi.baseName()).arg(fi.completeSuffix());
    }

    return cachedImagePath;
}

QImage LocalFileImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    qDebug() << "LocalFileImageProvider::requestImage request id " << id
             << " size " << size->width() << "," << size->height()
             << " requestedSize " << requestedSize;

    if (id == "") {
        qDebug() << "LocalFileImageProvider::requestImage id is empty.";
        return QImage();
    }

//    if (requestedSize.width() <= 0 || requestedSize.height() <= 0) {
//        qDebug() << "LocalFileImageProvider::requestImage requestSize is invalid. " << requestedSize;
//        return QImage();
//    }

    // Remove timestamp suffix from id.
    QString absoluteFilePath = id.mid(0, id.lastIndexOf("#t="));

    // Check if id is cached image path.
    QImage image;
    if (absoluteFilePath.startsWith(m_cachePath)) {
        image.load(absoluteFilePath);
        qDebug() << "LocalFileImageProvider::requestImage return id" << id << "It's cached image.";
        return image;
    }

    // Check if cached image is available.
    QFileInfo cachedFileInfo(getCachedPath(absoluteFilePath, requestedSize));
    if (cachedFileInfo.exists()
            && cachedFileInfo.created().secsTo(QDateTime::currentDateTime()) < m_settings.value("image.cache.retention.seconds", QVariant(86400)).toInt() // 86400 secs = 1 day
            && image.load(cachedFileInfo.absoluteFilePath())) {
        qDebug() << "LocalFileImageProvider::requestImage return cached id" << id << "cached image path" << cachedFileInfo.absoluteFilePath();
        return image;
//    } else if (!requestedSize.isValid()) {
//        qDebug() << "LocalFileImageProvider::requestImage cache not found" << cachedFileInfo.absoluteFilePath() << "and requestSize is invalid. Response error to trigger cacheImageWorker.";
//        return QImage();
    }

    QFile file(absoluteFilePath);
    QByteArray format = getFileFormat(absoluteFilePath).toLatin1();
    QImageReader ir(&file, format);
    ir.setAutoDetectImageFormat(false);

    if (ir.canRead()) {
        // Calculate new thumbnail size with KeepAspectRatio.
        QSize defaultSize(DEFAULT_CACHE_IMAGE_SIZE, DEFAULT_CACHE_IMAGE_SIZE);
        if (!requestedSize.isValid() && (ir.size().width() > defaultSize.width() || ir.size().height() > defaultSize.height())) {
            QSize newSize = ir.size();
            newSize.scale(defaultSize, Qt::KeepAspectRatio);
            qDebug() << "LocalFileImageProvider::requestImage scale defaultSize" << defaultSize << "newSize" << newSize;
            ir.setScaledSize(newSize);
        } else if (ir.size().width() > requestedSize.width() || ir.size().height() > requestedSize.height()) {
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
        } else {
            // Save cached image.
            QDir cachePath(m_cachePath);
            if (!cachePath.exists()) {
                cachePath.mkpath(m_cachePath);
            }
            image.save(cachedFileInfo.absoluteFilePath());
            qDebug() << "LocalFileImageProvider::requestImage save id" << id << cachedFileInfo.absoluteFilePath();
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
