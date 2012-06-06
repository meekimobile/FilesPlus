#include "clouddrivejob.h"

CloudDriveJob::CloudDriveJob()
{
}

CloudDriveJob::CloudDriveJob(QString jobId, int operation, int type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex)
{
    this->jobId = jobId;
    this->operation = operation;
    this->type = type;
    this->uid = uid;
    this->localFilePath = localFilePath;
    this->remoteFilePath = remoteFilePath;
    this->isRunning = false;
    this->modelIndex = modelIndex;
}
