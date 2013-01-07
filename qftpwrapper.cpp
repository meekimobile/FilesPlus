#include "qftpwrapper.h"

QFtpWrapper::QFtpWrapper(QString nonce, QObject *parent) :
    QFtp(parent)
{
    m_nonce = nonce;
    m_isDone = false;
    m_currentPath = "";

    connect(this, SIGNAL(commandStarted(int)), this, SLOT(commandStartedFilter(int)));
    connect(this, SIGNAL(commandFinished(int,bool)), this, SLOT(commandFinishedFilter(int,bool)));
    connect(this, SIGNAL(rawCommandReply(int,QString)), this, SLOT(rawCommandReplyFilter(int,QString)));
    connect(this, SIGNAL(listInfo(QUrlInfo)), this, SLOT(listInfoFilter(QUrlInfo)));
    connect(this, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(dataTransferProgressFilter(qint64,qint64)));
    connect(this, SIGNAL(stateChanged(int)), this, SLOT(stateChangedFilter(int)));
    connect(this, SIGNAL(done(bool)), this, SLOT(doneFilter(bool)));
}

int QFtpWrapper::pwd()
{
    return rawCommand("PWD");
}

int QFtpWrapper::rawCommand(const QString &command)
{
    m_lastRawCommand = command;

    return QFtp::rawCommand(command);
}

QString QFtpWrapper::getCurrentPath()
{
    return m_currentPath;
}

QString QFtpWrapper::getNonce()
{
    return m_nonce;
}

QList<QUrlInfo> QFtpWrapper::getItemList()
{
    return m_itemList;
}

void QFtpWrapper::resetIsDone()
{
    m_isDone = false;
}

void QFtpWrapper::waitForDone()
{
    qDebug() << "QFtpWrapper::waitForDone" << m_isDone;

    int c = 100;
    while (!m_isDone && c-- > 0) {
//        qDebug() << "QFtpWrapper::waitForDone" << m_isDone << c;
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Reset m_isDone.
    m_isDone = false;
}

bool QFtpWrapper::deleteRecursive(QString remoteFilePath)
{
    if (remoteFilePath == "") {
        remoteFilePath = m_remoteFilePath;
    }

    qDebug() << "QFtpWrapper::deleteRecursive" << remoteFilePath;

    // Check if it's folder by cd.
    cd(remoteFilePath);
    waitForDone();
    if (error() == QFtp::NoError) {
        // It's folder. Drill it down.
        list();
        waitForDone();
        // ISSUE list() get clear before start listing. When pop to parent level, the list is empty.
        QList<QUrlInfo> itemList = getItemList();
        for (int i = 0; i < itemList.count(); i++) {
            qDebug() << "QFtpWrapper::deleteRecursive item" << remoteFilePath + "/" + itemList.at(i).name() << "isDir" << itemList.at(i).isDir();
            if (itemList.at(i).isDir()) {
                // Drill down into folder.
                bool res = deleteRecursive(remoteFilePath + "/" + itemList.at(i).name());
                if (!res) {
                    return false;
                }
            } else {
                // Delete a file.
                remove(itemList.at(i).name());
                waitForDone();
                if (error() != QFtp::NoError) {
                    qDebug() << "QFtpWrapper::deleteRecursive can't delete file" << remoteFilePath + "/" + itemList.at(i).name() << error() << errorString();
                    emit deleteRecursiveFinished(m_nonce, error(), errorString());
                    return false;
                } else {
                    qDebug() << "QFtpWrapper::deleteRecursive delete file" << remoteFilePath + "/" + itemList.at(i).name();
                }
            }
        }

        // Delete parent folder.
        cd("..");
        rmdir(remoteFilePath);
        waitForDone();
        if (error() != QFtp::NoError) {
            qDebug() << "QFtpWrapper::deleteRecursive can't delete folder" << remoteFilePath << error() << errorString();
            emit deleteRecursiveFinished(m_nonce, error(), errorString());
            return false;
        } else {
            qDebug() << "QFtpWrapper::deleteRecursive delete folder" << remoteFilePath;
        }
    } else {
        // It's a file. Delete it.
        remove(remoteFilePath);
        waitForDone();
        if (error() != QFtp::NoError) {
            qDebug() << "QFtpWrapper::deleteRecursive can't delete file" << remoteFilePath << error() << errorString();
            emit deleteRecursiveFinished(m_nonce, error(), errorString());
            return false;
        } else {
            qDebug() << "QFtpWrapper::deleteRecursive delete file" << remoteFilePath;
        }
    }

    emit deleteRecursiveFinished(m_nonce, error(), errorString());
    return true;
}

void QFtpWrapper::commandStartedFilter(int id)
{
    m_isDone = false;
    m_itemList.clear();

    emit commandStarted(m_nonce, id);
}

void QFtpWrapper::commandFinishedFilter(int id, bool error)
{
    emit commandFinished(m_nonce, id, error);
}

void QFtpWrapper::listInfoFilter(const QUrlInfo &i)
{
    // TODO File name is not UTF-8, workaround by re-encoding.
//    QString modName = QString::fromUtf8(i.name().toAscii());
//    qDebug() << "QFtpWrapper::listInfoFilter" << i.name() << modName;
//    QUrlInfo mod(i);
//    mod.setName(modName);

//    m_itemList.append(mod);

//    emit listInfo(m_nonce, mod);

    m_itemList.append(i);

    emit listInfo(m_nonce, i);
}

void QFtpWrapper::rawCommandReplyFilter(int replyCode, QString result)
{
    // Capture PWD result.
    qDebug() << "QFtpWrapper::rawCommandReplyFilter" << replyCode << result << "m_lastRawCommand" << m_lastRawCommand;
    if (m_lastRawCommand.toLower() == "pwd") {
        QRegExp rx(".*\"([^\"]+)\".*");
        if (rx.exactMatch(result)) {
            if (rx.captureCount() > 0) {
                m_currentPath = rx.cap(1);
            }
        }
    }

    emit rawCommandReply(m_nonce, replyCode, result);
}

void QFtpWrapper::dataTransferProgressFilter(qint64 done, qint64 total)
{
    if (currentCommand() == QFtp::Get) {
        emit downloadProgress(m_nonce, done, total);
    } else if (currentCommand() == QFtp::Put) {
        emit uploadProgress(m_nonce, done, total);
    }
}

void QFtpWrapper::stateChangedFilter(int state)
{
    emit stateChanged(m_nonce, state);
}

void QFtpWrapper::doneFilter(bool error)
{
    m_isDone = true;

    emit done(m_nonce, error);
}
