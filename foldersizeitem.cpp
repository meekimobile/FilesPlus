#include "foldersizeitem.h"
#include <QRegExp>

FolderSizeItem::FolderSizeItem()
{
    this->isRunning = false;
    this->runningOperation = 0;
    this->runningValue = 0;
    this->runningMaxValue = 0;
    this->isChecked = false;
    this->isHidden = false;
    this->isReadOnly = false;
}

FolderSizeItem::FolderSizeItem(const QString &name, const QString &absolutePath, const QDateTime &lastModified, const qint64 &size, const bool isDir, const qint64 &subDirCount, const qint64 &subFileCount)
{
    this->name = name;
    this->absolutePath = absolutePath;
    this->lastModified = lastModified;
    this->size = size;
    this->isDir = isDir;
    this->subDirCount = subDirCount;
    this->subFileCount = subFileCount;

    setFileType();
    this->isRunning = false;
    this->runningOperation = 0;
    this->runningValue = 0;
    this->runningMaxValue = 0;
    this->isChecked = false;
    this->isHidden = false;
    this->isReadOnly = false;
}

QString FolderSizeItem::toJsonText()
{
    QString jsonText("{ ");
    jsonText.append(QString("\"name\": \"%1\", ").arg(name));
    jsonText.append(QString("\"absolutePath\": \"%1\", ").arg(absolutePath));
    jsonText.append(QString("\"lastModified\": \"%1\", ").arg(lastModified.toString(Qt::ISODate)));
    jsonText.append(QString("\"size\": %1, ").arg(size));
    jsonText.append(QString("\"isDir\": %1, ").arg((isDir)?"true":"false"));
    jsonText.append(QString("\"subDir_count\": %1, ").arg(subDirCount));
    jsonText.append(QString("\"subFile_count\": %1, ").arg(subFileCount));
    jsonText.append(QString("\"fileType\": \"%1\", ").arg(fileType));
    jsonText.append(QString("\"isRunning\": %1, ").arg((isRunning)?"true":"false"));
    jsonText.append(QString("\"runningOperation\": %1, ").arg(runningOperation));
    jsonText.append(QString("\"runningValue\": %1, ").arg(runningValue));
    jsonText.append(QString("\"runningMaxValue\": %1, ").arg(runningMaxValue));
    jsonText.append(QString("\"isChecked\": %1, ").arg(isChecked));
    jsonText.append(QString("\"isDirty\": %1, ").arg(isDirty));
    jsonText.append(QString("\"isHidden\": %1, ").arg(isHidden));
    jsonText.append(QString("\"isReadOnly\": %1 ").arg(isReadOnly));
    jsonText.append("}");

    return jsonText;
}

QVariant FolderSizeItem::toJson()
{
    QMap<QString,QVariant> jsonObj;
    jsonObj["name"] = QVariant(name);
    jsonObj["absolutePath"] = QVariant(absolutePath);
    jsonObj["size"] = QVariant(size);
    jsonObj["lastModified"] = QVariant(lastModified.toString(Qt::ISODate));
    jsonObj["isDir"] = QVariant(isDir);
    jsonObj["subDirCount"] = QVariant(subDirCount);
    jsonObj["subFileCount"] = QVariant(subFileCount);
    jsonObj["fileType"] = QVariant(fileType);
    jsonObj["isRunning"] = QVariant(isRunning);
    jsonObj["runningValue"] = QVariant(runningValue);
    jsonObj["runningMaxValue"] = QVariant(runningMaxValue);
    jsonObj["runningOperation"] = QVariant(runningOperation);
    jsonObj["isChecked"] = QVariant(isChecked);
    jsonObj["isDirty"] = QVariant(isDirty);
    jsonObj["isHidden"] = QVariant(isHidden);
    jsonObj["isReadOnly"] = QVariant(isReadOnly);

    return QVariant(jsonObj);
}

void FolderSizeItem::setFileType()
{
    // Get file extension.
    QRegExp rx("^(.*)(\\.)(.+)$");
    rx.indexIn(name);
    if (rx.captureCount() == 3) {
        fileType = rx.cap(3);
    }
}

QDataStream &operator<<(QDataStream &out, const FolderSizeItem &item)
{
    // Save only dir.
    if (item.isDir) {
        out << item.name << item.absolutePath << item.lastModified << item.size << item.subDirCount << item.subFileCount;
    }
    return out;
}

QDataStream &operator>>(QDataStream &in, FolderSizeItem &item)
{
    QString name;
    QString absolutePath;
    QDateTime lastModified;
    qint64 size;
    qint64 subDirCount;
    qint64 subFileCount;

    // Load to dir.
    in >> name >> absolutePath >> lastModified >> size >> subDirCount >> subFileCount;
    item = FolderSizeItem(name, absolutePath, lastModified, size, true, subDirCount, subFileCount);

    return in;
}

QDebug &operator<<(QDebug &out, const FolderSizeItem &item)
{
    out << "FolderSizeItem(" << item.name << "," << item.fileType << "," << item.absolutePath << "," << item.lastModified
        << "," << item.size << "," << item.subDirCount << "," << item.subFileCount << "," << item.isDir << ")";

    return out;
}

