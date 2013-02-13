#include "remoteimageprovider.h"
#include <QDir>
#include <QDebug>
#include <QImageReader>
#include <QApplication>
#include <QCryptographicHash>
#include "sleeper.h"

RemoteImageProvider::RemoteImageProvider(QString cachePath) : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
    m_cachePath = cachePath;
}

QString RemoteImageProvider::getCachedPath(const QString &id, const QSize &requestedSize)
{
    QUrl url(id);
    QByteArray hash = QCryptographicHash::hash(url.host().append(url.path()).toUtf8(), QCryptographicHash::Md5).toHex();
    QString cachedImagePath = QString("%1/%2_%3x%4.png").arg(m_cachePath).arg(QString(hash)).arg(requestedSize.width()).arg(requestedSize.height());

    return cachedImagePath;
}

QImage RemoteImageProvider::getCachedImage(const QString &id, const QSize &requestedSize)
{
    QImage image;
    QFileInfo cachedFileInfo(getCachedPath(id, requestedSize));
    // Create directory path if it's not exist.
    if (!cachedFileInfo.dir().exists()) {
        cachedFileInfo.dir().mkpath(cachedFileInfo.absolutePath());
    }

    QNetworkAccessManager *qnam = new QNetworkAccessManager();
    QNetworkRequest req(QUrl::fromEncoded(id.toAscii())); // id is encoded url.
    QNetworkReply *reply = qnam->get(req);
    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "RemoteImageProvider::requestImage reply->bytesAvailable()" << reply->bytesAvailable();
        QImageReader ir(reply, QImageReader::imageFormat(reply));
        image = ir.read();
        if (ir.error() == 0) {
            if (image.save(cachedFileInfo.absoluteFilePath())) {
                qDebug() << "RemoteImageProvider::requestImage save id" << id << "cached image path" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
            } else {
                qDebug() << "RemoteImageProvider::requestImage can't save" << cachedFileInfo.absoluteFilePath();
            }
        } else {
            qDebug() << "RemoteImageProvider::requestImage can't read image error" << ir.error() << ir.errorString();
        }
    } else {
        qDebug() << "RemoteImageProvider::requestImage can't download error" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
    }

    // Clean up.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return image;
}

QImage RemoteImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    qDebug() << "RemoteImageProvider::requestImage request id" << id << "size" << size << "requestedSize" << requestedSize;

    if (id == "") {
        qDebug() << "RemoteImageProvider::requestImage id is empty.";
        return QImage();
    }

    if (requestedSize.width() <= 0 || requestedSize.height() <= 0) {
        qDebug() << "RemoteImageProvider::requestImage requestSize is invalid." << requestedSize;
        return QImage();
    }

    // Check if cached image is available.
    // 86400 secs = 1 day
    QImage image;
    QFileInfo cachedFileInfo(getCachedPath(id, requestedSize));
    if (cachedFileInfo.exists()
            && cachedFileInfo.created().secsTo(QDateTime::currentDateTime()) < m_settings.value("image.cache.retention.seconds", QVariant(86400)).toInt()) {
        qDebug() << "RemoteImageProvider::requestImage cache exists" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
        QImageReader ir(cachedFileInfo.absoluteFilePath());
        image = ir.read();
        if (ir.error() == 0) {
            qDebug() << "RemoteImageProvider::requestImage return cached image" << cachedFileInfo.absoluteFilePath();
            return image;
        } else {
            qDebug() << "RemoteImageProvider::requestImage can't load cached image path" << cachedFileInfo.absoluteFilePath() << "error" << ir.error() << ir.errorString();
        }
    } else {
        qDebug() << "RemoteImageProvider::requestImage not found or expired cache image path" << cachedFileInfo.absoluteFilePath();
    }

    if (m_settings.value("RemoteImageProvider.getCachedImage.enabled", QVariant(false)).toBool()) {
        // Get image from source URL.
        image = getCachedImage(id, requestedSize);
        size->setWidth(image.size().width());
        size->setHeight(image.size().height());
    }

    qDebug() << "RemoteImageProvider::requestImage reply id" << id
             << "size" << size->width() << "," << size->height()
             << "requestedSize" << requestedSize
             << "image size" << image.size();

    return image;
}
