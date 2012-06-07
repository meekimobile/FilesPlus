#ifndef LOCALFILEIMAGEPROVIDER_H
#define LOCALFILEIMAGEPROVIDER_H

#include <QDeclarativeImageProvider>

class LocalFileImageProvider : public QDeclarativeImageProvider
{
public:
    LocalFileImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    const char *getFileFormat(const QString &fileName);
};

#endif // LOCALFILEIMAGEPROVIDER_H
