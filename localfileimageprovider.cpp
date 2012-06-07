#include "localfileimageprovider.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QImageReader>

LocalFileImageProvider::LocalFileImageProvider() : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
}

const char * LocalFileImageProvider::getFileFormat(const QString &fileName) {
    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,4})$");
    rx.indexIn(fileName);
    for(int i=0; i<=rx.captureCount(); i++) {
        qDebug() << "i=" << i << " rx.cap=" << rx.cap(i);
    }

    const char * format = rx.cap(3).toLower().toStdString().c_str();
    return format;
}

QImage LocalFileImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    //    QFile imageFile(id);
    //    if (imageFile.open(QIODevice::ReadOnly)) {
    //    }
    //    imageFile.close();

    // TODO it works with some image. Ex. C:/fierce_bear.jpg
    // TODO To be figured out.
    QImage image;
    if (id == "") {
        QImageReader ir(id);
        image = ir.read();
        qDebug() << "read err " << ir.error() << " " << ir.errorString();
    }
    return image;
}
