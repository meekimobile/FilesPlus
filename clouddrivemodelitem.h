#ifndef CLOUDDRIVEMODELITEM_H
#define CLOUDDRIVEMODELITEM_H

#include <QDateTime>
#include <QDebug>

class CloudDriveModelItem
{
public:
    CloudDriveModelItem();

    // Properties from cloud service.
    QString name;
    QString absolutePath;
    QString parentPath;
    qint64 size;
    QDateTime lastModified;
    bool isDir;
    QString hash;
    qint64 subDirCount;
    qint64 subFileCount;
    QString fileType;
    bool isDeleted;
    bool isHidden;
    bool isReadOnly;
    QString source;
    QString alternative;
    QString thumbnail;
    QString thumbnail128;
    QString preview;
    QString downloadUrl;
    QString webContentLink;
    QString embedLink;

    // Temporary Properties.
    bool isRunning;
    qint64 runningValue;
    qint64 runningMaxValue;
    int runningOperation;
    bool isChecked;
    bool isDirty;
    bool isConnected;
    qint64 timestamp;


    QString toJsonText();
private:

};

QDebug &operator<<(QDebug &out, const CloudDriveModelItem &t);

#endif // CLOUDDRIVEMODELITEM_H
