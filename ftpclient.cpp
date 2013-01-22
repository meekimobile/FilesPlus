#include "ftpclient.h"
#include <QApplication>
//#include <QtNetwork>
//#include "qnetworkreplywrapper.h"

const QString FtpClient::FtpRoot = "~";

FtpClient::FtpClient(QObject *parent) :
    CloudDriveClient(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Load accessTokenPair from file
    loadAccessPairMap();

    m_ftpHash = new QHash<QString, QFtpWrapper*>();
}

FtpClient::~FtpClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

    m_ftpHash = 0;
}

void FtpClient::quota(QString nonce, QString uid)
{
    // Signal with empty message to get default quota value.
    emit quotaReplySignal(nonce, 0, "", "{ }");
}

void FtpClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath)
{
    qDebug() << "----- FtpClient::fileGet -----" << uid << remoteFilePath << localFilePath;

    QString remoteParentPath = getParentRemotePath(remoteFilePath);

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    m_ftp->m_remoteFilePath = remoteFilePath;
    m_ftp->m_localFilePath = localFilePath;
    connect(m_ftp, SIGNAL(done(QString,bool)), this, SLOT(fileGetReplyFinished(QString,bool)) );

    m_ftp->cd(remoteParentPath);

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localTargetFile = m_localFileHash[nonce];
    if (localTargetFile->open(QIODevice::WriteOnly)) {
        m_ftp->get(getRemoteFileName(remoteFilePath), localTargetFile);
    }
}

/*
void FtpClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::filePut -----" << localFilePath << "to" << remoteFilePath;

    QStringList tokens = accessTokenPairMap[uid].token.split("@");

    QUrl uploadurl("ftp://" + tokens[1] + remoteFilePath);
    uploadurl.setUserName(tokens[0]);
    uploadurl.setPassword(accessTokenPairMap[uid].secret);
    qDebug() << "FtpClient::filePut uploadurl" << uploadurl;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
//        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
        QNetworkRequest req = QNetworkRequest(uploadurl);
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setHeader(QNetworkRequest::ContentLengthHeader, fileSize);
        QNetworkReply *reply = manager->put(req, localSourceFile);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

        while (!reply->isFinished()) {
            QApplication::processEvents(QEventLoop::AllEvents, 100);
            Sleeper::msleep(100);
        }

        emit filePutReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

        // Scheduled to delete later.
        reply->deleteLater();
        reply->manager()->deleteLater();
    } else {
        emit filePutReplySignal(nonce, -1, tr("Can't open %1").arg(localFilePath), "");
    }
}
*/

void FtpClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::filePut -----" << uid << localFilePath << remoteFilePath;

    QString remoteParentPath = getParentRemotePath(remoteFilePath);

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    m_ftp->m_localFilePath = localFilePath;
    m_ftp->m_remoteFilePath = remoteFilePath;
    connect(m_ftp, SIGNAL(done(QString,bool)), this, SLOT(filePutReplyFinished(QString,bool)) );

    m_ftp->cd(remoteParentPath);

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        m_ftp->put(localSourceFile, getRemoteFileName(remoteFilePath));
        m_ftp->waitForDone();
    }
}

QString FtpClient::property(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::property -----" << uid << remoteFilePath;

    QString propertyJson = "";
    QString remoteParentPath = getParentRemotePath(remoteFilePath);
    QString remoteFileName = getRemoteFileName(remoteFilePath);

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    m_ftp->list(remoteParentPath);
    m_ftp->waitForDone();

    if (m_ftp->error() == QFtp::NoError) {
        if (m_ftp->getItemList().isEmpty()) {
            // remoteFilePath is not found.
            qDebug() << "FtpClient::property" << uid << remoteFilePath << "is not found.";
        } else {
            for (int i=0; i < m_ftp->getItemList().count(); i++) {
//                qDebug() << "FtpClient::property item" << m_ftp->getItemList().at(i).name();
                if (m_ftp->getItemList().at(i).name() == remoteFileName) {
                    propertyJson = getPropertyJson(remoteParentPath, m_ftp->getItemList().at(i));
                    break;
                }
            }
        }
    } else {
        qDebug() << "FtpClient::property" << uid << remoteFilePath << "error" << m_ftp->error() << m_ftp->errorString();
        propertyJson = QString("{ \"error\": %1, \"errorString\": \"%2\" }").arg(m_ftp->error()).arg(m_ftp->errorString());
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    return propertyJson;
}

void FtpClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::metadata -----" << uid << remoteFilePath;

    QString propertyJson = property(nonce, uid, remoteFilePath);
    if (propertyJson.isEmpty()) {
        qDebug() << "FtpClient::metadata" << uid << remoteFilePath << "is not found.";
        emit metadataReplySignal(nonce, QNetworkReply::ContentNotFoundError, tr("%1 is not found.").arg(remoteFilePath), "");
        return;
    }

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

    // Check if remoteFilePath is dir or file by change directory.
    m_ftp->cd(remoteFilePath);
    m_ftp->waitForDone();

    if (m_ftp->error() == QFtp::NoError) {
        // remoteFilePath is dir.
        qDebug() << "FtpClient::metadata" << uid << remoteFilePath << " is dir.";
        m_ftp->pwd();
        m_ftp->list();
        m_ftp->waitForDone();

        emit metadataReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"data\": %1, \"property\": %2 }").arg(getItemListJson(m_ftp->getCurrentPath(), m_ftp->getItemList())).arg(propertyJson) );
    } else {
        // remoteFilePath is file or not found.
        qDebug() << "FtpClient::metadata" << uid << remoteFilePath << "is file or not found. error" << m_ftp->error() << m_ftp->errorString();
        m_ftp->list(remoteFilePath);
        m_ftp->waitForDone();

        if (m_ftp->getItemList().isEmpty()) {
            // remoteFilePath is not found.
            qDebug() << "FtpClient::metadata" << uid << remoteFilePath << "is not found.";
            emit metadataReplySignal(m_ftp->getNonce(), QNetworkReply::ContentNotFoundError, tr("%1 is not found.").arg(remoteFilePath), "");
        } else {
            // remoteFilePath is file.
            QString remoteParentPath = getParentRemotePath(remoteFilePath);
            qDebug() << "FtpClient::metadata" << uid << remoteFilePath << "is a file. remoteParentPath" << remoteParentPath << "remoteFileName" << m_ftp->getItemList().first().name();
            emit metadataReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"data\": [], \"property\": %1 }").arg(getPropertyJson(remoteParentPath, m_ftp->getItemList().first())) );
        }
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::browse -----" << uid << remoteFilePath;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

    // Get item list.
    if (!remoteFilePath.isEmpty()) m_ftp->cd(remoteFilePath);
    m_ftp->pwd();
    m_ftp->list(remoteFilePath);
    m_ftp->waitForDone();

    emit browseReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), QString("{ \"data\": %1, \"property\": %2 }").arg(getItemListJson(m_ftp->getCurrentPath(), m_ftp->getItemList())).arg(QString("{ \"path\": \"%1\", \"isDir\": true }").arg(m_ftp->getCurrentPath())) );

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName)
{
    createFolder(nonce, uid, remoteParentPath, newRemoteFolderName, false);
}

void FtpClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    emit moveFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, tr("FTP doesn't support move command."), "");
}

void FtpClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    emit copyFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, tr("FTP doesn't support copy command."), "");
}

void FtpClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::deleteFile -----" << uid << remoteFilePath;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

    deleteRecursive(m_ftp, remoteFilePath);

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::shareFile -----" << uid << remoteFilePath;

    emit shareFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, "FTP doesn't support resource link sharing.", "");
}

QString FtpClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- FtpClient::createFolder -----" << uid << remoteParentPath << newRemoteFolderName;

    if (remoteParentPath.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return "";
    }

    QString newRemoteFolderPath = remoteParentPath + "/" + newRemoteFolderName;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

    m_ftp->mkdir(newRemoteFolderPath);
    m_ftp->waitForDone();

    QString result;
    if (m_ftp->error() != QFtp::NoError) {
        // NOTE json string doesn't support newline character.
        QString escapedErrorString =  m_ftp->errorString().replace("\n", " ");
        emit createFolderReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, m_ftp->errorString(), QString("{ \"error\": \"%1\", \"path\": \"%2\" }").arg(escapedErrorString).arg(newRemoteFolderPath) );
    } else {
        // Get property.
        QString propertyJson = property(nonce, uid, newRemoteFolderPath);
        if (propertyJson.isEmpty()) {
            qDebug() << "FtpClient::createFolder" << uid << newRemoteFolderPath << "is not found.";
            emit createFolderReplySignal(nonce, QNetworkReply::ContentNotFoundError, "Created path is not found.", QString("{ \"error\": \"Created path is not found.\", \"path\": \"%1\" }").arg(newRemoteFolderPath) );
        } else {
            result = propertyJson;
            emit createFolderReplySignal(nonce, QNetworkReply::NoError, "", result);
        }
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    return result;
}

QIODevice *FtpClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- FtpClient::fileGet -----" << uid << remoteFilePath << "synchronous" << synchronous;

    QString remoteParentPath = getParentRemotePath(remoteFilePath);

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    m_ftp->m_remoteFilePath = remoteFilePath;
    m_ftp->cd(remoteParentPath);

    // Creat buffer as QIODevice.
    QBuffer *buf = new QBuffer();
    if (buf->open(QIODevice::WriteOnly)) {
        m_ftp->get(getRemoteFileName(remoteFilePath), buf);
        m_ftp->waitForDone();
        buf->close();
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    return buf;
}

QNetworkReply *FtpClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- FtpClient::filePut -----" << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    m_ftp->m_remoteFilePath = remoteParentPath + "/" + remoteFileName;
    m_ftp->cd(remoteParentPath);

    m_ftp->put(source, remoteFileName);
    m_ftp->waitForDone();
    source->close();

    // TODO Get uploaded file property.
    m_ftp->list(remoteParentPath + "/" + remoteFileName);
    m_ftp->waitForDone();

    if (m_ftp->getItemList().isEmpty()) {
        // remoteFilePath is not found.
        qDebug() << "FtpClient::filePut" << uid << remoteParentPath + "/" + remoteFileName << "is not found.";
        emit migrateFilePutReplySignal(m_ftp->getNonce(), -1, tr("Can't put %1").arg(remoteParentPath + "/" + remoteFileName), "");
    } else {
        qDebug() << "FtpClient::filePut" << uid << remoteParentPath + "/" + remoteFileName << "is a file. remoteParentPath" << remoteParentPath << "remoteFileName" << m_ftp->getItemList().first().name();
        emit migrateFilePutReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(remoteParentPath, m_ftp->getItemList().first()) );
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    return 0;
}

QFtpWrapper *FtpClient::connectToHost(QString nonce, QString uid)
{
    /* Notes:
     * Stores token as user@host --> Token(token=user@host, secret=password, email=user@host)
     */
    QString url = accessTokenPairMap[uid].token;
    QString username, hostname;
    quint16 port = 21; // Default ftp port = 21
    QRegExp rx("([^@]*)@([^:]*)(:.*)*");
    if (rx.exactMatch(url)) {
        qDebug() << "FtpClient::connectToHost" << rx.captureCount() << rx.capturedTexts();
        if (rx.captureCount() >= 2) {
            if (rx.captureCount() == 3) {
                // Skip : by start at position 1.
                port = rx.cap(3).mid(1).toInt();
            }
            username = rx.cap(1);
            hostname = rx.cap(2);
        }
    }
    QString password = accessTokenPairMap[uid].secret;
    qDebug() << "FtpClient::connectToHost" << hostname << port << username << password;

    QFtpWrapper *m_ftp = new QFtpWrapper(nonce, this);
    m_ftp->m_uid = uid;
    connect(m_ftp, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(m_ftp, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    m_ftp->abort();
    m_ftp->connectToHost(hostname, port);
    m_ftp->login(username, password);

    // Insert to hash.
    m_ftpHash->insert(nonce, m_ftp);

    return m_ftp;
}

QString FtpClient::getPropertyJson(const QString parentPath, const QUrlInfo item)
{
    QString jsonText;

    // TODO handle absolute path in name.
    QString path = (item.name().startsWith("/") ? "" : parentPath + "/") + item.name();
    jsonText = QString("{ \"name\": \"%1\", \"path\": \"%2\", \"lastModified\": \"%3\", \"size\": %4, \"isDir\": %5 }")
            .arg(item.name())
            .arg(path)
            .arg(item.lastModified().toUTC().toString(Qt::ISODate))
            .arg(item.size())
            .arg(item.isDir());

    return jsonText;
}

QString FtpClient::getItemListJson(const QString parentPath, const QList<QUrlInfo> itemList)
{
    QString dataJsonText;
    QUrlInfo item;
    for (int i = 0; i < itemList.count(); i++) {
        item = itemList.at(i);
        if (dataJsonText.length() > 0) {
            dataJsonText.append(", ");
        }
        dataJsonText.append(getPropertyJson(parentPath, item));
    }
    dataJsonText.prepend("[ ").append(" ]");

    return dataJsonText;
}

QString FtpClient::getParentRemotePath(QString remotePath)
{
    QString remoteParentPath = "";
    if (remotePath != "") {
        remoteParentPath = remotePath.mid(0, remotePath.lastIndexOf("/"));
    }

    return remoteParentPath;
}

QString FtpClient::getRemoteFileName(QString remotePath)
{
    QString name = "";
    if (remotePath != "") {
        name = remotePath.mid(remotePath.lastIndexOf("/") + 1);
    }

    return name;
}

bool FtpClient::testConnection(QString hostname, quint16 port, QString username, QString password)
{
    qDebug() << "----- FtpClient::testConnection -----";

    // Reset m_isDone.
//    m_isDone = false;

    bool res = false;
    QFtpWrapper *m_ftp = new QFtpWrapper("test", this);
    m_ftp->connectToHost(hostname, port);
    m_ftp->login(username, password);
    m_ftp->waitForDone();
    qDebug() << "FtpClient::testConnection done state" << m_ftp->state();

    res = (m_ftp->state() == QFtp::LoggedIn);

    m_ftp->close();
    m_ftp->deleteLater();

    return res;
}

void FtpClient::saveConnection(QString id, QString hostname, quint16 port, QString username, QString password)
{
    qDebug() << "----- FtpClient::saveConnection -----";

    /* Notes:
     * Stores token as user@host --> Token(token=user@host, secret=password, email=user@host)
     */
    // TODO Encrypt password before store to file.
    TokenPair tokenPair;
    tokenPair.token = QString("%1@%2:%3").arg(username).arg(hostname).arg(port);
    tokenPair.secret = password;
    tokenPair.email = QString("%1@%2:%3").arg(username).arg(hostname).arg(port);
    accessTokenPairMap[id] = tokenPair;

    saveAccessPairMap();
}

bool FtpClient::isRemoteAbsolutePath()
{
    return true;
}

bool FtpClient::deleteRecursive(QFtpWrapper *m_ftp, QString remoteFilePath)
{
    if (remoteFilePath == "") {
        emit deleteFileReplySignal(m_ftp->getNonce(),-1, tr("Specified remote path is empty."), QString("{ \"path\": \"%1\" }").arg(remoteFilePath));
        return false;
    }

    qDebug() << "FtpClient::deleteRecursive" << remoteFilePath;

    // Check if it's folder by cd.
    m_ftp->cd(remoteFilePath);
    m_ftp->waitForDone();
    if (m_ftp->error() == QFtp::NoError) {
        // It's folder. Drill it down.
        m_ftp->list();
        m_ftp->waitForDone();
        // ISSUE list() get clear before start listing. When pop to parent level, the list is empty.
        QList<QUrlInfo> itemList = m_ftp->getItemList();
        for (int i = 0; i < itemList.count(); i++) {
            qDebug() << "FtpClient::deleteRecursive item" << remoteFilePath + "/" + itemList.at(i).name() << "isDir" << itemList.at(i).isDir();
            if (itemList.at(i).isDir()) {
                // Drill down into folder.
                bool res = deleteRecursive(m_ftp, remoteFilePath + "/" + itemList.at(i).name());
                if (!res) {
                    return false;
                }
            } else {
                // Delete a file.
                m_ftp->remove(itemList.at(i).name());
                m_ftp->waitForDone();
                if (m_ftp->error() != QFtp::NoError) {
                    qDebug() << "FtpClient::deleteRecursive can't delete file" << remoteFilePath + "/" + itemList.at(i).name() << m_ftp->error() << m_ftp->errorString();
                    emit deleteFileReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath));
                    return false;
                } else {
                    qDebug() << "FtpClient::deleteRecursive delete file" << remoteFilePath + "/" + itemList.at(i).name();
                    emit deleteFileReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath));
                }
            }
        }

        // Delete parent folder.
        m_ftp->cd("..");
        m_ftp->rmdir(remoteFilePath);
        m_ftp->waitForDone();
        if (m_ftp->error() != QFtp::NoError) {
            qDebug() << "FtpClient::deleteRecursive can't delete folder" << remoteFilePath << m_ftp->error() << m_ftp->errorString();
            emit deleteFileReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath));
            return false;
        } else {
            qDebug() << "FtpClient::deleteRecursive delete folder" << remoteFilePath;
        }
    } else {
        // It's a file. Delete it.
        m_ftp->remove(remoteFilePath);
        m_ftp->waitForDone();
        if (m_ftp->error() != QFtp::NoError) {
            qDebug() << "FtpClient::deleteRecursive can't delete file" << remoteFilePath << m_ftp->error() << m_ftp->errorString();
            emit deleteFileReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath));
            return false;
        } else {
            qDebug() << "FtpClient::deleteRecursive delete file" << remoteFilePath;
        }
    }

    // Emit signal for successful deleting.
    emit deleteFileReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath));
    return true;
}

void FtpClient::deleteFileReplyFinished(QString nonce, int error, QString errorString)
{
    qDebug() << "FtpClient::deleteFileReplyFinished" << nonce << error;

    QFtpWrapper *m_ftp = m_ftpHash->value(nonce);

    emit deleteFileReplySignal(nonce, error, errorString, QString("{ \"path\": \"%1\" }").arg(m_ftp->m_remoteFilePath) );

//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(nonce);
}

void FtpClient::fileGetReplyFinished(QString nonce, bool error)
{
    // Get stored parameters and clean ftp object.
    QFtpWrapper *m_ftp = m_ftpHash->value(nonce);
    QString uid = m_ftp->m_uid;
    QString remoteFilePath = m_ftp->m_remoteFilePath;
    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    if (!error) {
        // Connect and get uploaded file property.
        m_ftp = connectToHost(nonce, uid);
        m_ftp->list(remoteFilePath);
        m_ftp->waitForDone();

        if (m_ftp->getItemList().isEmpty()) {
            // remoteFilePath is not found.
            qDebug() << "FtpClient::fileGetReplyFinished" << uid << remoteFilePath << "is not found.";
            emit fileGetReplySignal(m_ftp->getNonce(), -1, tr("Can't get %1").arg(remoteFilePath), "");
        } else {
            qDebug() << "FtpClient::fileGetReplyFinished" << m_ftp->m_uid << m_ftp->m_remoteFilePath << "is a file.";
            emit fileGetReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(getParentRemotePath(remoteFilePath), m_ftp->getItemList().first()) );
        }
    } else {
        emit fileGetReplySignal(m_ftp->getNonce(), m_ftp->error(), tr("Can't get %1").arg(remoteFilePath), m_ftp->errorString());
    }

    // Close and remove localFile pointer.
    if (m_localFileHash.contains(nonce)) {
        m_localFileHash[nonce]->close();
        m_localFileHash.remove(nonce);
    }

    // Clean up.
    m_ftp->close();
    m_ftp->deleteLater();
}

void FtpClient::filePutReplyFinished(QString nonce, bool error)
{
    // Get stored parameters and clean ftp object.
    QFtpWrapper *m_ftp = m_ftpHash->value(nonce);
    QString uid = m_ftp->m_uid;
    QString remoteFilePath = m_ftp->m_remoteFilePath;
    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    if (!error) {
        // Connect and get uploaded file property.
        m_ftp = connectToHost(nonce, uid);
        m_ftp->list(remoteFilePath);
        m_ftp->waitForDone();

        if (m_ftp->getItemList().isEmpty()) {
            // remoteFilePath is not found.
            qDebug() << "FtpClient::filePutReplyFinished" << uid << remoteFilePath << "is not found.";
            emit filePutReplySignal(m_ftp->getNonce(), -1, tr("Can't put %1").arg(remoteFilePath), "");
        } else {
            qDebug() << "FtpClient::filePutReplyFinished" << uid << remoteFilePath << "is a file.";
            emit filePutReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(getParentRemotePath(remoteFilePath), m_ftp->getItemList().first()) );
        }
    } else {
        emit filePutReplySignal(m_ftp->getNonce(), m_ftp->error(), tr("Can't put %1").arg(remoteFilePath), m_ftp->errorString());
    }

    // Close and remove localFile pointer.
    if (m_localFileHash.contains(nonce)) {
        m_localFileHash[nonce]->close();
        m_localFileHash.remove(nonce);
    }

    // Clean up.
    m_ftp->close();
    m_ftp->deleteLater();
}
