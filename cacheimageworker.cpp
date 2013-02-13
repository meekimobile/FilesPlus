#include "cacheimageworker.h"
#include <QCryptographicHash>
#include <QDir>
#include <QtNetwork>
#include <QApplication>
#include "sleeper.h"
#include <QImageReader>

CacheImageWorker::CacheImageWorker(const QString remoteFilePath, const QString url, const QSize &requestedSize, const QString cachePath)
{
    m_remoteFilePath = remoteFilePath;
    m_url = url;
    m_requestedSize = requestedSize;
    m_cachePath = cachePath;
}

void CacheImageWorker::run()
{
    cacheImage(m_url, m_requestedSize);
}

QString CacheImageWorker::getCachedPath(const QString &id, const QSize &requestedSize)
{
    QUrl url(id);
    QByteArray hash = QCryptographicHash::hash(url.host().append(url.path()).toUtf8(), QCryptographicHash::Md5).toHex();
    QString cachedImagePath = QString("%1/%2_%3x%4.png").arg(m_cachePath).arg(QString(hash)).arg(requestedSize.width()).arg(requestedSize.height());

    return cachedImagePath;
}

void CacheImageWorker::cacheImage(const QString &url, const QSize &requestedSize)
{
    qDebug() << "CacheImageWorker::cacheImage thread" << QThread::currentThread() << "url" << url << "requestedSize" << requestedSize;

    if (url == "") {
        qDebug() << "CacheImageWorker::cacheImage url is empty.";
        emit cacheImageFinished(m_remoteFilePath, -1, "Url is empty.");
        return;
    }

    // Check if cached image is available.
    // 86400 secs = 1 day
    QFileInfo cachedFileInfo(getCachedPath(url, requestedSize));
    // Create directory path if it's not exist.
    if (!cachedFileInfo.dir().exists()) {
        cachedFileInfo.dir().mkpath(cachedFileInfo.absolutePath());
    }
    if (cachedFileInfo.exists()
            && cachedFileInfo.created().secsTo(QDateTime::currentDateTime()) < m_settings.value("image.cache.retention.seconds", QVariant(86400)).toInt()) {
        qDebug() << "CacheImageWorker::cacheImage cache exists" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
        return;
    }

    QNetworkAccessManager *qnam = new QNetworkAccessManager();
    QNetworkRequest req(QUrl::fromEncoded(url.toAscii()));
    QNetworkReply *reply = qnam->get(req);
    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "CacheImageWorker::cacheImage reply->bytesAvailable()" << reply->bytesAvailable();
        // Read image according to its format, then save to PNG.
        QImageReader ir(reply, QImageReader::imageFormat(reply));
        QImage image = ir.read();
        if (ir.error() == 0) {
            if (image.save(cachedFileInfo.absoluteFilePath())) {
                cachedFileInfo.refresh();
                qDebug() << "CacheImageWorker::cacheImage save image to" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
                emit cacheImageFinished(m_remoteFilePath, 0, "Image was saved.");
            } else {
                qDebug() << "CacheImageWorker::cacheImage can't save image to" << cachedFileInfo.absoluteFilePath();
                emit cacheImageFinished(m_remoteFilePath, -1, "Can't save image.");
            }
        } else {
            qDebug() << "CacheImageWorker::cacheImage can't read downloaded image. Error" << ir.error() << ir.errorString();
            emit cacheImageFinished(m_remoteFilePath, ir.error(), ir.errorString());
        }
    } else {
        qDebug() << "CacheImageWorker::cacheImage can't download. Error" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        emit cacheImageFinished(m_remoteFilePath, reply->error(), reply->errorString());
    }

    // Clean up.
    reply->deleteLater();
    reply->manager()->deleteLater();
}
