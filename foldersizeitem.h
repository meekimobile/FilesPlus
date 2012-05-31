#ifndef FOLDERSIZEITEM_H
#define FOLDERSIZEITEM_H

#include <QDateTime>
#include <QDataStream>

class FolderSizeItem
{
public:
    FolderSizeItem();
    FolderSizeItem(const QString &name, const QString &absolutePath, const QDateTime &lastModified, const double &size, const bool isDir, const double &subDirCount, const double &subFileCount);

    QString name;
    QString absolutePath;
    double size;
    QDateTime lastModified;
    bool isDir;
    double subDirCount;
    double subFileCount;
};

QDataStream &operator<<(QDataStream &out, const FolderSizeItem &item);
QDataStream &operator>>(QDataStream &in, FolderSizeItem &item);

#endif // FOLDERSIZEITEM_H
