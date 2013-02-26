#include "cacheimageworker.h"
#include <QCryptographicHash>
#include <QDir>
#include <QtNetwork>
#include <QApplication>
#include "sleeper.h"
#include <QImageReader>

#if defined(Q_WS_HARMATTAN)
const int CacheImageWorker::DEFAULT_CACHE_IMAGE_SIZE = 854;
#else
const int CacheImageWorker::DEFAULT_CACHE_IMAGE_SIZE = 640;
#endif

CacheImageWorker::CacheImageWorker(const QString absoluteFilePath, const QString url, const QSize &requestedSize, const QString cachePath, const QString caller)
{
    m_absoluteFilePath = absoluteFilePath;
    m_url = url;
    m_requestedSize = requestedSize;
    m_cachePath = cachePath;
    m_caller = caller;
}

void CacheImageWorker::run()
{
    if (m_url.startsWith("http")) {
        cacheRemoteImage(m_url, m_requestedSize);
    } else {
        cacheLocalImage(m_url, m_requestedSize);
    }
}

QString CacheImageWorker::getCachedLocalPath(const QString &id, const QSize &requestedSize)
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

QString CacheImageWorker::getCachedRemotePath(const QString &id, const QSize &requestedSize)
{
    QUrl url(id);
    QByteArray hash = QCryptographicHash::hash(url.host().append(url.path()).toUtf8(), QCryptographicHash::Md5).toHex();
    QString cachedImagePath;
    if (requestedSize.isValid()) {
        cachedImagePath = QString("%1/%2_%3x%4.png").arg(m_cachePath).arg(QString(hash)).arg(requestedSize.width()).arg(requestedSize.height());
    } else {
        cachedImagePath = QString("%1/%2.png").arg(m_cachePath).arg(QString(hash));
    }

    return cachedImagePath;
}

void CacheImageWorker::cacheRemoteImage(const QString &url, const QSize &requestedSize)
{
    qDebug() << "CacheImageWorker::cacheRemoteImage thread" << QThread::currentThread() << "url" << url << "requestedSize" << requestedSize << "caller" << m_caller;

    if (url == "") {
        qDebug() << "CacheImageWorker::cacheRemoteImage url is empty.";
        emit cacheImageFinished(m_absoluteFilePath, -1, "Url is empty.", m_caller);
        return;
    }

    // Check if cached image is available.
    // 86400 secs = 1 day
    QFileInfo cachedFileInfo(getCachedRemotePath(url, requestedSize));
    // Create directory path if it's not exist.
    if (!cachedFileInfo.dir().exists()) {
        cachedFileInfo.dir().mkpath(cachedFileInfo.absolutePath());
    }
    if (cachedFileInfo.exists()
            && cachedFileInfo.created().secsTo(QDateTime::currentDateTime()) < m_settings.value("image.cache.retention.seconds", QVariant(86400)).toInt()) {
        qDebug() << "CacheImageWorker::cacheRemoteImage cache exists" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
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
        if (reply->header(QNetworkRequest::LocationHeader).isValid()) {
            QString redirectedUrl = reply->header(QNetworkRequest::LocationHeader).toString();
            qDebug() << "CacheImageWorker::cacheRemoteImage reply redirectedUrl" << redirectedUrl;
            reply = qnam->get(QNetworkRequest(QUrl(redirectedUrl)));
            if (reply->error() != QNetworkReply::NoError) {
                // Handle error.
                qDebug() << "CacheImageWorker::cacheRemoteImage can't download. Error" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
                emit cacheImageFinished(m_absoluteFilePath, reply->error(), reply->errorString(), m_caller);

                // Clean up.
                reply->deleteLater();
                reply->manager()->deleteLater();

                return;
            }
        }

        qDebug() << "CacheImageWorker::cacheRemoteImage reply->bytesAvailable()" << reply->bytesAvailable();
        // Read image according to its format, then save to PNG.
        QImageReader ir(reply, QImageReader::imageFormat(reply));
        qDebug() << "CacheImageWorker::cacheRemoteImage reply size" << ir.size();

        // Set cached image size with KeepAspectRatio.
        if (requestedSize.isValid()) {
            QSize newSize = ir.size();
            newSize.scale(requestedSize, Qt::KeepAspectRatio);
            qDebug() << "CacheImageWorker::cacheRemoteImage scale requestedSize" << requestedSize << "newSize" << newSize;
            ir.setScaledSize(newSize);
        }

        // Read image into requested size.
        QImage image = ir.read();
        if (ir.error() == 0) {
            if (image.save(cachedFileInfo.absoluteFilePath())) {
                cachedFileInfo.refresh();
                qDebug() << "CacheImageWorker::cacheRemoteImage save image to" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
                emit cacheImageFinished(m_absoluteFilePath, 0, "Image was saved.", m_caller);
                emit refreshFolderCacheSignal(cachedFileInfo.absoluteFilePath());
            } else {
                qDebug() << "CacheImageWorker::cacheRemoteImage can't save image to" << cachedFileInfo.absoluteFilePath();
                emit cacheImageFinished(m_absoluteFilePath, -1, "Can't save image.", m_caller);
            }
        } else {
            qDebug() << "CacheImageWorker::cacheRemoteImage can't read downloaded image. Error" << ir.error() << ir.errorString();
            emit cacheImageFinished(m_absoluteFilePath, ir.error(), ir.errorString(), m_caller);
        }
    } else {
        qDebug() << "CacheImageWorker::cacheRemoteImage can't download. Error" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        emit cacheImageFinished(m_absoluteFilePath, reply->error(), reply->errorString(), m_caller);
    }

    // Clean up.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void CacheImageWorker::cacheLocalImage(const QString &filePath, const QSize &requestedSize)
{
    qDebug() << "CacheImageWorker::cacheLocalImage thread" << QThread::currentThread() << "filePath" << filePath << "requestedSize" << requestedSize << "caller" << m_caller;

    if (filePath == "") {
        qDebug() << "CacheImageWorker::cacheLocalImage filePath is empty.";
        emit cacheImageFinished(m_absoluteFilePath, -1, "Url is empty.", m_caller);
        return;
    }

    // Check if filePath is cached image path.
    QImage image;
    if (filePath.startsWith(m_cachePath)) {
        qDebug() << "CacheImageWorker::cacheLocalImage filePath" << filePath << "is cached image.";
        emit cacheImageFinished(m_absoluteFilePath, -1, "FilePath is cached image path.", m_caller);
        return;
    }

    // Check if cached image is available.
    // 86400 secs = 1 day
    QFileInfo cachedFileInfo(getCachedLocalPath(filePath, requestedSize));
    // Create directory path if it's not exist.
    if (!cachedFileInfo.dir().exists()) {
        cachedFileInfo.dir().mkpath(cachedFileInfo.absolutePath());
    }
    if (cachedFileInfo.exists()
            && cachedFileInfo.created().secsTo(QDateTime::currentDateTime()) < m_settings.value("image.cache.retention.seconds", QVariant(86400)).toInt()) {
        qDebug() << "CacheImageWorker::cacheLocalImage cache exists" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
        return;
    }

    QImageReader ir(filePath);
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
            qDebug() << "CacheImageWorker::cacheLocalImage scale requestedSize" << requestedSize << "newSize" << newSize;
            ir.setScaledSize(newSize);
        }

        // Read into image.
        image = ir.read();
        if (ir.error() == 0) {
            if (image.save(cachedFileInfo.absoluteFilePath())) {
                cachedFileInfo.refresh();
                qDebug() << "CacheImageWorker::cacheLocalImage save image to" << cachedFileInfo.absoluteFilePath() << "size" << cachedFileInfo.size();
                emit cacheImageFinished(m_absoluteFilePath, 0, "Image was saved.", m_caller);
                emit refreshFolderCacheSignal(cachedFileInfo.absoluteFilePath());
            } else {
                qDebug() << "CacheImageWorker::cacheLocalImage can't save image to" << cachedFileInfo.absoluteFilePath();
                emit cacheImageFinished(m_absoluteFilePath, -1, "Can't save image.", m_caller);
            }
        } else {
            qDebug() << "CacheImageWorker::cacheLocalImage can't read image. Error" << ir.error() << ir.errorString();
            emit cacheImageFinished(m_absoluteFilePath, ir.error(), ir.errorString(), m_caller);
        }
    } else {
        qDebug() << "CacheImageWorker::cacheLocalImage can't read from file. Invalid file content.";
        emit cacheImageFinished(m_absoluteFilePath, -1, "Can't read from file. Invalid file content.", m_caller);
    }
}
