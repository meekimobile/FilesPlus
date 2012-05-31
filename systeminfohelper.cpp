#include "systeminfohelper.h"

SystemInfoHelper::SystemInfoHelper(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    m_ssi = new QSystemStorageInfo(this);
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
