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
//    m_ftp = new QFtp(this);
//    connect(m_ftp, SIGNAL(commandStarted(int)), this, SLOT(commandStarted(int)));
//    connect(m_ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(commandFinished(int,bool)));
//    connect(m_ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(addToList(QUrlInfo)));
//    connect(m_ftp, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(updateDataTransferProgress(qint64,qint64)));
//    connect(m_ftp, SIGNAL(stateChanged(int)), this, SLOT(stateChangedFilter(int)));
//    connect(m_ftp, SIGNAL(done(bool)), this, SLOT(doneFilter(bool)));

    // Load accessTokenPair from file
    loadAccessPairMap();

    m_itemList = new QList<QUrlInfo>();
}

FtpClient::~FtpClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

    m_itemList = 0;
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

void FtpClient::commandStarted(int id)
{
    qDebug() << "FtpClient::commandStarted" << id;
}

void FtpClient::commandFinished(int id, bool error)
{
    qDebug() << "FtpClient::commandFinished" << id << "error" << error;
}

void FtpClient::addToList(const QUrlInfo &i)
{
    qDebug() << "FtpClient::addToList" << i.name() << i.isDir() << i.isFile() << i.lastModified();
    m_itemList->append(i);
}

void FtpClient::rawCommandReplyFinished(int replyCode, QString result)
{
    qDebug() << "FtpClient::rawCommandReplyFinished" << replyCode << result;
    QRegExp rx(".*\"([^\"]+)\".*");
    if (rx.exactMatch(result)) {
        qDebug() << "FtpClient::rawCommandReplyFinished rx.capturedTexts()" << rx.capturedTexts();
        if (rx.captureCount() > 0) {
            m_currentPath = rx.cap(1);
        }
    }
}

void FtpClient::updateDataTransferProgress(qint64 done, qint64 total)
{
    qDebug() << "FtpClient::updateDataTransferProgress" << done << total;
}

void FtpClient::stateChangedFilter(int state)
{
    qDebug() << "FtpClient::stateChangedFilter" << state;
}

void FtpClient::doneFilter(bool error)
{
    qDebug() << "FtpClient::doneFilter" << error;
    m_isDone = true;
}

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
}

void FtpClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath)
{
}

void FtpClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "FtpClient::metadata" << uid << remoteFilePath;

    m_ftp = connectToHost(uid);

    // Get item list.
    m_itemList->clear();
    if (!remoteFilePath.isEmpty()) m_ftp->cd(remoteFilePath);
    m_ftp->rawCommand("PWD");
    m_ftp->list();

    int c = 100;
    while (!m_isDone && c-- > 0) {
        qDebug() << "FtpClient::metadata waitForDone" << m_isDone << c;
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }

    emit metadataReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), getItemListJson());

    m_ftp->close();
    m_ftp->deleteLater();
}

void FtpClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "FtpClient::browse" << uid << remoteFilePath;

    m_ftp = connectToHost(uid);

    // Get item list.
    m_itemList->clear();
    if (!remoteFilePath.isEmpty()) m_ftp->cd(remoteFilePath);
    m_ftp->rawCommand("PWD");
    m_ftp->list();

    int c = 100;
    while (!m_isDone && c-- > 0) {
        qDebug() << "FtpClient::browse waitForDone" << m_isDone << c;
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }

    emit browseReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), getItemListJson());

    m_ftp->close();
    m_ftp->deleteLater();
}

void FtpClient::createFolder(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "FtpClient::createFolder" << uid << localFilePath << remoteFilePath;

    m_ftp = connectToHost(uid);

    m_ftp->mkdir(remoteFilePath);

    int c = 100;
    while (!m_isDone && c-- > 0) {
        qDebug() << "FtpClient::createFolder waitForDone" << m_isDone << c;
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }

    emit createFolderReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath) );

    m_ftp->close();
    m_ftp->deleteLater();
}

void FtpClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "FtpClient::deleteFile" << uid << remoteFilePath;

    m_ftp = connectToHost(uid);

    m_ftp->rmdir(remoteFilePath);

    int c = 100;
    while (!m_isDone && c-- > 0) {
        qDebug() << "FtpClient::deleteFile waitForDone" << m_isDone << c;
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }

    emit deleteFileReplySignal(nonce, m_ftp->error(), m_ftp->errorString(), QString("{ \"path\": \"%1\" }").arg(remoteFilePath) );

    m_ftp->close();
    m_ftp->deleteLater();
}

QFtp * FtpClient::connectToHost(QString uid)
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

    m_isDone = false;
    QFtp *m_ftp = new QFtp(this);
    connect(m_ftp, SIGNAL(commandStarted(int)), this, SLOT(commandStarted(int)));
    connect(m_ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(commandFinished(int,bool)));
    connect(m_ftp, SIGNAL(rawCommandReply(int,QString)), this, SLOT(rawCommandReplyFinished(int,QString)));
    connect(m_ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(addToList(QUrlInfo)));
    connect(m_ftp, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(updateDataTransferProgress(qint64,qint64)));
    connect(m_ftp, SIGNAL(stateChanged(int)), this, SLOT(stateChangedFilter(int)));
    connect(m_ftp, SIGNAL(done(bool)), this, SLOT(doneFilter(bool)));

    m_ftp->abort();

    m_ftp->connectToHost(hostname, port);
    m_ftp->login(username, password);

    return m_ftp;
}

void FtpClient::saveConnection(QString id, QString hostname, quint16 port, QString username, QString password)
{
    /* Notes:
     * Stores token as user@host --> Token(token=user@host, secret=password, email=user@host)
     */
    TokenPair tokenPair;
    tokenPair.token = QString("%1@%2:%3").arg(username).arg(hostname).arg(port);
    tokenPair.secret = password;
    tokenPair.email = QString("%1@%2").arg(username).arg(hostname);
    accessTokenPairMap[id] = tokenPair;

    saveAccessPairMap();
}

QString FtpClient::getItemListJson()
{
    QString dataJsonText;
    QUrlInfo item;
    for (int i = 0; i < m_itemList->count(); i++) {
        item = m_itemList->at(i);
        if (dataJsonText.length() > 0) {
            dataJsonText.append(", ");
        }
        dataJsonText.append(QString("{ \"name\": \"%1\", \"path\": \"%2\", \"lastModified\": \"%3\", \"size\": %4, \"isDir\": %5 }")
                            .arg(item.name())
                            .arg(m_currentPath + "/" + item.name())
                            .arg(item.lastModified().toUTC().toString(Qt::ISODate))
                            .arg(item.size())
                            .arg(item.isDir())
                            );
    }
    dataJsonText.prepend("[ ").append(" ]");

    QString propertyJsonText;
    propertyJsonText.append(QString("{ \"path\": \"%1\" }").arg(m_currentPath));

    return QString("{ \"data\": %1, \"property\": %2 }").arg(dataJsonText).arg(propertyJsonText);
}

bool FtpClient::testConnection(QString hostname, quint16 port, QString username, QString password)
{
    qDebug() << "FtpClient::testConnection";

    // Reset m_isDone.
//    m_isDone = false;

    bool res = false;
    m_ftp = new QFtp(this);
    m_ftp->connectToHost(hostname, port);
    m_ftp->login(username, password);

    int c = 100;
    while (m_ftp->hasPendingCommands() && c-- > 0) {
        qDebug() << "FtpClient::testConnection waitForDone" << c;
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }
    qDebug() << "FtpClient::testConnection done state" << m_ftp->state();

    res = (m_ftp->state() == QFtp::LoggedIn);

    m_ftp->close();
    m_ftp->deleteLater();

    return res;
}
