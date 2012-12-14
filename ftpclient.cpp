#include "ftpclient.h"

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString FtpClient::KeyStoreFilePath = "/home/user/.filesplus/FtpClient.dat";
#else
const QString FtpClient::KeyStoreFilePath = "C:/FtpClient.dat";
#endif
const QString FtpClient::consumerKey = "";
const QString FtpClient::consumerSecret = "";
const QString FtpClient::clientTypeName = "FtpClient";
const QString FtpClient::FtpRoot = "~";

FtpClient::FtpClient(QObject *parent) :
    QObject(parent)
{
    // Load accessTokenPair from file
    loadAccessPairMap();

//    m_itemList = new QList<QUrlInfo>();

//    m_isDone = true;

    m_ftpHash = new QHash<QString, QFtpWrapper*>();
}

FtpClient::~FtpClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

//    m_itemList = 0;

    m_ftpHash = 0;
}

bool FtpClient::isAuthorized()
{
    return (!accessTokenPairMap.isEmpty());
}

QStringList FtpClient::getStoredUidList()
{
    QStringList list;
    foreach (QString s, accessTokenPairMap.keys()) {
        TokenPair t = accessTokenPairMap[s];

        QString jsonText = "{ ";
        jsonText.append( QString("\"uid\": \"%1\", ").arg(s) );
        jsonText.append( QString("\"email\": \"%1\", ").arg(t.email) );
        jsonText.append( QString("\"type\": \"%1\"").arg(clientTypeName) );
        jsonText.append(" }");

        list.append(jsonText);
    }
    return list;
}

int FtpClient::removeUid(QString uid)
{
    qDebug() << "FtpClient::removeUid uid" << uid;
    int n = accessTokenPairMap.remove(uid);
    qDebug() << "FtpClient::removeUid accessTokenPairMap" << accessTokenPairMap;

    return n;
}

//void FtpClient::commandStarted(int id)
//{
//    qDebug() << "FtpClient::commandStarted" << id << "currentCommand" << m_ftp->currentCommand();
//}

//void FtpClient::commandFinished(int id, bool error)
//{
//    qDebug() << "FtpClient::commandFinished" << id << "error" << error;
//}

//void FtpClient::addToList(const QUrlInfo &i)
//{
//    qDebug() << "FtpClient::addToList" << i.name() << i.isDir() << i.isFile() << i.lastModified();
//    m_itemList->append(i);
//}

//void FtpClient::rawCommandReplyFinished(int replyCode, QString result)
//{
//    // Capture PWD result.
//    qDebug() << "FtpClient::rawCommandReplyFinished" << replyCode << result;
//    QRegExp rx(".*\"([^\"]+)\".*");
//    if (rx.exactMatch(result)) {
//        qDebug() << "FtpClient::rawCommandReplyFinished rx.capturedTexts()" << rx.capturedTexts();
//        if (rx.captureCount() > 0) {
//            m_currentPath = rx.cap(1);
//        }
//    }
//}

//void FtpClient::uploadProgressFilter(qint64 done, qint64 total)
//{
//    qDebug() << "FtpClient::uploadProgressFilter" << done << total;

//    emit uploadProgress(m_currentNonce, done, total);
//}

//void FtpClient::downloadProgressFilter(qint64 done, qint64 total)
//{
//    qDebug() << "FtpClient::downloadProgressFilter" << done << total;

//    emit downloadProgress(m_currentNonce, done, total);
//}

//void FtpClient::stateChangedFilter(int state)
//{
//    qDebug() << "FtpClient::stateChangedFilter" << state;
//}

//void FtpClient::doneFilter(bool error)
//{
//    qDebug() << "FtpClient::doneFilter" << error;
//    m_isDone = true;
//}

void FtpClient::loadAccessPairMap() {
    QFile file(KeyStoreFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> accessTokenPairMap;

        qDebug() << QTime::currentTime() << "FtpClient::loadAccessPairMap " << accessTokenPairMap;
    }
}

void FtpClient::saveAccessPairMap() {
    // TODO workaround fix to remove tokenPair with key="".
    accessTokenPairMap.remove("");

    // TODO To prevent invalid code to save damage data for testing only.
//    if (accessTokenPairMap.isEmpty()) return;

    QFile file(KeyStoreFilePath);
    QFileInfo info(file);
    if (!info.absoluteDir().exists()) {
        qDebug() << "FtpClient::saveAccessPairMap dir" << info.absoluteDir().absolutePath() << "doesn't exists.";
        bool res = QDir::home().mkpath(info.absolutePath());
        if (!res) {
            qDebug() << "FtpClient::saveAccessPairMap can't make dir" << info.absolutePath();
        } else {
            qDebug() << "FtpClient::saveAccessPairMap make dir" << info.absolutePath();
        }
    }    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << accessTokenPairMap;

        qDebug() << "FtpClient::saveAccessPairMap " << accessTokenPairMap;
    }
}

void FtpClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath)
{
    qDebug() << "FtpClient::fileGet" << uid << remoteFilePath << localFilePath;

    QString remoteParentPath = getParentRemotePath(remoteFilePath);

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    m_ftp->cd(remoteParentPath);

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localTargetFile = m_localFileHash[nonce];
    if (localTargetFile->open(QIODevice::WriteOnly)) {
        m_ftp->get(getRemoteFileName(remoteFilePath), localTargetFile);
        m_ftp->waitForDone();

        // TODO Get uploaded file property.
        m_ftp->list(remoteFilePath);
        m_ftp->waitForDone();
    }

    localTargetFile->close();
    m_localFileHash.remove(nonce);

    if (m_ftp->getItemList().isEmpty()) {
        // remoteFilePath is not found.
        qDebug() << "FtpClient::fileGet" << uid << remoteFilePath << "is not found.";
        emit fileGetReplySignal(m_ftp->getNonce(), -1, tr("Can't get %1").arg(remoteFilePath), "");
    } else {
        qDebug() << "FtpClient::fileGet" << uid << remoteFilePath << localFilePath << "is a file. remoteParentPath" << remoteParentPath << "remoteFileName" << m_ftp->getItemList().first().name();
        emit fileGetReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(remoteParentPath, m_ftp->getItemList().first()) );
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "FtpClient::filePut" << uid << localFilePath << remoteFilePath;

    QString remoteParentPath = getParentRemotePath(remoteFilePath);

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    m_ftp->cd(remoteParentPath);

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        m_ftp->put(localSourceFile, getRemoteFileName(remoteFilePath));
        m_ftp->waitForDone();

        // TODO Get uploaded file property.
        m_ftp->list(remoteFilePath);
        m_ftp->waitForDone();
    }

    localSourceFile->close();
    m_localFileHash.remove(nonce);

    if (m_ftp->getItemList().isEmpty()) {
        // remoteFilePath is not found.
        qDebug() << "FtpClient::filePut" << uid << remoteFilePath << "is not found.";
        emit filePutReplySignal(m_ftp->getNonce(), -1, tr("Can't put %1").arg(remoteFilePath), "");
    } else {
        qDebug() << "FtpClient::filePut" << uid << localFilePath << remoteFilePath << "is a file. remoteParentPath" << remoteParentPath << "remoteFileName" << m_ftp->getItemList().first().name();
        emit filePutReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(remoteParentPath, m_ftp->getItemList().first()) );
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

QString FtpClient::property(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "FtpClient::property" << uid << remoteFilePath;

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
    qDebug() << "FtpClient::metadata" << uid << remoteFilePath;

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
    qDebug() << "FtpClient::browse" << uid << remoteFilePath;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

    // Get item list.
//    m_itemList->clear();
    if (!remoteFilePath.isEmpty()) m_ftp->cd(remoteFilePath);
    m_ftp->pwd();
    m_ftp->list(remoteFilePath);
    m_ftp->waitForDone();

    emit browseReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), QString("{ \"data\": %1, \"property\": %2 }").arg(getItemListJson(m_ftp->getCurrentPath(), m_ftp->getItemList())).arg(QString("{ \"path\": \"%1\", \"isDir\": true }").arg(m_ftp->getCurrentPath())) );

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::createFolder(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "FtpClient::createFolder" << uid << localFilePath << remoteFilePath;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

    m_ftp->mkdir(remoteFilePath);
    m_ftp->waitForDone();

    if (m_ftp->error() == QFtp::UnknownError) {
        emit createFolderReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath) );
    } else {
        // Get property.
        QString propertyJson = property(nonce, uid, remoteFilePath);
        if (propertyJson.isEmpty()) {
            qDebug() << "FtpClient::createFolder" << uid << remoteFilePath << "is not found.";
            emit createFolderReplySignal(nonce, QNetworkReply::ContentNotFoundError, tr("%1 is not found.").arg(remoteFilePath), "");
        } else {
            emit createFolderReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), propertyJson);
        }
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "FtpClient::deleteFile" << uid << remoteFilePath;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

    deleteRecursive(m_ftp, remoteFilePath);

    emit deleteFileReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath) );

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "FtpClient::shareFile" << uid << remoteFilePath;

    emit shareFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, "FTP doesn't support resource link sharing.", "");
}

bool FtpClient::deleteRecursive(QFtpWrapper *ftp, QString remoteFilePath)
{
    qDebug() << "FtpClient::deleteRecursive" << remoteFilePath;

    if (!ftp) return false;

    // Check if it's folder by cd.
    ftp->cd(remoteFilePath);
    ftp->waitForDone();
    if (ftp->error() == QFtp::NoError) {
        // It's folder. Drill it down.
        ftp->list();
        ftp->waitForDone();
        // ISSUE ftp->list() get clear before start listing. When pop to parent level, the list is empty.
        QList<QUrlInfo> itemList = ftp->getItemList();
        for (int i = 0; i < itemList.count(); i++) {
            qDebug() << "FtpClient::deleteRecursive item" << remoteFilePath + "/" + itemList.at(i).name() << "isDir" << itemList.at(i).isDir();
            if (itemList.at(i).isDir()) {
                // Drill down into folder.
                bool res = deleteRecursive(ftp, remoteFilePath + "/" + itemList.at(i).name());
                if (!res) return false;
            } else {
                // Delete a file.
                ftp->remove(itemList.at(i).name());
                ftp->waitForDone();
                if (ftp->error() != QFtp::NoError) {
                    qDebug() << "FtpClient::deleteRecursive can't delete file" << remoteFilePath + "/" + itemList.at(i).name() << ftp->error() << ftp->errorString();
                    return false;
                } else {
                    qDebug() << "FtpClient::deleteRecursive delete file" << remoteFilePath + "/" + itemList.at(i).name();
                }
            }
        }

        // Delete parent folder.
        ftp->cd("..");
        ftp->rmdir(remoteFilePath);
        ftp->waitForDone();
        if (ftp->error() != QFtp::NoError) {
            qDebug() << "FtpClient::deleteRecursive can't delete folder" << remoteFilePath << ftp->error() << ftp->errorString();
            return false;
        } else {
            qDebug() << "FtpClient::deleteRecursive delete folder" << remoteFilePath;
        }
    } else {
        // It's a file. Delete it.
        ftp->remove(remoteFilePath);
        ftp->waitForDone();
        if (ftp->error() != QFtp::NoError) {
            qDebug() << "FtpClient::deleteRecursive can't delete file" << remoteFilePath << ftp->error() << ftp->errorString();
            return false;
        } else {
            qDebug() << "FtpClient::deleteRecursive delete file" << remoteFilePath;
        }
    }

    return true;
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
    connect(m_ftp, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(m_ftp, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    m_ftp->abort();
    m_ftp->connectToHost(hostname, port);
    m_ftp->login(username, password);

    // Insert to hash.
    m_ftpHash->insert(nonce, m_ftp);

    return m_ftp;
}

void FtpClient::saveConnection(QString id, QString hostname, quint16 port, QString username, QString password)
{
    /* Notes:
     * Stores token as user@host --> Token(token=user@host, secret=password, email=user@host)
     */
    // TODO Encrypt password before store to file.
    TokenPair tokenPair;
    tokenPair.token = QString("%1@%2:%3").arg(username).arg(hostname).arg(port);
    tokenPair.secret = password;
    tokenPair.email = QString("%1@%2").arg(username).arg(hostname);
    accessTokenPairMap[id] = tokenPair;

    saveAccessPairMap();
}

//void FtpClient::waitForDone()
//{
//    int c = 100;
//    while (!m_isDone && c-- > 0) {
//        qDebug() << "FtpClient::waitForDone" << m_isDone << c;
//        QApplication::processEvents(QEventLoop::AllEvents, 100);
//        Sleeper().sleep(100);
//    }

//    // Reset m_isDone.
//    m_isDone = false;
//}

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
    qDebug() << "FtpClient::testConnection";

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
