#include "ftpclient.h"

FtpClient::FtpClient(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    m_ftp = new QFtp(this);
}

void FtpClient::test()
{
    connect(m_ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(ftpCommandFinished(int,bool)));
    connect(m_ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(addToList(QUrlInfo)));
    connect(m_ftp, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(updateDataTransferProgress(qint64,qint64)));

    m_ftp->connectToHost("192.168.0.100", 21);
    m_ftp->login("meek", "Maijung04");
//    m_ftp->cd();

    m_ftp->list();
    m_ftp->close();
}

void FtpClient::ftpCommandFinished(int id, bool error)
{
    qDebug() << "FtpClient::ftpCommandFinished" << id << m_ftp->currentCommand() << error;
    if (error) {
        qDebug() << "FtpClient::ftpCommandFinished error" <<  m_ftp->errorString();
    }
}

void FtpClient::addToList(const QUrlInfo &i)
{
    qDebug() << "FtpClient::addToList" << i.name() << i.isDir() << i.isFile() << i.lastModified();
}

void FtpClient::updateDataTransferProgress(qint64 done, qint64 total)
{
    qDebug() << "FtpClient::updateDataTransferProgress" << done << total;
}
