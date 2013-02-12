#ifndef REMOTEIMAGEPROVIDER_H
#define REMOTEIMAGEPROVIDER_H

#include <QDeclarativeImageProvider>
#include <QtCore>

class RemoteImageProvider : public QDeclarativeImageProvider
{
public:
    RemoteImageProvider(QString cachePath);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QString getFileFormat(const QString &fileName);
    QString getCachedPath(const QString &id, const QSize &requestedSize);
private:
    QString m_cachePath;
    QSettings m_setting;
};

#endif // REMOTEIMAGEPROVIDER_H
