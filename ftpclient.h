#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QDeclarativeItem>
#include <QFtp>

class FtpClient : public QDeclarativeItem
{
    Q_OBJECT
public:
    explicit FtpClient(QDeclarativeItem *parent = 0);
    
    void test();
signals:
    
public slots:
    void ftpCommandFinished(int id, bool error);
    void addToList(const QUrlInfo &i);
    void updateDataTransferProgress(qint64 done, qint64 total);

private:
    QFtp *m_ftp;
    
};

#endif // FTPCLIENT_H
