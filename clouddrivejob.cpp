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
    this->newLocalFilePath = "";
    this->newRemoteFilePath = "";
    this->newRemoteFileName = "";
    this->isRunning = false;
    this->modelIndex = modelIndex;
    this->bytes = 0;
    this->bytesTotal = 0;
    this->targetType = -1;
    this->createdTime = QDateTime::currentDateTime();
    this->downloadOffset = 0;
    this->uploadOffset = 0;
    this->err = 0;
    this->retryCount = 0;
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
    jsonText.append(QString("\"new_local_file_path\": \"%1\", ").arg(newLocalFilePath));
    jsonText.append(QString("\"new_remote_file_path\": \"%1\", ").arg(newRemoteFilePath));
    jsonText.append(QString("\"new_remote_file_name\": \"%1\", ").arg(newRemoteFileName));
    jsonText.append(QString("\"target_uid\": \"%1\", ").arg(targetUid));
    jsonText.append(QString("\"target_type\": %1, ").arg(targetType));
    jsonText.append(QString("\"model_index\": %1, ").arg(modelIndex));
    jsonText.append(QString("\"bytes\": %1, ").arg(bytes));
    jsonText.append(QString("\"bytes_total\": %1, ").arg(bytesTotal));
    jsonText.append(QString("\"force_put\": %1, ").arg( (forcePut)?"true":"false" ));
    jsonText.append(QString("\"force_get\": %1, ").arg( (forceGet)?"true":"false" ));
    jsonText.append(QString("\"download_offset\": %1, ").arg(downloadOffset));
    jsonText.append(QString("\"upload_id\": \"%1\", ").arg(uploadId));
    jsonText.append(QString("\"upload_offset\": %1, ").arg(uploadOffset));
    jsonText.append(QString("\"created_time\": \"%1\", ").arg(createdTime.toString(Qt::ISODate)));
    jsonText.append(QString("\"last_started_time\": \"%1\", ").arg(lastStartedTime.toString(Qt::ISODate)));
    jsonText.append(QString("\"last_stopped_time\": \"%1\", ").arg(lastStoppedTime.toString(Qt::ISODate)));
    jsonText.append(QString("\"err\": %1, ").arg(err));
    jsonText.append(QString("\"err_string\": \"%1\", ").arg(errString));
    jsonText.append(QString("\"err_message\": \"%1\", ").arg(errMessage));
    jsonText.append(QString("\"retry_count\": %1, ").arg(retryCount));
    jsonText.append(QString("\"next_job_id\": \"%1\" ").arg(nextJobId));
    jsonText.append(" }");

    return jsonText;
}
