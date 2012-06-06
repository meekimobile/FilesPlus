#include "foldersizeitem.h"
#include <QRegExp>

FolderSizeItem::FolderSizeItem()
{
}

FolderSizeItem::FolderSizeItem(const QString &name, const QString &absolutePath, const QDateTime &lastModified, const double &size, const bool isDir, const double &subDirCount, const double &subFileCount)
{
    this->name = name;
    this->absolutePath = absolutePath;
    this->lastModified = lastModified;
    this->size = size;
    this->isDir = isDir;
    this->subDirCount = subDirCount;
    this->subFileCount = subFileCount;

    setFileType();
    this->isRunning = false;
    this->runningValue = 0;
    this->runningMaxValue = 0;
}

void FolderSizeItem::setFileType()
{
    // Get file extension.
    QRegExp rx("^(.+)(\\.)(.+)$");
    rx.indexIn(name);
    if (rx.captureCount() == 3) {
        fileType = rx.cap(3);
    }
}

QDataStream &operator<<(QDataStream &out, const FolderSizeItem &item)
{
    // Save only dir.
    if (item.isDir) {
        out << item.name << item.absolutePath << item.lastModified << item.size << item.subDirCount << item.subFileCount;
    }
    return out;
}

QDataStream &operator>>(QDataStream &in, FolderSizeItem &item)
{
    QString name;
    QString absolutePath;
    QDateTime lastModified;
    double size;
    double subDirCount;
    double subFileCount;

    // Load to dir.
    in >> name >> absolutePath >> lastModified >> size >> subDirCount >> subFileCount;
    item = FolderSizeItem(name, absolutePath, lastModified, size, true, subDirCount, subFileCount);

    return in;
}


