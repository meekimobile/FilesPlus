#include "cacheimagehelper.h"

#if defined(Q_WS_HARMATTAN)
const QString CacheImageHelper::IMAGE_CACHE_PATH = "/home/user/MyDocs/temp/.filesplus";
#else
const QString CacheImageHelper::IMAGE_CACHE_PATH = "E:/temp/.filesplus";
#endif

CacheImageHelper::CacheImageHelper(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
#ifdef Q_OS_SYMBIAN
    m_cacheImageThreadPool.setMaxThreadCount(3);
#else
    m_cacheImageThreadPool.setMaxThreadCount(3);
#endif
}

void CacheImageHelper::cacheImage(QString remoteFilePath, QString url, int w, int h, QString caller)
{
    CacheImageWorker *worker = new CacheImageWorker(remoteFilePath, url, QSize(w,h), m_settings.value("image.cache.path", IMAGE_CACHE_PATH).toString(), caller);
    connect(worker, SIGNAL(cacheImageFinished(QString,int,QString,QString)), SLOT(cacheImageFinishedFilter(QString,int,QString,QString)));
    connect(worker, SIGNAL(refreshFolderCacheSignal(QString)), SIGNAL(refreshFolderCacheSignal(QString)));
    m_cacheImageThreadPool.start(worker);
    qDebug() << "CacheImageHelper::cacheImage m_cacheImageThreadPool" << m_cacheImageThreadPool.activeThreadCount() << "/" << m_cacheImageThreadPool.maxThreadCount();
}

void CacheImageHelper::cacheImageFinishedFilter(QString absoluteFilePath, int err, QString errMsg, QString caller)
{
    qDebug() << "CacheImageHelper::cacheImageFinishedFilter started. m_cacheImageThreadPool" << m_cacheImageThreadPool.activeThreadCount() << "/" << m_cacheImageThreadPool.maxThreadCount();

#ifdef Q_OS_SYMBIAN
    // Remove all threads from pool once there is no active thread.
    // NOTE To prevent thread panicking on Symbian. (#FP20130153)
    if (m_cacheImageThreadPool.activeThreadCount() <= 0) {
        m_cacheImageThreadPool.waitForDone();
        qDebug() << "CacheImageHelper::cacheImageFinishedFilter waitForDone() is invoked. m_cacheImageThreadPool" << m_cacheImageThreadPool.activeThreadCount() << "/" << m_cacheImageThreadPool.maxThreadCount();
    }
#endif

    emit cacheImageFinished(absoluteFilePath, err, errMsg, caller);
}
