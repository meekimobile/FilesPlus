#ifndef LOCALFILEIMAGEPROVIDER_H
#define LOCALFILEIMAGEPROVIDER_H

#include <QDeclarativeImageProvider>
#include <QtCore>
#include <QHash>

class LocalFileImageProvider : public QDeclarativeImageProvider
{
public:
    static const int MAX_CACHE_COST;

    LocalFileImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    const char *getFileFormat(const QString &fileName);
private:
    QHash<QString, QImage> m_thumbnailHash;
};

#endif // LOCALFILEIMAGEPROVIDER_H
