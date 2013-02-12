#ifndef LOCALFILEIMAGEPROVIDER_H
#define LOCALFILEIMAGEPROVIDER_H

#include <QDeclarativeImageProvider>
#include <QtCore>

class LocalFileImageProvider : public QDeclarativeImageProvider
{
public:
    LocalFileImageProvider(QString cachePath);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QString getFileFormat(const QString &fileName);
    QString getCachedPath(const QString &id, const QSize &requestedSize);
private:
    QString m_cachePath;
    QSettings m_settings;
};

#endif // LOCALFILEIMAGEPROVIDER_H
