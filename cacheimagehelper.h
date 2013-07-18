#ifndef CACHEIMAGEHELPER_H
#define CACHEIMAGEHELPER_H

#include <QDeclarativeItem>
#include <QThreadPool>
#include "cacheimageworker.h"

class CacheImageHelper : public QDeclarativeItem
{
    Q_OBJECT
public:
    static const QString IMAGE_CACHE_PATH;

    explicit CacheImageHelper(QDeclarativeItem *parent = 0);
    
    Q_INVOKABLE void cacheImage(QString remoteFilePath, QString url, int w, int h, QString caller);
signals:
    void cacheImageFinished(QString absoluteFilePath, int err, QString errMsg, QString caller);

public slots:
    // Cache image.
    void cacheImageFinishedFilter(QString absoluteFilePath, int err, QString errMsg, QString caller);
private:
    QSettings m_settings;
    QThreadPool m_cacheImageThreadPool;
};

#endif // CACHEIMAGEHELPER_H
