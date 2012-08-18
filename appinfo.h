#ifndef APPINFO_H
#define APPINFO_H

#include <QtGui>
#include <QDeclarativeItem>
#include <QSettings>
#include "monitoring.h"

class AppInfo : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString domain READ getDomainName WRITE setDomainName)
    Q_PROPERTY(QString app READ getAppName WRITE setAppName)
    Q_PROPERTY(QString emptyStr READ getEmptyStr NOTIFY localeChanged)
    Q_PROPERTY(QString locale READ getLocale WRITE setLocale NOTIFY localeChanged)
    Q_PROPERTY(QString emptySetting READ getEmptySetting NOTIFY settingChanged)
public:
    explicit AppInfo(QDeclarativeItem *parent = 0);
    
    QString getDomainName() const;
    void setDomainName(const QString domainName);
    QString getAppName() const;
    void setAppName(const QString appName);
    Q_INVOKABLE bool isMonitoring() const;
    void setMonitoring(const bool flag);
    Q_INVOKABLE QString getMonitoringFilePath() const;
    Q_INVOKABLE QVariant getSettingValue(const QString key, const QVariant defaultValue);
    Q_INVOKABLE bool getSettingBoolValue(const QString key, const bool defaultValue);
    Q_INVOKABLE bool setSettingValue(const QString key, const QVariant v);
    Q_INVOKABLE void startMonitoring();
    Q_INVOKABLE bool isLogging() const;

    Q_INVOKABLE QString getSystemLocale() const;
    Q_INVOKABLE QString getLocale();
    Q_INVOKABLE void setLocale(const QString locale);
    QString getEmptyStr();
    QString getEmptySetting();

    void componentComplete();
    void init();
signals:
    void notifyLoggingSignal(QString logFilePath);
    void notifyMonitoringSignal(QString monitoringFilePath);
    void localeChanged(QString locale);
    void settingChanged();
public slots:

private:
    QString m_domainName;
    QString m_appName;
    QSettings *m_settings;
    Monitoring *mon;
    QTranslator *m_ts;

    void initTS();
    bool loadTS(const QString localeName);
};

#endif // APPINFO_H
