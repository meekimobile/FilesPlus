#ifndef APPINFO_H
#define APPINFO_H

#include <QtGui>
#include <QDeclarativeItem>
#include <QSettings>
#include "monitoring.h"

class AppInfo : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString emptyStr READ getEmptyStr NOTIFY localeChanged)
    Q_PROPERTY(QString locale READ getLocale WRITE setLocale NOTIFY localeChanged)
    Q_PROPERTY(QString emptySetting READ getEmptySetting NOTIFY settingChanged)
    Q_PROPERTY(QString version READ getVersion NOTIFY versionChanged)
public:
    explicit AppInfo(QDeclarativeItem *parent = 0);
    
    QString getVersion() const;

    Q_INVOKABLE QVariant getSettingValue(const QString key, const QVariant defaultValue);
    Q_INVOKABLE bool getSettingBoolValue(const QString key, const bool defaultValue);
    Q_INVOKABLE int getSettingIntValue(const QString key, const int defaultValue);
    Q_INVOKABLE bool setSettingValue(const QString key, const QVariant v, const bool forceUpdate = false);
    Q_INVOKABLE bool hasSettingValue(const QString key);
    Q_INVOKABLE void removeSettingValue(const QString key);

    // Monitoring for Symbian.
    Q_INVOKABLE bool isMonitoring();
    void setMonitoring(const bool flag);
    Q_INVOKABLE QString getMonitoringFilePath() const;
    Q_INVOKABLE void toggleMonitoring();

    // Logging.
    Q_INVOKABLE bool isLogging();
    Q_INVOKABLE void stopLogging();
    Q_INVOKABLE QString getLogPath() const;

    // Locale.
    Q_INVOKABLE QString getSystemLocale() const;
    Q_INVOKABLE QString getLocale();
    Q_INVOKABLE void setLocale(const QString locale);
    QString getEmptyStr();
    QString getEmptySetting();

    // Clipboard.
    Q_INVOKABLE void addToClipboard(const QString text);
    Q_INVOKABLE QString getFromClipboard();
    Q_INVOKABLE void clearClipboard();

    // Configuration.
    Q_INVOKABLE QString getConfigPath();

    void componentComplete();
    void init();
signals:
    void notifyLoggingSignal(QString logFilePath);
    void notifyMonitoringSignal(QString monitoringFilePath);
    void localeChanged(QString locale);
    void settingChanged();
    void versionChanged();
public slots:

private:
    QSettings *m_settings;
    Monitoring *mon;
    QTranslator *m_ts;

    void initTS();
    bool loadTS(const QString localeName);
};

#endif // APPINFO_H
