#ifndef FOLDERSIZEITEM_H
#define FOLDERSIZEITEM_H

#include <QDateTime>
#include <QDataStream>
#include <QDebug>
#include <QVariant>

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
    int runningOperation;

    bool isChecked;
    bool isDirty;
    bool isHidden;
    bool isReadOnly;

    QString toJsonText();
    QVariant toJson();
private:
    void setFileType();
};

QDataStream &operator<<(QDataStream &out, const FolderSizeItem &item);
QDataStream &operator>>(QDataStream &in, FolderSizeItem &item);
QDebug &operator<<(QDebug &out, const FolderSizeItem &t);

#endif // FOLDERSIZEITEM_H
