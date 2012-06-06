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

QString CloudDriveJob::toJsonText()
{
    QString jsonText;
    jsonText.append("{ ");
    jsonText.append(QString("\"job_id\": \"%1\", ").arg(jobId));
    jsonText.append(QString("\"is_running\": %1, ").arg( (isRunning)?"true":"false" ));
    jsonText.append(QString("\"uid\": \"%1\", ").arg(uid));
    jsonText.append(QString("\"type\": %1, ").arg(type));
    jsonText.append(QString("\"operation\": %1, ").arg(operation));
    jsonText.append(QString("\"local_file_path\": \"%1\", ").arg(localFilePath));
    jsonText.append(QString("\"remote_file_path\": \"%1\", ").arg(remoteFilePath));
    jsonText.append(QString("\"model_index\": %1, ").arg(modelIndex));
    jsonText.append(QString("\"bytes\": %1, ").arg(bytes));
    jsonText.append(QString("\"bytes_total\": %1 ").arg(bytesTotal));
    jsonText.append(" }");

    return jsonText;
}