#ifndef SYSTEMINFOHELPER_H
#define SYSTEMINFOHELPER_H

#include <QDeclarativeItem>
#include <QCache>
#include <QFileInfo>
#include <QSystemStorageInfo>

using namespace QtMobility;

class SystemInfoHelper : public QDeclarativeItem
{
    Q_OBJECT
public:
    explicit SystemInfoHelper(QDeclarativeItem *parent = 0);
    
    Q_INVOKABLE int getDriveTypeInt(const QString &drive);
    Q_INVOKABLE QStringList getDriveList();
    Q_INVOKABLE QString getPrivateDrive();

    Q_INVOKABLE QString getFileContent(const QString &localPath);
    Q_INVOKABLE int saveFileContent(const QString &localPath, const QString &text, bool useWindowsEOL = false);

    Q_INVOKABLE QVariant getFileAttribute(const QString &localPath, const QString &attributeName);

    Q_INVOKABLE QString getUrl(const QString absPath);
    Q_INVOKABLE QString getCompletedUrl(const QString userInputLocation);

    Q_INVOKABLE void shareFile(const QString &absolutePath);
    Q_INVOKABLE void shareUrl(const QString &urlString, const QString &title = "", const QString &desc = "");
signals:
    
public slots:
    
private:
    QSystemStorageInfo *m_ssi;
    QCache<QString, QFileInfo> *m_fileInfoCache;
};

#endif // SYSTEMINFOHELPER_H
