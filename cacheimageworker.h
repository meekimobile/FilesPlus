#ifndef CACHEIMAGEWORKER_H
#define CACHEIMAGEWORKER_H

#include <QRunnable>
#include <QSize>
#include <QSettings>

class CacheImageWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    CacheImageWorker(const QString remoteFilePath, const QString url, const QSize &requestedSize, const QString cachePath, const QString caller);

    void run();
signals:
    void cacheImageFinished(QString remoteFilePath, int err, QString errMsg, QString caller);
private:
    QString m_remoteFilePath;
    QString m_url;
    QSize m_requestedSize;
    QString m_cachePath;
    QString m_caller;
    QSettings m_settings;

    QString getCachedPath(const QString &id, const QSize &requestedSize);
    void cacheImage(const QString &url, const QSize &requestedSize);
};

#endif // CACHEIMAGEWORKER_H
