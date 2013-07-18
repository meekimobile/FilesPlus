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
    this->isDir = false;
    this->modelIndex = modelIndex;
    this->bytes = 0;
    this->bytesTotal = 0;
    this->targetType = -1;
    this->createdTime = QDateTime::currentDateTime();
    this->downloadOffset = 0;
    this->uploadOffset = 0;
    this->err = 0;
    this->retryCount = 0;
    this->isAborted = false;
    this->suppressDeleteLocal = false;
    this->suppressRemoveJob = false;
    this->remotePathList = QStringList("*");
}

CloudDriveJob::CloudDriveJob(QScriptValue item)
{
    jobId = item.property("jobId").toString();
    operation = item.property("operation").toInteger();
    type = item.property("type").toInteger();
    uid = item.property("uid").toString();
    localFilePath = item.property("localFilePath").toString();
    remoteFilePath = item.property("remoteFilePath").toString();
    modelIndex = -1;
    newLocalFilePath = item.property("newLocalFilePath").toString();
    newRemoteFilePath = item.property("newRemoteFilePath").toString();
    newRemoteFileName = item.property("newRemoteFileName").toString();
    targetUid = item.property("targetUid").toString();
    targetType = item.property("targetType").toInteger();
    bytes = item.property("bytes").toInteger();
    bytesTotal = item.property("bytesTotal").toInteger();
    forcePut = item.property("forcePut").toBool();
    downloadOffset = item.property("downloadOffset").toInteger();
    uploadId = item.property("uploadId").toString();
    uploadOffset = item.property("upload_offset").toInteger();
    createdTime = item.property("createdTime").toDateTime();
    lastStartedTime = item.property("lastStartedTime").toDateTime();
    lastStoppedTime = item.property("lastStoppedTime").toDateTime();
    err = item.property("err").toInteger();
    errString = item.property("errString").toString();
    errMessage = item.property("errMessage").toString();
    nextJobId = item.property("nextJobId").toString();
    isRunning = false;
    isDir = false;
    retryCount = 0;
    isAborted = false;
    suppressDeleteLocal = false;
    suppressRemoveJob = false;
    remotePathList = QStringList("*");
}

QString CloudDriveJob::toJsonText()
{
    QString jsonText;
    jsonText.append("{ ");
    jsonText.append(QString("\"jobId\": \"%1\", ").arg(jobId));
    jsonText.append(QString("\"isRunning\": %1, ").arg( (isRunning)?"true":"false" ));
    jsonText.append(QString("\"isDir\": %1, ").arg( (isDir)?"true":"false" ));
    jsonText.append(QString("\"uid\": \"%1\", ").arg(uid));
    jsonText.append(QString("\"type\": %1, ").arg(type));
    jsonText.append(QString("\"operation\": %1, ").arg(operation));
    jsonText.append(QString("\"localFilePath\": \"%1\", ").arg(localFilePath));
    jsonText.append(QString("\"remoteFilePath\": \"%1\", ").arg(remoteFilePath));
    jsonText.append(QString("\"newLocalFilePath\": \"%1\", ").arg(newLocalFilePath));
    jsonText.append(QString("\"newRemoteFilePath\": \"%1\", ").arg(newRemoteFilePath));
    jsonText.append(QString("\"newRemoteFileName\": \"%1\", ").arg(newRemoteFileName));
    jsonText.append(QString("\"targetUid\": \"%1\", ").arg(targetUid));
    jsonText.append(QString("\"target_type\": %1, ").arg(targetType));
    jsonText.append(QString("\"modelIndex\": %1, ").arg(modelIndex));
    jsonText.append(QString("\"bytes\": %1, ").arg(bytes));
    jsonText.append(QString("\"bytesTotal\": %1, ").arg(bytesTotal));
    jsonText.append(QString("\"forcePut\": %1, ").arg( (forcePut)?"true":"false" ));
    jsonText.append(QString("\"forceGet\": %1, ").arg( (forceGet)?"true":"false" ));
    jsonText.append(QString("\"downloadOffset\": %1, ").arg(downloadOffset));
    jsonText.append(QString("\"uploadId\": \"%1\", ").arg(uploadId));
    jsonText.append(QString("\"uploadOffset\": %1, ").arg(uploadOffset));
    jsonText.append(QString("\"createdTime\": \"%1\", ").arg(createdTime.toString(Qt::ISODate)));
    jsonText.append(QString("\"lastStartedTime\": \"%1\", ").arg(lastStartedTime.toString(Qt::ISODate)));
    jsonText.append(QString("\"lastStoppedTime\": \"%1\", ").arg(lastStoppedTime.toString(Qt::ISODate)));
    jsonText.append(QString("\"err\": %1, ").arg(err));
    jsonText.append(QString("\"errString\": \"%1\", ").arg(errString));
    jsonText.append(QString("\"errMessage\": \"%1\", ").arg(errMessage));
    jsonText.append(QString("\"retryCount\": %1, ").arg(retryCount));
    jsonText.append(QString("\"nextJobId\": \"%1\" ").arg(nextJobId));
    jsonText.append(" }");

    return jsonText;
}

QVariant CloudDriveJob::toJson()
{
    QMap<QString,QVariant> jsonObj;
    jsonObj["jobId"] = QVariant(jobId);
    jsonObj["isRunning"] = QVariant(isRunning);
    jsonObj["isDir"] = QVariant(isDir);
    jsonObj["uid"] = QVariant(uid);
    jsonObj["type"] = QVariant(type);
    jsonObj["operation"] = QVariant(operation);
    jsonObj["localFilePath"] = QVariant(localFilePath);
    jsonObj["remoteFilePath"] = QVariant(remoteFilePath);
    jsonObj["newLocalFilePath"] = QVariant(newLocalFilePath);
    jsonObj["newRemoteFilePath"] = QVariant(newRemoteFilePath);
    jsonObj["newRemoteFileName"] = QVariant(newRemoteFileName);
    jsonObj["targetUid"] = QVariant(targetUid);
    jsonObj["targetType"] = QVariant(targetType);
    jsonObj["modelIndex"] = QVariant(modelIndex);
    jsonObj["bytes"] = QVariant(bytes);
    jsonObj["bytesTotal"] = QVariant(bytesTotal);
    jsonObj["forcePut"] = QVariant(forcePut);
    jsonObj["forceGet"] = QVariant(forceGet);
    jsonObj["downloadOffset"] = QVariant(downloadOffset);
    jsonObj["uploadId"] = QVariant(uploadId);
    jsonObj["uploadOffset"] = QVariant(uploadOffset);
    jsonObj["createdTime"] = QVariant(createdTime.toString(Qt::ISODate));
    jsonObj["lastStartedTime"] = QVariant(lastStartedTime.toString(Qt::ISODate));
    jsonObj["lastStoppedTime"] = QVariant(lastStoppedTime.toString(Qt::ISODate));
    jsonObj["err"] = QVariant(err);
    jsonObj["errString"] = QVariant(errString);
    jsonObj["errMessage"] = QVariant(errMessage);
    jsonObj["retryCount"] = QVariant(retryCount);
    jsonObj["nextJobId"] = QVariant(nextJobId);

    return QVariant(jsonObj);
}
