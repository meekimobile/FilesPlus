#ifndef CACHEIMAGEWORKER_H
#define CACHEIMAGEWORKER_H

#include <QRunnable>
#include <QSize>
#include <QSettings>

class CacheImageWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    static const int DEFAULT_CACHE_IMAGE_SIZE;

    CacheImageWorker(const QString absoluteFilePath, const QString url, const QSize &requestedSize, const QString cachePath, const QString caller);

    void run();
signals:
    void cacheImageFinished(QString absoluteFilePath, int err, QString errMsg, QString caller);
private:
    QString m_absoluteFilePath;
    QString m_url;
    QSize m_requestedSize;
    QString m_cachePath;
    QString m_caller;
    QSettings m_settings;

    QString getCachedRemotePath(const QString &id, const QSize &requestedSize);
    void cacheRemoteImage(const QString &url, const QSize &requestedSize);
    QString getCachedLocalPath(const QString &id, const QSize &requestedSize);
    void cacheLocalImage(const QString &filePath, const QSize &requestedSize);
};

#endif // CACHEIMAGEWORKER_H
