#ifndef REMOTEIMAGEPROVIDER_H
#define REMOTEIMAGEPROVIDER_H

#include <QDeclarativeImageProvider>
#include <QtCore>
#include <QtNetwork>

class RemoteImageProvider : public QDeclarativeImageProvider
{
public:
    RemoteImageProvider(QString cachePath);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QString getCachedPath(const QString &id, const QSize &requestedSize);
private:
    QString m_cachePath;
    QSettings m_settings;
};

#endif // REMOTEIMAGEPROVIDER_H
