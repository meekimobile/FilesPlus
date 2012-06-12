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
    
    QString getHashFilePath() const;
    void setHashFilePath(const QString hashFilePath);

    QMultiMap<QString, CloudDriveItem> getCloudDriveItems();

    void run();
signals:
    void dataLoadedSignal();
public slots:

private:
    QString m_hashFilePath;
    QMultiMap<QString, CloudDriveItem> m_cloudDriveItems;

    void loadCloudDriveItems();
};

#endif // CLOUDDRIVEMODELTHREAD_H
