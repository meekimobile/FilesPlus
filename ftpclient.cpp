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
    emit quotaReplySignal(nonce, 0, "", "{ }", 0, 0, 0);
}

QString FtpClient::fileGetReplyFinished(QString nonce, bool error, bool synchronous)
{
    QFtpWrapper *m_ftp = m_ftpHash->value(nonce);
    disconnect(m_ftp, SIGNAL(done(QString,bool)), this, SLOT(fileGetReplyFinished(QString,bool)) );
    QString uid = m_ftp->m_uid;
    QString remoteFilePath = m_ftp->m_remoteFilePath;
    QString result;

    if (!error) {
        if (m_ftp->getItemList().isEmpty()) {
            // remoteFilePath is not found.
            qDebug() << "FtpClient::fileGet" << uid << remoteFilePath << "is not found.";
            result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(-1).arg(tr("%1 is not found.").arg(remoteFilePath));
            if (!synchronous) emit fileGetReplySignal(m_ftp->getNonce(), -1, tr("Can't get %1").arg(remoteFilePath), result);
        } else {
            qDebug() << "FtpClient::fileGet" << uid << remoteFilePath << "is a file.";
            result = getPropertyJson(getParentRemotePath(remoteFilePath), m_ftp->getItemList().first());
            if (!synchronous) emit fileGetReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), result);
        }
    } else {
        // NOTE json string doesn't support newline character.
        QString escapedErrorString =  m_ftp->errorString().replace("\n", " ");
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(m_ftp->error()).arg(escapedErrorString);
        if (!synchronous) emit fileGetReplySignal(m_ftp->getNonce(), m_ftp->error(), escapedErrorString, result);
    }

    // Close and remove localFile pointer.
    if (m_localFileHash.contains(m_ftp->getNonce())) {
        m_localFileHash[m_ftp->getNonce()]->close();
        m_localFileHash.remove(m_ftp->getNonce());
    }

    // Clean up.
    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    return result;
}

QString FtpClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous)
{
    qDebug() << "----- FtpClient::fileGet -----" << nonce << uid << remoteFilePath << localFilePath << synchronous;

    QString remoteParentPath = getParentRemotePath(remoteFilePath);
    QString result;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        return QString("{ \"error\": -1, \"error_string\": \"%1\" }").arg(tr("Host is not accessible."));
    }
    m_ftp->m_remoteFilePath = remoteFilePath;
    m_ftp->m_localFilePath = localFilePath;
    m_ftp->cd(remoteParentPath);
    m_ftp->waitForDone();
    // TODO Check if it's timeout then emit error.

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localTargetFile = m_localFileHash[nonce];
    if (localTargetFile->open(QIODevice::WriteOnly)) {
        // Connect done signal to fileGetReplyFinished slot.
        if (!synchronous) connect(m_ftp, SIGNAL(done(QString,bool)), this, SLOT(fileGetReplyFinished(QString,bool)) );
        m_ftp->get(getRemoteFileName(remoteFilePath), localTargetFile);
        m_ftp->list(remoteFilePath);
    } else {
        result = QString("{ \"error\": -1, \"error_string\": \"Can't open file %1\" }").arg(localFilePath);
        if (!synchronous) emit fileGetReplySignal(m_ftp->getNonce(), -1, "", result);
    }

    if (!synchronous) {
        return result;
    }

    // Wait until pending commands are done.
    m_ftp->waitForDone();

    return fileGetReplyFinished(nonce, (m_ftp->error() != QFtp::NoError), synchronous);
}

/*
void FtpClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::filePut -----" << localFilePath << "to" << remoteFilePath;

    QStringList tokens = accessTokenPairMap[uid].email.split("@");

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

void FtpClient::filePutReplyFinished(QString nonce, bool error, bool synchronous)
{
    QFtpWrapper *m_ftp = m_ftpHash->value(nonce);
    disconnect(m_ftp, SIGNAL(done(QString,bool)), this, SLOT(filePutReplyFinished(QString,bool)) );
    QString uid = m_ftp->m_uid;
    QString remoteFilePath = m_ftp->m_remoteFilePath;
    QString result;

    if (!error) {
        if (m_ftp->getItemList().isEmpty()) {
            // remoteFilePath is not found.
            qDebug() << "FtpClient::filePut" << uid << remoteFilePath << "is not found.";
            result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(-1).arg(tr("%1 is not found.").arg(remoteFilePath));
            if (!synchronous) emit filePutReplySignal(m_ftp->getNonce(), -1, tr("Can't put %1").arg(remoteFilePath), result);
        } else {
            qDebug() << "FtpClient::filePut" << uid << remoteFilePath << "is a file.";
            result = getPropertyJson(getParentRemotePath(remoteFilePath), m_ftp->getItemList().first());
            if (!synchronous) emit filePutReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), result);
        }
    } else {
        // NOTE json string doesn't support newline character.
        QString escapedErrorString =  m_ftp->errorString().replace("\n", " ");
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(m_ftp->error()).arg(escapedErrorString);
        if (!synchronous) emit filePutReplySignal(m_ftp->getNonce(), m_ftp->error(), escapedErrorString, result);
    }

    // Close and remove localFile pointer.
    if (m_localFileHash.contains(m_ftp->getNonce())) {
        m_localFileHash[m_ftp->getNonce()]->close();
        m_localFileHash.remove(m_ftp->getNonce());
    }

    // Clean up.
    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName)
{
    qDebug() << "----- FtpClient::filePut -----" << nonce << uid << localFilePath << remoteParentPath << remoteFileName;

    QString remoteFilePath = remoteParentPath + "/" + remoteFileName;
    QString result;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        emit filePutReplySignal(nonce, QNetworkReply::HostNotFoundError, tr("Host is not accessible."), "");
        return;
    }
    m_ftp->m_localFilePath = localFilePath;
    m_ftp->m_remoteFilePath = remoteFilePath;
    m_ftp->cd(remoteParentPath);
    m_ftp->waitForDone();
    // TODO Check if it's timeout then emit error.

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        // Connect done signal to filePutReplyFinished slot.
        connect(m_ftp, SIGNAL(done(QString,bool)), this, SLOT(filePutReplyFinished(QString,bool)) );
        m_ftp->put(localSourceFile, remoteFileName);
        m_ftp->list(remoteFilePath);
    } else {
        result = QString("{ \"error\": -1, \"error_string\": \"Can't open file %1\" }").arg(localFilePath);
        emit filePutReplySignal(m_ftp->getNonce(), -1, "", result);
    }
}

QString FtpClient::mergePropertyAndFilesJson(QString propertyJsonText, QString filesJsonText)
{
    QScriptEngine engine;
    QScriptValue mergedObj = engine.evaluate("(" + propertyJsonText + ")");
    mergedObj.setProperty("children", engine.evaluate("(" + filesJsonText + ")"));

    return stringifyScriptValue(engine, mergedObj);
}

// TODO Issue: #FP20130201 FTPClient abort() doesn't return as error. It is told as successful job ??
//bool FtpClient::abort(QString nonce)
//{
//    if (m_ftpHash->contains(nonce)) {
//        QFtpWrapper *ftp = m_ftpHash->value(nonce);
//        ftp->abort();
//        qDebug() << "FtpClient::abort nonce" << nonce << "is aborted.";
//    } else {
//        qDebug() << "FtpClient::abort nonce" << nonce << "is not found. Operation is ignored.";
//    }
//}

QString FtpClient::property(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::property -----" << uid << remoteFilePath;

    if (remoteFilePath == "/" || remoteFilePath == "") {
        return getRootPropertyJson();
    }

    QString propertyJson = "";
    QString remoteParentPath = getParentRemotePath(remoteFilePath);
    QString remoteFileName = getRemoteFileName(remoteFilePath);

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        return QString("{ \"error\": %1, \"errorString\": \"%2\" }").arg(-1).arg(tr("Host is not accessible."));
    }
    m_ftp->resetIsDone();
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
        // NOTE json string doesn't support newline character.
        QString escapedErrorString =  m_ftp->errorString().replace("\n", " ");
        qDebug() << "FtpClient::property" << uid << remoteFilePath << "error" << m_ftp->error() << escapedErrorString;
        propertyJson = QString("{ \"error\": %1, \"errorString\": \"%2\" }").arg(m_ftp->error()).arg(escapedErrorString);
    }

    // NOTE It's private method which is invoked by others. So leave clean up for caller.
//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(m_ftp->getNonce());

    return propertyJson;
}

void FtpClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::metadata -----"<< nonce << uid << remoteFilePath;

    QString propertyJson = property(nonce, uid, remoteFilePath);
    if (propertyJson.isEmpty()) {
        qDebug() << "FtpClient::metadata" << nonce << uid << remoteFilePath << "is not found.";
        emit metadataReplySignal(nonce, QNetworkReply::ContentNotFoundError, tr("%1 is not found.").arg(remoteFilePath), "");
        QFtpWrapper *m_ftp = m_ftpHash->value(nonce);
        if (m_ftp != 0) {
            m_ftp->close();
            m_ftp->deleteLater();
            m_ftpHash->remove(m_ftp->getNonce());
        }
        return;
    } else {
        QScriptEngine engine;
        QScriptValue propertyJsonObj = engine.evaluate("(" + propertyJson + ")");
        if (propertyJsonObj.isValid() && propertyJsonObj.property("error").isValid()) {
            emit metadataReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, tr("%1 is not accessible.").arg(remoteFilePath), propertyJson);
            QFtpWrapper *m_ftp = m_ftpHash->value(nonce);
            if (m_ftp != 0) {
                m_ftp->close();
                m_ftp->deleteLater();
                m_ftpHash->remove(m_ftp->getNonce());
            }
            return;
        }
    }

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        emit metadataReplySignal(nonce, QNetworkReply::HostNotFoundError, tr("Host is not accessible."), "");
        m_ftp->close();
        m_ftp->deleteLater();
        m_ftpHash->remove(m_ftp->getNonce());
        return;
    }

    // Check if remoteFilePath is dir or file by change directory.
    m_ftp->resetIsDone();
    m_ftp->cd(remoteFilePath);
    m_ftp->waitForDone();

    if (m_ftp->error() == QFtp::NoError) {
        // remoteFilePath is dir.
        qDebug() << "FtpClient::metadata" << uid << remoteFilePath << " is dir.";
        m_ftp->resetIsDone();
        m_ftp->pwd();
        m_ftp->list();
        m_ftp->waitForDone();

        QString dataJson = getItemListJson(m_ftp->getCurrentPath(), m_ftp->getItemList());

        emit metadataReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), mergePropertyAndFilesJson(propertyJson, dataJson) );
    } else {
        // remoteFilePath is file or not found.
        qDebug() << "FtpClient::metadata" << uid << remoteFilePath << "is file or not found. error" << m_ftp->error() << m_ftp->errorString();
        m_ftp->resetIsDone();
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
            emit metadataReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), mergePropertyAndFilesJson(getPropertyJson(remoteParentPath, m_ftp->getItemList().first()), "[]") );
        }
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::browse -----" << nonce << uid << remoteFilePath;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        emit browseReplySignal(nonce, QNetworkReply::HostNotFoundError, tr("Host is not accessible."), "");
        return;
    }

    // Get item list.
    if (!remoteFilePath.isEmpty()) {
        m_ftp->cd(remoteFilePath);
        m_ftp->waitForDone();
    }
    // TODO Make it provide port for firewall environment.
//    m_ftp->rawCommand("PORT 10,0,2,15,43,113");

    if (m_ftp->error() == QFtp::NoError) {
        // Browse a folder.
        m_ftp->pwd();
        m_ftp->list(remoteFilePath);
        m_ftp->waitForDone();

        if (remoteFilePath == "") {
            m_remoteRootHash[uid] = m_ftp->getCurrentPath();
            qDebug() << "FtpClient::browse" << nonce << uid << "save RemoteRoot" << m_remoteRootHash[uid];
        }
        QString dataJson = getItemListJson(m_ftp->getCurrentPath(), m_ftp->getItemList());
        QString propertyJson = property(nonce, uid, m_ftp->getCurrentPath());

        emit browseReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), mergePropertyAndFilesJson(propertyJson, dataJson) );
    } else {
        // remoteFilePath is file or not found.
        qDebug() << "FtpClient::browse nonce" << nonce << "uid" << uid << remoteFilePath << "is file or not found. error" << m_ftp->error() << m_ftp->errorString();
        m_ftp->list(remoteFilePath);
        m_ftp->waitForDone();

        if (m_ftp->getItemList().isEmpty()) {
            // remoteFilePath is not found.
            qDebug() << "FtpClient::browse nonce" << nonce << "uid" << uid << remoteFilePath << "is not found.";
            emit browseReplySignal(m_ftp->getNonce(), QNetworkReply::ContentNotFoundError, tr("%1 is not found.").arg(remoteFilePath), "");
        } else {
            // remoteFilePath is file.
            QString remoteParentPath = getParentRemotePath(remoteFilePath);
            qDebug() << "FtpClient::browse nonce" << nonce << "uid" << uid << remoteFilePath << "is a file. remoteParentPath" << remoteParentPath << "remoteFileName" << m_ftp->getItemList().first().name();
            emit browseReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), mergePropertyAndFilesJson(getPropertyJson(remoteParentPath, m_ftp->getItemList().first()), "[]") );
        }
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());
}

void FtpClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    emit moveFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, tr("FTP doesn't support move command."), "");
}

void FtpClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    emit copyFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, tr("FTP doesn't support copy command."), "");
}

QString FtpClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- FtpClient::deleteFile -----" << nonce << uid << remoteFilePath << synchronous;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        emit deleteFileReplySignal(nonce, QNetworkReply::HostNotFoundError, tr("Host is not accessible."), "");
        return "";
    }

    deleteRecursive(m_ftp, remoteFilePath);

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    return "";
}

void FtpClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- FtpClient::shareFile -----" << uid << remoteFilePath;

    emit shareFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, "FTP doesn't support resource link sharing.", "", "", 0);
}

QString FtpClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- FtpClient::createFolder -----" << nonce << uid << remoteParentPath << newRemoteFolderName;

    // NOTE Emit signal only for asynchronous request. To avoid invoking CloudDriveModel.createFolderReplyFilter() as it's not required. And also avoid invoking jobDone() which causes issue #FP20130232.

    if (remoteParentPath.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return "";
    }

    QString newRemoteFolderPath = remoteParentPath + "/" + newRemoteFolderName;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        if (!synchronous) emit createFolderReplySignal(nonce, QNetworkReply::HostNotFoundError, tr("Host is not accessible."), "");
        return QString("{ \"error\": %1, \"errorString\": \"%2\" }").arg(-1).arg(tr("Host is not accessible."));
    }

    m_ftp->mkdir(newRemoteFolderPath);
    m_ftp->waitForDone();

    QString result;
    if (m_ftp->error() == QFtp::NoError) {
        // Get property.
        QString propertyJson = property(nonce, uid, newRemoteFolderPath);
        if (propertyJson != "") {
            result = propertyJson;
            if (!synchronous) emit createFolderReplySignal(nonce, QNetworkReply::NoError, "", result);
        } else {
            qDebug() << "FtpClient::createFolder" << nonce << uid << newRemoteFolderPath << "is not found.";
            if (!synchronous) emit createFolderReplySignal(nonce, QNetworkReply::ContentNotFoundError, "Created path is not found.", QString("{ \"error\": \"Created path is not found.\", \"path\": \"%1\" }").arg(newRemoteFolderPath) );
        }
    } else {
        qDebug() << "FtpClient::createFolder" << nonce << "error" << m_ftp->error() << m_ftp->errorString();

        // Try getting property if folder with same name exists.
        QString propertyJson = property(nonce, uid, newRemoteFolderPath);
        if (propertyJson != "") {
            result = propertyJson;
            if (!synchronous) emit createFolderReplySignal(nonce, QNetworkReply::NoError, "", result);
        } else {
            // NOTE json string doesn't support newline character.
            QString escapedErrorString =  m_ftp->errorString().replace("\n", " ");
            if (!synchronous) emit createFolderReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, m_ftp->errorString(), QString("{ \"error\": \"%1\", \"path\": \"%2\" }").arg(escapedErrorString).arg(newRemoteFolderPath) );
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
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        return 0;
    }
    m_ftp->m_remoteFilePath = remoteFilePath;
    m_ftp->cd(remoteParentPath);
    m_ftp->waitForDone();

    // Creat buffer as QIODevice.
    QBuffer *buf = new QBuffer();
    if (buf->open(QIODevice::WriteOnly)) {
        m_ftp->resetIsDone();
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

    QString remoteFilePath = remoteParentPath + "/" + remoteFileName;

    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
    // Check state if it's accessible.
    if (m_ftp->state() != QFtp::LoggedIn) {
        emit migrateFilePutReplySignal(nonce, QNetworkReply::HostNotFoundError, tr("Host is not accessible."), "");
        return 0;
    }
    m_ftp->m_remoteFilePath = remoteFilePath;
    m_ftp->cd(remoteParentPath);

    m_ftp->put(source, remoteFileName);
    m_ftp->waitForDone();
    source->close();

    // TODO Get uploaded file property.
    m_ftp->list(remoteFilePath);
    m_ftp->waitForDone();

    if (m_ftp->getItemList().isEmpty()) {
        // remoteFilePath is not found.
        qDebug() << "FtpClient::filePut" << uid << remoteFilePath << "is not found.";
        emit migrateFilePutReplySignal(m_ftp->getNonce(), -1, tr("Can't put %1").arg(remoteFilePath), "");
    } else {
        qDebug() << "FtpClient::filePut" << uid << remoteFilePath << "is a file. remoteParentPath" << remoteParentPath << "remoteFileName" << m_ftp->getItemList().first().name();
        emit migrateFilePutReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(remoteParentPath, m_ftp->getItemList().first()) );
    }

    m_ftp->close();
    m_ftp->deleteLater();
    m_ftpHash->remove(m_ftp->getNonce());

    return 0;
}

QFtpWrapper *FtpClient::connectToHost(QString nonce, QString uid)
{
    if (m_ftpHash->contains(nonce)) {
        QFtpWrapper *ftp = m_ftpHash->value(nonce);
        qDebug() << "FtpClient::connectToHost" << nonce << "return stored ftp object" << ftp;
        return ftp;
    }

    /* Notes:
     * Stores email as user@host --> Token(token="", secret=password, email=user@host)
     */
    QString url = accessTokenPairMap[uid].email;
    QString username, hostname;
    quint16 port = 21; // Default ftp port = 21
    QRegExp rx("([^@]*)@([^:]*)(:.*)*");
    if (rx.exactMatch(url)) {
        qDebug() << "FtpClient::connectToHost" << rx.captureCount() << rx.capturedTexts();
        if (rx.captureCount() >= 2) {
            if (rx.captureCount() == 3 && rx.cap(3) != "") {
                // Skip ':' by start at position 1.
                port = rx.cap(3).mid(1).toInt();
            }
            username = rx.cap(1);
            hostname = rx.cap(2);
        }
    }
    QString password = accessTokenPairMap[uid].secret;
    qDebug() << "FtpClient::connectToHost" << hostname << port << username;

    QFtpWrapper *m_ftp = new QFtpWrapper(nonce, this);
    m_ftp->m_uid = uid;
    connect(m_ftp, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(m_ftp, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    m_ftp->abort();
    m_ftp->connectToHost(hostname, port);
    m_ftp->login(username, password);
    m_ftp->waitForDone();
    // Check state if it's accessible.
    if (m_ftp->state() == QFtp::LoggedIn) {
        // Insert to hash.
        m_ftpHash->insert(nonce, m_ftp);
    }

    return m_ftp;
}

QString FtpClient::getPropertyJson(const QString parentPath, const QUrlInfo item)
{
    // Handle absolute path in name.
    QString path = removeDoubleSlash((item.name().startsWith("/") ? "" : parentPath + "/") + item.name());
    uint itemSize = item.size();
    bool isHidden = item.name().startsWith(".");

    QScriptEngine engine;
    QScriptValue parsedObj = engine.newObject();
    parsedObj.setProperty("name", QScriptValue(item.name()));
    parsedObj.setProperty("absolutePath", QScriptValue(path));
    parsedObj.setProperty("parentPath", QScriptValue(parentPath));
    parsedObj.setProperty("size", QScriptValue(itemSize));
    parsedObj.setProperty("isDeleted", QScriptValue(false));
    parsedObj.setProperty("isHidden", QScriptValue(isHidden));
    parsedObj.setProperty("isDir", QScriptValue(item.isDir()));
    parsedObj.setProperty("lastModified", QScriptValue(formatJSONDateString(item.lastModified())));
    parsedObj.setProperty("hash", QScriptValue(formatJSONDateString(item.lastModified())));
    parsedObj.setProperty("source", QScriptValue());
    parsedObj.setProperty("thumbnail", QScriptValue());
    parsedObj.setProperty("fileType", QScriptValue(getFileType(item.name())));

    return stringifyScriptValue(engine, parsedObj);
}

QString FtpClient::getRootPropertyJson()
{
    QScriptEngine engine;
    QScriptValue parsedObj = engine.newObject();
    parsedObj.setProperty("name", QScriptValue("/"));
    parsedObj.setProperty("absolutePath", QScriptValue("/"));
    parsedObj.setProperty("parentPath", QScriptValue(""));
    parsedObj.setProperty("size", QScriptValue(0));
    parsedObj.setProperty("isDeleted", QScriptValue(false));
    parsedObj.setProperty("isDir", QScriptValue(true));
    parsedObj.setProperty("lastModified", QScriptValue(""));
    parsedObj.setProperty("hash", QScriptValue(""));
    parsedObj.setProperty("source", QScriptValue(""));
    parsedObj.setProperty("thumbnail", QScriptValue(""));
    parsedObj.setProperty("fileType", QScriptValue(""));

    return stringifyScriptValue(engine, parsedObj);
}

QString FtpClient::getItemListJson(const QString parentPath, const QList<QUrlInfo> itemList)
{
    QString dataJsonText;
    QUrlInfo item;
    for (int i = 0; i < itemList.count(); i++) {
        item = itemList.at(i);
        // Hide hidden child.
        if (!m_settings.value("FtpClient.getItemListJson.showHiddenSystem.enabled", false).toBool()
                && item.name().startsWith(".")) {
            // Skip hidden child if showing is disabled.
        } else {
            // Add child to list.
            if (dataJsonText.length() > 0) {
                dataJsonText.append(", ");
            }
            dataJsonText.append(getPropertyJson(parentPath, item));
        }
    }
    dataJsonText.prepend("[ ").append(" ]");

    return dataJsonText;
}

QString FtpClient::getParentRemotePath(QString remotePath)
{
    QString remoteParentPath = "";
    if (remotePath != "" && remotePath != "/") {
        remoteParentPath = remotePath.mid(0, remotePath.lastIndexOf("/"));
        remoteParentPath = (remoteParentPath == "") ? "/" : remoteParentPath;
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

bool FtpClient::testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname)
{
    qDebug() << "----- FtpClient::testConnection -----";

    // Reset m_isDone.
//    m_isDone = false;
    QStringList tokens = hostname.split(":");
    QString host = tokens.at(0);
    int port = (tokens.length() >= 2) ? tokens.at(1).toInt() : 21;

    bool res = false;
    QFtpWrapper *m_ftp = new QFtpWrapper("test", this);
    m_ftp->connectToHost(host, port);
    m_ftp->login(username, password);
    m_ftp->waitForDone();
    qDebug() << "FtpClient::testConnection done state" << m_ftp->state();

    res = (m_ftp->state() == QFtp::LoggedIn);
    if (!res) {
        emit logRequestSignal("",
                              "error",
                              "FTP " + tr("Test connection"),
                              m_ftp->errorString(),
                              5000);
    }

    m_ftp->close();
    m_ftp->deleteLater();

    return res;
}

bool FtpClient::saveConnection(QString id, QString hostname, QString username, QString password, QString token)
{
    qDebug() << "----- FtpClient::saveConnection -----";

    /* Notes:
     * Stores email as username@hostname --> Token(token="", secret=password, email=username@hostname)
     */
    // TODO Encrypt password before store to file.
    if (accessTokenPairMap.contains(id)) {
        accessTokenPairMap[id].token = token;
        accessTokenPairMap[id].secret = password;
        accessTokenPairMap[id].email = QString("%1@%2").arg(username).arg(hostname);
    } else {
        TokenPair tokenPair;
        tokenPair.token = token;
        tokenPair.secret = password;
        tokenPair.email = QString("%1@%2").arg(username).arg(hostname);
        accessTokenPairMap[id] = tokenPair;
    }

    saveAccessPairMap();

    return true;
}

QString FtpClient::getRemoteRoot(QString uid)
{
    return m_remoteRootHash[uid];
}

bool FtpClient::isRemoteAbsolutePath()
{
    return true;
}

bool FtpClient::isConfigurable()
{
    return true;
}

bool FtpClient::isUnicodeSupported()
{
    return false;
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
