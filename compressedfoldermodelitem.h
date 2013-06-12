#ifndef COMPRESSEDFOLDERMODELITEM_H
#define COMPRESSEDFOLDERMODELITEM_H

#include <QString>
#include <QDateTime>

class CompressedFolderModelItem
{
public:
    CompressedFolderModelItem();

    QString name;
    quint32 size;
    quint32 compressedSize;
    QDateTime lastModified;
    bool isDir;
    bool isChecked;
};

#endif // COMPRESSEDFOLDERMODELITEM_H
