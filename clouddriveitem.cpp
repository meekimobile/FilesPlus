#include "clouddriveitem.h"

CloudDriveItem::CloudDriveItem()
{
}

CloudDriveItem::CloudDriveItem(int type, QString uid, QString localPath, QString remotePath, QString hash)
{
    this->type = type;
    this->uid = uid;
    this->localPath = localPath;
    this->remotePath = remotePath;
    this->hash = hash;
}

bool CloudDriveItem::operator==(const CloudDriveItem &item)
{
    return (type == item.type && uid == item.uid && localPath == item.localPath);
}

QDataStream &operator<<(QDataStream &out, const CloudDriveItem &item)
{
        out << item.type << item.uid << item.localPath << item.remotePath << item.hash;

        return out;
}

QDataStream &operator>>(QDataStream &in, CloudDriveItem &item)
{
    int type;
    QString uid;
    QString localPath;
    QString remotePath;
    QString hash;

    in >> type >> uid >> localPath >> remotePath >> hash;
    item = CloudDriveItem(type, uid, localPath, remotePath, hash);

    return in;
}

QDebug &operator<<(QDebug &out, const CloudDriveItem &item)
{
    out << "CloudDriveItem(" << item.type << "," << item.uid << "," << item.localPath << "," << item.remotePath << "," << item.hash << ")";

    return out;
}
