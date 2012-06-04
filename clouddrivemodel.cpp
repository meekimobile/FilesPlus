#include "clouddrivemodel.h"
#include <QFile>

const QString CloudDriveModel::HashFilePath = "C:/CloudDriveModel.dat";

CloudDriveModel::CloudDriveModel(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    loadCloudDriveItems();
}

CloudDriveModel::~CloudDriveModel()
{
    saveCloudDriveItems();
}

void CloudDriveModel::loadCloudDriveItems() {
    QFile file(HashFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> m_cloudDriveItems;

        qDebug() << "CloudDriveModel::loadCloudDriveItems " << m_cloudDriveItems;
    }
}

void CloudDriveModel::saveCloudDriveItems() {
    QFile file(HashFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << m_cloudDriveItems;
    }
}

CloudDriveItem CloudDriveModel::getItem(QString localPath, ClientTypes type, QString uid)
{
//    qDebug() << "CloudDriveModel::getItem " << localPath << ", " << type << ", " << uid;
    CloudDriveItem item;
    foreach (item, getItemList(localPath)) {
        if (item.type == type && item.uid == uid) {
//            qDebug() << "CloudDriveModel::getItem " << item;
            return item;
        }
    }

    return CloudDriveItem();
}

QList<CloudDriveItem> CloudDriveModel::getItemList(QString localPath) {
    return m_cloudDriveItems.values(localPath);
}

bool CloudDriveModel::isConnected(QString localPath)
{
    return m_cloudDriveItems.contains(localPath);
}

void CloudDriveModel::addItem(QString localPath, CloudDriveItem item)
{
    m_cloudDriveItems.insert(localPath, item);

    qDebug() << "CloudDriveModel::addItem m_cloudDriveItems " << m_cloudDriveItems;
}

void CloudDriveModel::addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    if (item.localPath != "") {
        qDebug() << "CloudDriveModel::addItem remove " << item;
        removeItem(type, uid, localPath);
    }
    item = CloudDriveItem(type, uid, localPath, remotePath, hash);
    addItem(localPath, item);
}

void CloudDriveModel::removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudDriveItem item =  getItem(localPath, type, uid);
    m_cloudDriveItems.remove(localPath, item);
}

QString CloudDriveModel::getItemListJson(QString localPath)
{
    QList<CloudDriveItem> list = getItemList(localPath);
    qDebug() << "CloudDriveModel::getItemListJson getItemList " << list;

    QString jsonText;
    foreach (CloudDriveItem item, list) {
        if (jsonText != "") jsonText.append(", ");
        jsonText.append( QString("{ \"type\": \"%1\", \"uid\": \"%2\", \"hash\": \"%3\" }").arg(item.type).arg(item.uid).arg(item.hash) );
    }

    return "[ " + jsonText + " ]";
}

QString CloudDriveModel::getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    return item.remotePath;
}

QString CloudDriveModel::getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    return item.hash;
}
