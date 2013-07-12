#include "qftpwrapper.h"

const int QFtpWrapper::MaxWaitCount = 150; // 150*100 msec(s) = 15 sec(s).
const int QFtpWrapper::DefaultTimeoutMSec = 15000;

QFtpWrapper::QFtpWrapper(QString nonce, QObject *parent) :
    QFtp(parent)
{
    m_nonce = nonce;
    m_isDone = false;
    m_isAborted = false;
    m_currentPath = "";

    connect(this, SIGNAL(commandStarted(int)), this, SLOT(commandStartedFilter(int)));
    connect(this, SIGNAL(commandFinished(int,bool)), this, SLOT(commandFinishedFilter(int,bool)));
    connect(this, SIGNAL(rawCommandReply(int,QString)), this, SLOT(rawCommandReplyFilter(int,QString)));
    connect(this, SIGNAL(listInfo(QUrlInfo)), this, SLOT(listInfoFilter(QUrlInfo)));
    connect(this, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(dataTransferProgressFilter(qint64,qint64)));
    connect(this, SIGNAL(stateChanged(int)), this, SLOT(stateChangedFilter(int)));
    connect(this, SIGNAL(done(bool)), this, SLOT(doneFilter(bool)));

    // Initialize idle timer.
    m_idleTimer = new QTimer(this);
    connect(m_idleTimer, SIGNAL(timeout()), this, SLOT(idleTimerTimeoutSlot()) );
//    connect(this, SIGNAL(destroyed()), m_idleTimer, SLOT(stop()) );
//    connect(this, SIGNAL(destroyed()), m_idleTimer, SLOT(deleteLater()) );
    m_idleTimer->setInterval(m_settings.value("QFtpWrapper.timeout.interval", QVariant(DefaultTimeoutMSec)).toInt());
    m_idleTimer->setSingleShot(true);
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

QString QFtpWrapper::getUid()
{
    return m_uid;
}

QList<QUrlInfo> QFtpWrapper::getItemList()
{
    return m_itemList;
}

void QFtpWrapper::resetIsDone()
{
    m_isDone = false;
}

bool QFtpWrapper::waitForDone(int waitCount)
{
    qDebug() << "QFtpWrapper::waitForDone" << m_nonce << m_isDone << waitCount;

    int c = waitCount;
    while (!m_isDone && c-- > 0) {
//        qDebug() << "QFtpWrapper::waitForDone" << m_nonce << m_isDone << c << "/" << waitCount;
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }
    qDebug() << "QFtpWrapper::waitForDone" << m_nonce << m_isDone << c << "/" << waitCount;

    // Check isDone then return true if it's actually done.
    if (m_isDone) {
        m_isDone = false;
        return true;
    } else {
        return false;
    }
}

bool QFtpWrapper::deleteRecursive(QString remoteFilePath)
{
    if (remoteFilePath == "") {
        remoteFilePath = m_remoteFilePath;
    }

    qDebug() << "QFtpWrapper::deleteRecursive" << m_nonce << remoteFilePath;

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
            qDebug() << "QFtpWrapper::deleteRecursive" << m_nonce << "item" << remoteFilePath + "/" + itemList.at(i).name() << "isDir" << itemList.at(i).isDir();
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
                    qDebug() << "QFtpWrapper::deleteRecursive" << m_nonce << "can't delete file" << remoteFilePath + "/" + itemList.at(i).name() << error() << errorString();
                    emit deleteRecursiveFinished(m_nonce, error(), errorString());
                    return false;
                } else {
                    qDebug() << "QFtpWrapper::deleteRecursive" << m_nonce << "delete file" << remoteFilePath + "/" + itemList.at(i).name();
                }
            }
        }

        // Delete parent folder.
        cd("..");
        rmdir(remoteFilePath);
        waitForDone();
        if (error() != QFtp::NoError) {
            qDebug() << "QFtpWrapper::deleteRecursive" << m_nonce << "can't delete folder" << remoteFilePath << error() << errorString();
            emit deleteRecursiveFinished(m_nonce, error(), errorString());
            return false;
        } else {
            qDebug() << "QFtpWrapper::deleteRecursive" << m_nonce << "delete folder" << remoteFilePath;
        }
    } else {
        // It's a file. Delete it.
        remove(remoteFilePath);
        waitForDone();
        if (error() != QFtp::NoError) {
            qDebug() << "QFtpWrapper::deleteRecursive" << m_nonce << "can't delete file" << remoteFilePath << error() << errorString();
            emit deleteRecursiveFinished(m_nonce, error(), errorString());
            return false;
        } else {
            qDebug() << "QFtpWrapper::deleteRecursive" << m_nonce << "delete file" << remoteFilePath;
        }
    }

    emit deleteRecursiveFinished(m_nonce, error(), errorString());
    return true;
}

QString QFtpWrapper::getCommandName(Command cmd)
{
    switch (cmd) {
    case QFtp::None: return "None";
    case QFtp::SetTransferMode: return "SetTransferMode";
    case QFtp::SetProxy: return "SetProxy";
    case QFtp::ConnectToHost: return "ConnectToHost";
    case QFtp::Login: return "Login";
    case QFtp::Close: return "Close";
    case QFtp::List: return "List";
    case QFtp::Cd: return "Cd";
    case QFtp::Get: return "Get";
    case QFtp::Put: return "Put";
    case QFtp::Remove: return "Remove";
    case QFtp::Mkdir: return "Mkdir";
    case QFtp::Rmdir: return "Rmdir";
    case QFtp::Rename: return "Rename";
    case QFtp::RawCommand: return "RawCommand";
    default:
        return "Invalid";
    }
}

bool QFtpWrapper::isAborted()
{
    return m_isAborted;
}

void QFtpWrapper::setIsAborted(bool flag)
{
//    qDebug() << "QFtpWrapper::setIsAborted" << flag << sender();
    m_isAborted = flag;
}

void QFtpWrapper::commandStartedFilter(int id)
{
    qDebug() << "QFtpWrapper::commandStartedFilter" << m_nonce << id << "currentCommand" << currentCommand() << getCommandName(currentCommand());

    m_isDone = false;
    m_itemList.clear();

    emit commandStarted(m_nonce, id);

    // Restart idle timer once command started.
    m_idleTimer->start();
}

void QFtpWrapper::commandFinishedFilter(int id, bool error)
{
    qDebug() << "QFtpWrapper::commandFinishedFilter" << m_nonce << id << (error ? (this->error() + " " + this->errorString()) : "");

    emit commandFinished(m_nonce, id, error);

    // Stop idle timer once command finished.
    m_idleTimer->stop();
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

    qDebug() << "QFtpWrapper::listInfoFilter" << m_nonce << "name" << i.name() << "isDir" << i.isDir();

    m_itemList.append(i);

    emit listInfo(m_nonce, i);
}

void QFtpWrapper::rawCommandReplyFilter(int replyCode, QString result)
{
    // Capture PWD result.
    qDebug() << "QFtpWrapper::rawCommandReplyFilter" << m_nonce << replyCode << result << "m_lastRawCommand" << m_lastRawCommand;
    if (m_lastRawCommand.toLower() == "pwd") {
        QRegExp rx(".*\"([^\"]+)\".*");
        if (rx.exactMatch(result)) {
            if (rx.captureCount() > 0) {
                m_currentPath = rx.cap(1);
            }
        }
        qDebug() << "QFtpWrapper::rawCommandReplyFilter" << m_nonce << "m_currentPath" << m_currentPath;
    }

    emit rawCommandReply(m_nonce, replyCode, result);
}

void QFtpWrapper::dataTransferProgressFilter(qint64 done, qint64 total)
{
//    qDebug() << "QFtpWrapper::dataTransferProgressFilter" << currentCommand() << done << total;
    if (currentCommand() == QFtp::Get) {
        emit downloadProgress(m_nonce, done, total);
    } else if (currentCommand() == QFtp::Put) {
        emit uploadProgress(m_nonce, done, total);
    }

    // Restart idle timer if there is any progress.
    m_idleTimer->start();
}

void QFtpWrapper::stateChangedFilter(int state)
{
    emit stateChanged(m_nonce, state);
}

void QFtpWrapper::doneFilter(bool error)
{
    qDebug() << "QFtpWrapper::doneFilter" << m_nonce << (error ? "error" : "");

    m_isDone = true;

    emit done(m_nonce, error);
}

void QFtpWrapper::idleTimerTimeoutSlot()
{
    // Abort connection if it's timeout.
    abort();
    setIsAborted(true);
    m_isDone = true;
    qDebug() << "QFtpWrapper::idleTimerTimeoutSlot nonce" << m_nonce << "is aborted.";
}
