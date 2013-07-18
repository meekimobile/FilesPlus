#ifndef CloudDriveItem_H
#define CloudDriveItem_H

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QVariant>

class CloudDriveItem
{
public:
    CloudDriveItem();
    CloudDriveItem(int type, QString uid, QString localPath, QString remotePath, QString hash, QDateTime lastModified);

    bool operator==(const CloudDriveItem &item);

    int type;
    QString uid;
    QString localPath;
    QString remotePath;
    QString hash;
    QDateTime lastModified;
    int syncDirection;

    QString toJsonText();
    QVariant toJson();
    bool isValid();
};

QDataStream &operator<<(QDataStream &out, const CloudDriveItem &item);
QDataStream &operator>>(QDataStream &in, CloudDriveItem &item);
QDebug &operator<<(QDebug &out, const CloudDriveItem &t);

#endif // CloudDriveItem_H
