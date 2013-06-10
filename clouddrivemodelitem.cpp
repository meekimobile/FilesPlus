#include "clouddrivemodelitem.h"

CloudDriveModelItem::CloudDriveModelItem()
{
    this->subDirCount = 0;
    this->subFileCount = 0;
    this->isRunning = false;
    this->runningOperation = 0;
    this->runningValue = 0;
    this->runningMaxValue = 0;
    this->isChecked = false;
    this->isConnected = false;
    this->isDeleted = false;
    this->isHidden = false;
    this->isReadOnly = false;
    this->timestamp = QDateTime::currentMSecsSinceEpoch();
}

QString CloudDriveModelItem::toJsonText()
{
    QString jsonText("{ ");
    jsonText.append(QString("\"name\": \"%1\", ").arg(name));
    jsonText.append(QString("\"absolute_path\": \"%1\", ").arg(absolutePath));
    jsonText.append(QString("\"parent_path\": \"%1\", ").arg(parentPath));
    jsonText.append(QString("\"size\": %1, ").arg(size));
    jsonText.append(QString("\"last_modified\": \"%1\", ").arg(lastModified.toString(Qt::ISODate)));
    jsonText.append(QString("\"is_dir\": %1, ").arg((isDir)?"true":"false"));
    jsonText.append(QString("\"hash\": \"%1\", ").arg(hash));
    jsonText.append(QString("\"sub_dir_count\": %1, ").arg(subDirCount));
    jsonText.append(QString("\"sub_file_count\": %1, ").arg(subFileCount));
    jsonText.append(QString("\"file_type\": \"%1\", ").arg(fileType));
    jsonText.append(QString("\"is_deleted\": %1, ").arg(isDeleted?"true":"false"));
    jsonText.append(QString("\"is_hidden\": %1, ").arg(isHidden?"true":"false"));
    jsonText.append(QString("\"is_read_only\": %1, ").arg(isReadOnly?"true":"false"));
    jsonText.append(QString("\"source\": \"%1\", ").arg(source));
    jsonText.append(QString("\"alternative\": \"%1\", ").arg(alternative));
    jsonText.append(QString("\"thumbnail\": \"%1\", ").arg(thumbnail));
    jsonText.append(QString("\"thumbnail128\": \"%1\", ").arg(thumbnail128));
    jsonText.append(QString("\"preview\": \"%1\", ").arg(preview));
    jsonText.append(QString("\"downloadUrl\": \"%1\", ").arg(downloadUrl));
    jsonText.append(QString("\"webContentLink\": \"%1\", ").arg(webContentLink));
    jsonText.append(QString("\"embedLink\": \"%1\", ").arg(embedLink));
    jsonText.append(QString("\"is_running\": %1, ").arg((isRunning)?"true":"false"));
    jsonText.append(QString("\"running_value\": %1, ").arg(runningValue));
    jsonText.append(QString("\"running_max_value\": %1, ").arg(runningMaxValue));
    jsonText.append(QString("\"running_operation\": %1, ").arg(runningOperation));
    jsonText.append(QString("\"is_checked\": %1, ").arg(isChecked?"true":"false"));
    jsonText.append(QString("\"is_dirty\": %1, ").arg(isDirty?"true":"false"));
    jsonText.append(QString("\"is_connected\": %1, ").arg(isConnected?"true":"false"));
    jsonText.append(QString("\"timestamp\": %1 ").arg(timestamp));
    jsonText.append("}");

    return jsonText;
}

QDebug &operator<<(QDebug &out, const CloudDriveModelItem &item)
{
    out << "CloudDriveModelItem(" << item.name << "," << item.fileType << "," << item.absolutePath << "," << item.parentPath
        << "," << item.lastModified << "," << item.hash
        << "," << item.size << "," << item.subDirCount << "," << item.subFileCount << "," << item.isDir << ")";

    return out;
}

