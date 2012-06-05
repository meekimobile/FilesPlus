#ifndef CLOUDDRIVEJOB_H
#define CLOUDDRIVEJOB_H

#include <QDebug>

class CloudDriveJob
{
public:
    CloudDriveJob();
    CloudDriveJob(QString jobId, int operation, int type, QString uid, QString localFilePath, QString remoteFilePath);

    QString jobId;
    int operation;
    int type;
    QString uid;
    QString localFilePath;
    QString remoteFilePath;
    bool isRunning;
    qint64 bytes;
    qint64 bytesTotal;
};

#endif // CLOUDDRIVEJOB_H
