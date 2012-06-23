#ifndef APPINFO_H
#define APPINFO_H

#include <QDeclarativeItem>
#include <QSettings>
#include "monitoring.h"

class AppInfo : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString domain READ getDomainName WRITE setDomainName)
    Q_PROPERTY(QString app READ getAppName WRITE setAppName)
public:
    explicit AppInfo(QDeclarativeItem *parent = 0);
    
    QString getDomainName() const;
    void setDomainName(const QString domainName);
    QString getAppName() const;
    void setAppName(const QString appName);
    bool isMonitoring() const;
    void setMonitoring(const bool flag);
    Q_INVOKABLE QVariant getSettingValue(const QString key, const QVariant defaultValue);
    Q_INVOKABLE void setSettingValue(const QString key, const QVariant v);
    Q_INVOKABLE void startMonitoring();

    void componentComplete();
    void init();
signals:
    
public slots:

private:
    QString m_domainName;
    QString m_appName;
    QSettings *m_settings;
    Monitoring *mon;
};

#endif // APPINFO_H
