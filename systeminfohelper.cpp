#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include "systeminfohelper.h"

SystemInfoHelper::SystemInfoHelper(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    m_ssi = new QSystemStorageInfo(this);
    m_fileInfoCache = new QCache<QString, QFileInfo>();
}

int SystemInfoHelper::getDriveTypeInt(const QString &drive)
{
    QSystemStorageInfo::DriveType drvType = m_ssi->typeForDrive(drive);
    int drvTypeInt = -1;

    switch(drvType) {
    case QSystemStorageInfo::NoDrive:
    {
        drvTypeInt = 0;
        break;
    }
    case QSystemStorageInfo::InternalDrive:
    {
        drvTypeInt = 1;
        break;
    }
    case QSystemStorageInfo::RemovableDrive:
    {
        drvTypeInt = 2;
        break;
    }
    case QSystemStorageInfo::RemoteDrive:
    {
        drvTypeInt = 3;
        break;
    }
    case QSystemStorageInfo::CdromDrive:
    {
        drvTypeInt = 4;
        break;
    }
    case QSystemStorageInfo::InternalFlashDrive:
    {
        drvTypeInt = 5;
        break;
    }
    case QSystemStorageInfo::RamDrive:
    {
        drvTypeInt = 6;
        break;
    }
    default:
    {
        break;
    }
    }

    return drvTypeInt;
}

QStringList SystemInfoHelper::getDriveList() {
    QStringList driveList;

#if defined(Q_WS_SIMULATOR)
    qDebug() << "SystemInfoHelper::getDriveList platform=Q_WS_SIMULATOR";
    QFileInfoList drives = QDir::drives();
    for (int i=0; i<drives.length(); i++) {
        qDebug() << "SystemInfoHelper::getDriveList drives" << drives.at(i).absoluteFilePath();
        driveList.append(drives.at(i).absoluteFilePath());
    }
#elif defined(Q_OS_SYMBIAN)
    qDebug() << "SystemInfoHelper::getDriveList platform=Q_OS_SYMBIAN";
    QFileInfoList drives = QDir::drives();
    for (int i=0; i<drives.length(); i++) {
        qDebug() << "SystemInfoHelper::getDriveList drives" << drives.at(i).absoluteFilePath();
        driveList.append(drives.at(i).absoluteFilePath());
    }
#endif

    return driveList;
}

QString SystemInfoHelper::getPrivateDrive()
{
#if defined(Q_OS_SYMBIAN)
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
    return "";
#endif

}

QString SystemInfoHelper::getFileContent(const QString &localPath)
{
    qDebug() << "SystemInfoHelper::getFileContent localPath " << localPath;

    if (localPath.isEmpty()) return "";

    QString text;
    QFile file(localPath);
    if (file.open(QIODevice::ReadOnly)) {
        text.append(file.readAll());
    }
    file.close();

    qDebug() << "SystemInfoHelper::getFileContent text.length() " << text.length();

    return text;
}

int SystemInfoHelper::saveFileContent(const QString &localPath, const QString &text)
{
    qDebug() << "SystemInfoHelper::saveFileContent localPath " << localPath;

    if (localPath.isEmpty()) return -1;

    qint64 c = -1;
    QFile file(localPath);
    if (file.open(QIODevice::WriteOnly)) {
        c = file.write(text.toUtf8());
    }
    file.close();

    qDebug() << "SystemInfoHelper::saveFileContent bytes write " << c;

    return c;
}

QVariant SystemInfoHelper::getFileAttribute(const QString &localPath, const QString &attributeName)
{
    QFileInfo *fi = m_fileInfoCache->object(localPath);
    if (fi == 0) {
        fi = new QFileInfo(localPath);
    }

    // Issue: QFileInfo and QFile dont' have method to set attributes.
    if (attributeName == "permission.ReadOwner") {
        return fi->permission(QFile::ReadOwner);
    } else {
        return NULL;
    }
}

QString SystemInfoHelper::getUrl(const QString absPath)
{
    return QUrl::fromLocalFile(absPath).toString();
}

QString SystemInfoHelper::getCompletedUrl(const QString userInputLocation)
{
    return QUrl::fromUserInput(userInputLocation).toString();
}

