#include "clouddriveitem.h"

CloudDriveItem::CloudDriveItem()
{
    this->type = -1;
    this->syncDirection = 0;
}

CloudDriveItem::CloudDriveItem(int type, QString uid, QString localPath, QString remotePath, QString hash, QDateTime lastModified)
{
    this->type = type;
    this->uid = uid;
    this->localPath = localPath;
    this->remotePath = remotePath;
    this->hash = hash;
    this->lastModified = lastModified;
    this->syncDirection = 0;
}

bool CloudDriveItem::operator==(const CloudDriveItem &item)
{
    return (type == item.type && uid == item.uid && localPath == item.localPath && lastModified == item.lastModified);
}

QString CloudDriveItem::toJsonText()
{
    QString jsonText = "{ ";
    jsonText.append( QString("\"type\": %1, ").arg(type) );
    jsonText.append( QString("\"uid\": \"%1\", ").arg(uid) );
    jsonText.append( QString("\"localPath\": \"%1\", ").arg(localPath) );
    jsonText.append( QString("\"remotePath\": \"%1\", ").arg(remotePath) );
    jsonText.append( QString("\"hash\": \"%1\", ").arg(hash) );
    jsonText.append( QString("\"lastModified\": \"%1\", ").arg(lastModified.toString(Qt::ISODate)) );
    jsonText.append( QString("\"syncDirection\": %1").arg(syncDirection) );
    jsonText.append(" }");

    return jsonText;
}

QVariant CloudDriveItem::toJson()
{
    QMap<QString,QVariant> jsonObj;
    jsonObj["type"] = QVariant(type);
    jsonObj["uid"] = QVariant(uid);
    jsonObj["localPath"] = QVariant(localPath);
    jsonObj["remotePath"] = QVariant(remotePath);
    jsonObj["hash"] = QVariant(hash);
    jsonObj["lastModified"] = QVariant(lastModified.toString(Qt::ISODate));
    jsonObj["syncDirection"] = QVariant(syncDirection);

    return QVariant(jsonObj);
}

bool CloudDriveItem::isValid()
{
    return (localPath != "" && uid != "");
}

QDataStream &operator<<(QDataStream &out, const CloudDriveItem &item)
{
        out << item.type << item.uid << item.localPath << item.remotePath << item.hash << item.lastModified;

        return out;
}

QDataStream &operator>>(QDataStream &in, CloudDriveItem &item)
{
    int type;
    QString uid;
    QString localPath;
    QString remotePath;
    QString hash;
    QDateTime lastModified;

    in >> type >> uid >> localPath >> remotePath >> hash >> lastModified;
    item = CloudDriveItem(type, uid, localPath, remotePath, hash, lastModified);

    return in;
}

QDebug &operator<<(QDebug &out, const CloudDriveItem &item)
{
    out << "CloudDriveItem(" << item.type << "," << item.uid << "," << item.localPath << "," << item.remotePath << "," << item.hash << "," << item.lastModified << "," << item.syncDirection << ")";

    return out;
}
