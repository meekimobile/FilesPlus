#include "clouditem.h"

CloudItem::CloudItem()
{
}

CloudItem::CloudItem(int type, QString uid, QString localPath, QString remotePath, QString hash)
{
    this->type = type;
    this->uid = uid;
    this->localPath = localPath;
    this->remotePath = remotePath;
    this->hash = hash;
}

bool CloudItem::operator==(const CloudItem &item)
{
    return (type == item.type && uid == item.uid && localPath == item.localPath);
}

QDataStream &operator<<(QDataStream &out, const CloudItem &item)
{
        out << item.type << item.uid << item.localPath << item.remotePath << item.hash;

        return out;
}

QDataStream &operator>>(QDataStream &in, CloudItem &item)
{
    int type;
    QString uid;
    QString localPath;
    QString remotePath;
    QString hash;

    in >> type >> uid >> localPath >> remotePath >> hash;
    item = CloudItem(type, uid, localPath, remotePath, hash);

    return in;
}

QDebug &operator<<(QDebug &out, const CloudItem &item)
{
    out << "CloudItem(" << item.type << "," << item.uid << "," << item.localPath << "," << item.remotePath << "," << item.hash << ")";

    return out;
}
