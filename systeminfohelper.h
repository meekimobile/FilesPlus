#ifndef SYSTEMINFOHELPER_H
#define SYSTEMINFOHELPER_H

#include <QDeclarativeItem>
#include <qsystemstorageinfo.h>

using namespace QtMobility;

class SystemInfoHelper : public QDeclarativeItem
{
    Q_OBJECT
public:
    explicit SystemInfoHelper(QDeclarativeItem *parent = 0);
    
    Q_INVOKABLE int getDriveTypeInt(const QString &drive);
signals:
    
public slots:
    
private:
    QSystemStorageInfo *m_ssi;
};

#endif // SYSTEMINFOHELPER_H
