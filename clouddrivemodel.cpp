#include "clouddrivemodel.h"
#include <QFile>

const QString CloudDriveModel::HashFilePath = "C:/CloudDriveModel.dat";

CloudDriveModel::CloudDriveModel(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    loadCloudItems();
}

CloudDriveModel::~CloudDriveModel()
{
    saveCloudItems();
}

void CloudDriveModel::loadCloudItems() {
    QFile file(HashFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> m_cloudItems;

        qDebug() << "loadCloudItems " << m_cloudItems;
    }
}

void CloudDriveModel::saveCloudItems() {
    QFile file(HashFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << m_cloudItems;
    }
}

CloudItem CloudDriveModel::getItem(QString localPath, ClientTypes type, QString uid)
{
//    qDebug() << "CloudDriveModel::getItem " << localPath << ", " << type << ", " << uid;
    CloudItem item;
    foreach (item, getItemList(localPath)) {
        if (item.type == type && item.uid == uid) {
//            qDebug() << "CloudDriveModel::getItem " << item;
            return item;
        }
    }

    return CloudItem();
}

QList<CloudItem> CloudDriveModel::getItemList(QString localPath) {
    return m_cloudItems.values(localPath);
}

bool CloudDriveModel::isConnected(QString localPath)
{
    return m_cloudItems.contains(localPath);
}

void CloudDriveModel::addItem(QString localPath, CloudItem item)
{
    m_cloudItems.insert(localPath, item);

    qDebug() << "CloudDriveModel::addItem m_cloudItems " << m_cloudItems;
}

void CloudDriveModel::addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash)
{
    CloudItem item = getItem(localPath, type, uid);
    if (item.localPath != "") {
        qDebug() << "CloudDriveModel::addItem remove " << item;
        removeItem(type, uid, localPath);
    }
    item = CloudItem(type, uid, localPath, remotePath, hash);
    addItem(localPath, item);
}

void CloudDriveModel::removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudItem item =  getItem(localPath, type, uid);
    m_cloudItems.remove(localPath, item);
}

QString CloudDriveModel::getItemListJson(QString localPath)
{
    QList<CloudItem> list = getItemList(localPath);
    qDebug() << "CloudDriveModel::getItemListJson getItemList " << list;

    QString jsonText;
    foreach (CloudItem item, list) {
        if (jsonText != "") jsonText.append(", ");
        jsonText.append( QString("{ \"type\": \"%1\", \"uid\": \"%2\", \"hash\": \"%3\" }").arg(item.type).arg(item.uid).arg(item.hash) );
    }

    return "[ " + jsonText + " ]";
}

QString CloudDriveModel::getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudItem item = getItem(localPath, type, uid);
    return item.remotePath;
}

QString CloudDriveModel::getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudItem item = getItem(localPath, type, uid);
    return item.hash;
}
