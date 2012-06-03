#ifndef CLOUDDRIVEMODEL_H
#define CLOUDDRIVEMODEL_H

#include <QDeclarativeItem>
#include <QHash>
#include <QMultiHash>
#include <QAbstractListModel>
#include "clouditem.h"

class CloudDriveModel : public QDeclarativeItem
{
    Q_OBJECT
    Q_ENUMS(ClientTypes)
public:
    static const QString HashFilePath;

    enum ClientTypes {
        Dropbox,
        GoogleDrive,
        SkyDrive,
        SugarSync
    };

    explicit CloudDriveModel(QDeclarativeItem *parent = 0);
    ~CloudDriveModel();

    void addItem(QString localPath, CloudItem item);
    QList<CloudItem> getItemList(QString localPath);
    CloudItem getItem(QString localPath, CloudDriveModel::ClientTypes type, QString uid);

    Q_INVOKABLE bool isConnected(QString localPath);
    Q_INVOKABLE void addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash);
    Q_INVOKABLE void removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE QString getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemListJson(QString localPath);
signals:
    
public slots:
    
private:
    QMultiMap<QString, CloudItem> m_cloudItems;

    void loadCloudItems();
    void saveCloudItems();
};

#endif // CLOUDDRIVEMODEL_H
