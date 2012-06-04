#ifndef CloudDriveItem_H
#define CloudDriveItem_H

#include <QDataStream>
#include <QDebug>

class CloudDriveItem
{
public:
    enum ClientTypes {
        Dropbox,
        GoogleDrive,
        SkyDrive,
        SugarSync
    };

    CloudDriveItem();
    CloudDriveItem(int type, QString uid, QString localPath, QString remotePath, QString hash);

    bool operator==(const CloudDriveItem &item);

    int type;
    QString uid;
    QString localPath;
    QString remotePath;
    QString hash;
};

QDataStream &operator<<(QDataStream &out, const CloudDriveItem &item);
QDataStream &operator>>(QDataStream &in, CloudDriveItem &item);
QDebug &operator<<(QDebug &out, const CloudDriveItem &t);

#endif // CloudDriveItem_H
