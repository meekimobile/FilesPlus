#include "foldersizeitem.h"
#include <QRegExp>

FolderSizeItem::FolderSizeItem()
{
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
}

QString FolderSizeItem::toJsonText()
{
    QString jsonText("{ ");
    jsonText.append(QString("\"name\": \"%1\", ").arg(name));
    jsonText.append(QString("\"absolute_path\": \"%1\", ").arg(absolutePath));
    jsonText.append(QString("\"last_modified\": \"%1\", ").arg(lastModified.toString()));
    jsonText.append(QString("\"size\": %1, ").arg(size));
    jsonText.append(QString("\"is_dir\": %1, ").arg((isDir)?"true":"false"));
    jsonText.append(QString("\"sub_dir_count\": %1, ").arg(subDirCount));
    jsonText.append(QString("\"sub_file_count\": %1, ").arg(subFileCount));
    jsonText.append(QString("\"file_type\": \"%1\", ").arg(fileType));
    jsonText.append(QString("\"is_running\": %1, ").arg((isRunning)?"true":"false"));
    jsonText.append(QString("\"running_operation\": %1, ").arg(runningOperation));
    jsonText.append(QString("\"running_value\": %1, ").arg(runningValue));
    jsonText.append(QString("\"running_max_value\": %1, ").arg(runningMaxValue));
    jsonText.append(QString("\"is_checked\": %1 ").arg(isChecked));
    jsonText.append("}");

    return jsonText;
}

void FolderSizeItem::setFileType()
{
    // Get file extension.
    QRegExp rx("^(.+)(\\.)(.+)$");
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
    out << "FolderSizeItem(" << item.name << "," << item.absolutePath << "," << item.lastModified << "," << item.size << "," << item.isDir << ")";

    return out;
}

