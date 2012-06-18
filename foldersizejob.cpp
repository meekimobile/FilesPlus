#include "foldersizejob.h"

FolderSizeJob::FolderSizeJob()
{
}

FolderSizeJob::FolderSizeJob(QString jobId, int operation, QString sourcePath, QString targetPath, bool clearCache)
{
    this->jobId = jobId;
    this->operation = operation;
    this->sourcePath = sourcePath;
    this->targetPath = targetPath;
    this->clearCache = clearCache;
}

QString FolderSizeJob::toJsonText()
{
    QString jsonText;
    jsonText.append("{ ");
    jsonText.append(QString("\"job_id\": \"%1\", ").arg(jobId));
    jsonText.append(QString("\"is_running\": %1, ").arg( (isRunning)?"true":"false" ));
    jsonText.append(QString("\"operation\": %1, ").arg(operation));
    jsonText.append(QString("\"source_path\": \"%1\", ").arg(sourcePath));
    jsonText.append(QString("\"target_path\": \"%1\", ").arg(targetPath));
    jsonText.append(QString("\"clear_cache\": %1, ").arg( (clearCache)?"true":"false" ));
    jsonText.append(QString("\"bytes\": %1, ").arg(bytes));
    jsonText.append(QString("\"bytes_total\": %1 ").arg(bytesTotal));
    jsonText.append(" }");

    return jsonText;
}
