#ifndef CLOUDDRIVEMODELTHREAD_H
#define CLOUDDRIVEMODELTHREAD_H

#include <QThread>
#include <QMultiMap>
#include <QFile>
#include <QTime>
#include "clouddriveitem.h"

class CloudDriveModelThread : public QThread
{
    Q_OBJECT
public:
    explicit CloudDriveModelThread(QObject *parent = 0);
    
    enum RunnableMethods {
        LoadCloudDriveItems
    };

    QString getHashFilePath() const;
    void setHashFilePath(const QString hashFilePath);
    void setNonce(QString nonce);
    void setRunMethod(int method);
    void setCloudDriveItems(QMultiMap<QString, CloudDriveItem> *cloudDriveItems);

    void run();
signals:
    void loadCloudDriveItemsFinished(QString nonce);
public slots:

private:
    QString m_hashFilePath;
    QMultiMap<QString, CloudDriveItem> *m_cloudDriveItems;

    int m_runMethod;
    QString m_nonce;

    // Thread runnableMethod.
    void loadCloudDriveItems();
};

#endif // CLOUDDRIVEMODELTHREAD_H
