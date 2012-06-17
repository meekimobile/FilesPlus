#ifndef FOLDERSIZEITEM_H
#define FOLDERSIZEITEM_H

#include <QDateTime>
#include <QDataStream>

class FolderSizeItem
{
public:
    FolderSizeItem();
    FolderSizeItem(const QString &name, const QString &absolutePath, const QDateTime &lastModified, const qint64 &size, const bool isDir, const qint64 &subDirCount, const qint64 &subFileCount);

    QString name;
    QString absolutePath;
    qint64 size;
    QDateTime lastModified;
    bool isDir;
    qint64 subDirCount;
    qint64 subFileCount;

    QString fileType;

    bool isRunning;
    qint64 runningValue;
    qint64 runningMaxValue;

    bool isChecked;

    QString toJsonText();
private:
    void setFileType();
};

QDataStream &operator<<(QDataStream &out, const FolderSizeItem &item);
QDataStream &operator>>(QDataStream &in, FolderSizeItem &item);

#endif // FOLDERSIZEITEM_H
