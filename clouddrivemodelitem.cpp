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
    jsonText.append(QString("\"absolutePath\": \"%1\", ").arg(absolutePath));
    jsonText.append(QString("\"parentPath\": \"%1\", ").arg(parentPath));
    jsonText.append(QString("\"size\": %1, ").arg(size));
    jsonText.append(QString("\"lastModified\": \"%1\", ").arg(lastModified.toString(Qt::ISODate)));
    jsonText.append(QString("\"isDir\": %1, ").arg((isDir)?"true":"false"));
    jsonText.append(QString("\"hash\": \"%1\", ").arg(hash));
    jsonText.append(QString("\"subDirCount\": %1, ").arg(subDirCount));
    jsonText.append(QString("\"subFileCount\": %1, ").arg(subFileCount));
    jsonText.append(QString("\"fileType\": \"%1\", ").arg(fileType));
    jsonText.append(QString("\"isDeleted\": %1, ").arg(isDeleted?"true":"false"));
    jsonText.append(QString("\"isHidden\": %1, ").arg(isHidden?"true":"false"));
    jsonText.append(QString("\"isReadOnly\": %1, ").arg(isReadOnly?"true":"false"));
    jsonText.append(QString("\"source\": \"%1\", ").arg(source));
    jsonText.append(QString("\"alternative\": \"%1\", ").arg(alternative));
    jsonText.append(QString("\"thumbnail\": \"%1\", ").arg(thumbnail));
    jsonText.append(QString("\"thumbnail128\": \"%1\", ").arg(thumbnail128));
    jsonText.append(QString("\"preview\": \"%1\", ").arg(preview));
    jsonText.append(QString("\"downloadUrl\": \"%1\", ").arg(downloadUrl));
    jsonText.append(QString("\"webContentLink\": \"%1\", ").arg(webContentLink));
    jsonText.append(QString("\"embedLink\": \"%1\", ").arg(embedLink));
    jsonText.append(QString("\"isRunning\": %1, ").arg((isRunning)?"true":"false"));
    jsonText.append(QString("\"runningValue\": %1, ").arg(runningValue));
    jsonText.append(QString("\"runningMaxValue\": %1, ").arg(runningMaxValue));
    jsonText.append(QString("\"runningOperation\": %1, ").arg(runningOperation));
    jsonText.append(QString("\"isChecked\": %1, ").arg(isChecked?"true":"false"));
    jsonText.append(QString("\"isDirty\": %1, ").arg(isDirty?"true":"false"));
    jsonText.append(QString("\"isConnected\": %1, ").arg(isConnected?"true":"false"));
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

