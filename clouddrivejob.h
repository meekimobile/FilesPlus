#ifndef CLOUDDRIVEJOB_H
#define CLOUDDRIVEJOB_H

#include <QDebug>
#include <QDateTime>

class CloudDriveJob
{
public:
    CloudDriveJob();
    CloudDriveJob(QString jobId, int operation, int type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);

    QString jobId;
    int operation;
    int type;
    QString uid;
    QString localFilePath;
    QString remoteFilePath;
    QString newLocalFilePath;
    QString newRemoteFilePath;
    QString newRemoteFileName;
    int targetType;
    QString targetUid;
    bool isRunning;
    int modelIndex;
    qint64 bytes;
    qint64 bytesTotal;

    bool forcePut;
    bool forceGet;

    qint64 downloadOffset;

    QString uploadId;
    qint64 uploadOffset;

    QDateTime createdTime;
    QDateTime lastStartedTime;
    QDateTime lastStoppedTime;
    int err;
    QString errString;
    QString errMessage;

    int retryCount;

    QString nextJobId;

    QString toJsonText();
};

#endif // CLOUDDRIVEJOB_H
