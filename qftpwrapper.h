#ifndef QFTPWRAPPER_H
#define QFTPWRAPPER_H

#include <QFtp>
#include <QApplication>
#include <QRegExp>
#include <QDebug>
#include "sleeper.h"

class QFtpWrapper : public QFtp
{
    Q_OBJECT
public:
    static const int MaxWaitCount;

    explicit QFtpWrapper(QString nonce, QObject *parent = 0);
    
    int pwd();
    int rawCommand(const QString &command);

    QString getCurrentPath();
    QString getNonce();
    QString getUid();
    QList<QUrlInfo> getItemList();
    void resetIsDone();
    void waitForDone();

    // Parameter storage.
    QString m_uid;
    QString m_localFilePath;
    QString m_remoteFilePath;
signals:
    void uploadProgress(QString nonce, qint64 done, qint64 total);
    void downloadProgress(QString nonce, qint64 done, qint64 total);
    void stateChanged(QString nonce, int state);
    void listInfo(QString nonce, const QUrlInfo &i);
    void readyRead(QString nonce);
    void rawCommandReply(QString nonce, int replyCode, const QString &detail);
    void commandStarted(QString nonce, int id);
    void commandFinished(QString nonce, int id, bool error);
    void done(QString nonce, bool error);
    void deleteRecursiveFinished(QString nonce, int error, QString errorString);

public slots:
    void commandStartedFilter(int id);
    void commandFinishedFilter(int id, bool error);
    void listInfoFilter(const QUrlInfo &i);
    void rawCommandReplyFilter(int replyCode, QString result);
    void dataTransferProgressFilter(qint64 done, qint64 total);
    void stateChangedFilter(int state);
    void doneFilter(bool error);

    // Extended methods.
    bool deleteRecursive(QString remoteFilePath = "");
private:
    bool m_isDone;
    QString m_nonce;
    QString m_currentPath;
    QString m_lastRawCommand;
    QList<QUrlInfo> m_itemList;
};

#endif // QFTPWRAPPER_H
