#ifndef LOCALFILEIMAGEPROVIDER_H
#define LOCALFILEIMAGEPROVIDER_H

#include <QDeclarativeImageProvider>
#include <QtCore>

class LocalFileImageProvider : public QDeclarativeImageProvider
{
public:
    LocalFileImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QString getFileFormat(const QString &fileName);
private:
};

#endif // LOCALFILEIMAGEPROVIDER_H
