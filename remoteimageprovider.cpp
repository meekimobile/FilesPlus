#include "remoteimageprovider.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QImageReader>
#include <QtNetwork>
#include <QApplication>
#include <QCryptographicHash>
#include "sleeper.h"

RemoteImageProvider::RemoteImageProvider(QString cachePath) : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
    m_cachePath = cachePath;
}

QString RemoteImageProvider::getFileFormat(const QString &fileName) {
    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,4})$");
    rx.indexIn(fileName);
//    for(int i=0; i<=rx.captureCount(); i++) {
//        qDebug() << "i=" << i << " rx.cap=" << rx.cap(i);
//    }

    QString format = rx.cap(3).toUpper();
    return format;
}

QString RemoteImageProvider::getCachedPath(const QString &id, const QSize &requestedSize)
{
    QUrl url(id);
    QByteArray hash = QCryptographicHash::hash(url.host().append(url.path()).toUtf8(), QCryptographicHash::Md5).toHex();
    QString cachedImagePath = QString("%1/%2_%3x%4.png").arg(m_cachePath).arg(QString(hash)).arg(requestedSize.width()).arg(requestedSize.height());

    return cachedImagePath;
}

QImage RemoteImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    qDebug() << "RemoteImageProvider::requestImage request id " << id
             << " size " << size->width() << "," << size->height()
             << " requestedSize " << requestedSize;

    if (id == "") {
        qDebug() << "RemoteImageProvider::requestImage id is empty.";
        return QImage();
    }

    if (requestedSize.width() <= 0 || requestedSize.height() <= 0) {
        qDebug() << "RemoteImageProvider::requestImage requestSize is invalid. " << requestedSize;
        return QImage();
    }

    // Check if cached image is available.
    QImage image;
    QFileInfo cachedFileInfo(getCachedPath(id, requestedSize));
    if (cachedFileInfo.exists()
            && cachedFileInfo.created().secsTo(QDateTime::currentDateTime()) < m_setting.value("RemoteImageProvider.cache.retention.seconds", QVariant(86400)).toInt() // 86400 secs = 1 day
            && image.load(cachedFileInfo.absoluteFilePath())) {
        qDebug() << "RemoteImageProvider::requestImage return cached id" << id << "cached image path" << cachedFileInfo.absoluteFilePath();
        return image;
    }

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(id.toAscii())); // id is encoded url.
    QNetworkReply *reply = manager->get(req);
    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (reply->error() == QNetworkReply::NoError) {
        QImageReader ir(reply);

        // Calculate new thumbnail size with KeepAspectRatio.
        if (ir.size().width() > requestedSize.width() || ir.size().height() > requestedSize.height()) {
            QSize newSize = ir.size();
            newSize.scale(requestedSize, Qt::KeepAspectRatio);
            qDebug() << "RemoteImageProvider::requestImage scale requestedSize" << requestedSize << "newSize" << newSize;
            ir.setScaledSize(newSize);
        }

        image = ir.read();
        if (ir.error() != 0) {
            qDebug() << "RemoteImageProvider::requestImage can't read image error" << ir.error() << ir.errorString();
        } else {
            qDebug() << "RemoteImageProvider::requestImage read image format" << image.format() << "size" << image.size();

            // Return scaled size as requested.
            size->setWidth(image.size().width());
            size->setHeight(image.size().height());

            if (image.save(cachedFileInfo.absoluteFilePath())) {
                qDebug() << "RemoteImageProvider::requestImage save id" << id << "cached image path" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
            } else {
                qDebug() << "RemoteImageProvider::requestImage can't save" << cachedFileInfo.absoluteFilePath();
            }
        }
    } else {
        qDebug() << "RemoteImageProvider::requestImage can't download error" << reply->error() << QString::fromUtf8(reply->readAll());
    }

    qDebug() << "RemoteImageProvider::requestImage reply id" << id
             << " size " << size->width() << "," << size->height()
             << " requestedSize " << requestedSize
             << " image size " << image.size();

    reply->deleteLater();
    reply->manager()->deleteLater();

    return image;
}
