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

QString CloudDriveItem::toJsonText()
{
    QString jsonText = "{ ";
    jsonText.append( QString("\"type\": \"%1\", ").arg(type) );
    jsonText.append( QString("\"uid\": \"%1\", ").arg(uid) );
    jsonText.append( QString("\"local_path\": \"%1\", ").arg(localPath) );
    jsonText.append( QString("\"remote_path\": \"%1\", ").arg(remotePath) );
    jsonText.append( QString("\"hash\": \"%1\"").arg(hash) );
    jsonText.append(" }");

    return jsonText;
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
