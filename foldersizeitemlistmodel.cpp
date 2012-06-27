#include "foldersizeitemlistmodel.h"

const int FolderSizeItemListModel::TimerInterval = 100;
const int FolderSizeItemListModel::MaxRunningJobCount = 1;

FolderSizeItemListModel::FolderSizeItemListModel(QObject *parent)
    : QAbstractListModel(parent), m_pathToRootCache()
{
    // This map will be used when populating property to delegate.
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[AbsolutePathRole] = "absolutePath";
    roles[SizeRole] = "size";
    roles[LastModifiedRole] = "lastModified";
    roles[IsDirRole] = "isDir";
    roles[SubDirCountRole] = "subDirCount";
    roles[SubFileCountRole] = "subFileCount";
    roles[FileTypeRole] = "fileType";
    roles[IsRunningRole] = "isRunning";
    roles[RunningOperationRole] = "runningOperation";
    roles[RunningValueRole] = "runningValue";
    roles[RunningMaxValueRole] = "runningMaxValue";
    roles[IsCheckedRole] = "isChecked";
    setRoleNames(roles);

    // Connect job queue chain.
    connect(this, SIGNAL(proceedNextJobSignal()), SLOT(proceedNextJob()) );

    // Connect model class with listModel.
    connect(&m, SIGNAL(loadDirSizeCacheFinished()), this, SLOT(loadDirSizeCacheFinishedFilter()) );
    connect(&m, SIGNAL(fetchDirSizeStarted()), this, SIGNAL(fetchDirSizeStarted()) );
    connect(&m, SIGNAL(fetchDirSizeFinished()), this, SLOT(fetchDirSizeFinishedFilter()) );
    connect(&m, SIGNAL(copyStarted(int,QString,QString,QString,int)), this, SIGNAL(copyStarted(int,QString,QString,QString,int)) );
    connect(&m, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)), this, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)) );
    connect(&m, SIGNAL(copyFinished(int,QString,QString,QString,int)), this, SLOT(copyFinishedFilter(int,QString,QString,QString,int)) );
    connect(&m, SIGNAL(fetchDirSizeUpdated(QString)), this, SIGNAL(fetchDirSizeUpdated(QString)) );
    connect(&m, SIGNAL(deleteFinished(QString)), this, SLOT(deleteFinishedFilter(QString)) );
    connect(&m, SIGNAL(finished()), this, SLOT(jobDone()) );

//    // Load cache
//    m.setRunMethod(m.LoadDirSizeCache);
//    m.start();

    // Enqueue job.
    FolderSizeJob job(createNonce(), FolderSizeModelThread::LoadDirSizeCache, "", "");
    m_jobQueue.enqueue(job);

    emit proceedNextJobSignal();
}

FolderSizeItemListModel::~FolderSizeItemListModel()
{
    // Save cache
    m.saveDirSizeCache();
}

int FolderSizeItemListModel::rowCount(const QModelIndex & parent) const {
//    qDebug() << "FolderSizeItemListModel::rowCount " + QString("%1").arg(m.getDirContent().count());

    return getItemList().count();
}

int FolderSizeItemListModel::columnCount(const QModelIndex &parent) const
{
    return roleNames().count();
}

QVariant FolderSizeItemListModel::data(const QModelIndex & index, int role) const {
//    qDebug() << "FolderSizeItemListModel::data " + QString("%1 %2").arg(index.row()).arg(role);
    // TODO refactor to avoid populating whole folder's content which costs more heap.

    // This code causes error on QML because undefined is assigned to int, bool, ...
//    if (index.row() < 0 || index.row() > m.getItemList().count())
//        return QVariant();

    const FolderSizeItem &item = getItemList().at(index.row());
    if (role == NameRole)
        return item.name;
    else if (role == AbsolutePathRole)
        return item.absolutePath;
    else if (role == SizeRole)
        return item.size;
    else if (role == LastModifiedRole)
        return item.lastModified;
    else if (role == IsDirRole)
        return item.isDir;
    else if (role == SubDirCountRole)
        return item.subDirCount;
    else if (role == SubFileCountRole)
        return item.subFileCount;
    else if (role == FileTypeRole)
        return item.fileType;
    else if (role == IsRunningRole)
        return item.isRunning;
    else if (role == RunningOperationRole)
        return item.runningOperation;
    else if (role == RunningValueRole)
        return item.runningValue;
    else if (role == RunningMaxValueRole)
        return item.runningMaxValue;
    else if (role == IsCheckedRole)
        return item.isChecked;
    return QVariant();
}

QString FolderSizeItemListModel::currentDir() const
{
    return m.currentDir();
}

void FolderSizeItemListModel::setCurrentDir(const QString &path)
{
    m.setCurrentDir(path);

    // Emit to update currentDir on UI.
    emit currentDirChanged();

    // Invoke background refresh
    refreshDir();

    qDebug() << QTime::currentTime() << "FolderSizeItemListModel::setCurrentDir";
}

QStringList FolderSizeItemListModel::getDriveList()
{
    QFileInfoList drives = QDir::drives();
    QStringList driveList;

    for (int i=0; i<drives.length(); i++) {
        driveList.append(drives.at(i).absolutePath());
    }

    return driveList;
}

bool FolderSizeItemListModel::isDirSizeCacheExisting()
{
    return m.isDirSizeCacheExisting();
}

bool FolderSizeItemListModel::isReady()
{
    return !m.isRunning();
}

QVariant FolderSizeItemListModel::getProperty(const int index, FolderSizeItemListModel::FolderSizeItemRoles role)
{
    if (index >= 0 && index < rowCount()) {
        return data(createIndex(index,0), role);
    }

    return QVariant();
}

void FolderSizeItemListModel::setProperty(const int index, FolderSizeItemListModel::FolderSizeItemRoles role, QVariant value)
{
    if (index >= 0 && index < rowCount()) {
        // Update to limited properties.
        FolderSizeItem item = getItem(index);
        if (role == IsRunningRole)
            item.isRunning = value.toBool();
        else if (role == RunningOperationRole)
            item.runningOperation = value.toInt();
        else if (role == RunningValueRole)
            item.runningValue = value.toLongLong();
        else if (role == RunningMaxValueRole)
            item.runningMaxValue = value.toLongLong();
        else if (role == IsCheckedRole)
            item.isChecked = value.toBool();

        setItem(index, item);

        refreshItem(index);
    } else {
        // TODO do nothing.
    }
}

void FolderSizeItemListModel::setProperty(const QString localPath, FolderSizeItemListModel::FolderSizeItemRoles role, QVariant value)
{
    // Find index for localPath on currentDir.
    int modelIndex = getIndexOnCurrentDir(localPath);
//    qDebug() << "FolderSizeItemListModel::setProperty" << modelIndex;

    if (modelIndex > -1) {
        setProperty(modelIndex, role, value);
    } else if (modelIndex == -1) {
        // Do nothing as it's not on currentDir.
    } else if (modelIndex == -2) {
        refreshItems();
    }
}

QString FolderSizeItemListModel::formatFileSize(double size)
{
    const uint KB = 1024;
    const uint MB = 1048576;
    const uint GB = 1073741824;

    QString fileSize;

    if (size >= GB) {
        fileSize = QString("%1 GB").arg( int(size/GB) );
    } else if (size >= MB) {
        fileSize = QString("%1 MB").arg( int(size/MB) );
    } else if (size >= 1024) {
        fileSize = QString("%1 KB").arg( int(size/KB) );
    } else {
        fileSize = QString("%1").arg( size );
    }

    return fileSize;
}

QList<FolderSizeItem> FolderSizeItemListModel::getItemList() const
{
    return itemList;
}

QString FolderSizeItemListModel::createNonce() {
    QString ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString nonce;

    for(int i = 0; i <= 16; ++i)
    {
        nonce += ALPHANUMERIC.at( qrand() % ALPHANUMERIC.length() );
    }

    return nonce;
}

void FolderSizeItemListModel::jobDone()
{
    runningJobCount--;

    emit proceedNextJobSignal();
}

void FolderSizeItemListModel::changeDir(const QString &name)
{
    // m.changeDir() reuses m.setCurrentDir().
    bool isDirChanged = m.changeDir(name);

    if (isDirChanged) {
        // Emit to update currentDir on UI.
        emit currentDirChanged();

        // Reset m_indexOnCurrentDirHash.
        m_indexOnCurrentDirHash.clear();

        // Invoke background refresh
        refreshDir();
    }
}

void FolderSizeItemListModel::refreshDir(const bool clearCache)
{
    qDebug() << "FolderSizeItemListModel::refreshDir clearCache" << clearCache;

    if (isReady()) {
        if (!clearCache && !isDirSizeCacheExisting()) {
            // If UI invoke general refresh but there is no existing cache, ask user for confirmation.
            emit requestResetCache();
        } else {
            // If UI invoke reset cache or their is existing cache, proceed as user requested.
            // Enqueue job.
            FolderSizeJob job(createNonce(), FolderSizeModelThread::FetchDirSize, "", "", clearCache);
            m_jobQueue.enqueue(job);

            emit proceedNextJobSignal();
        }

        // Once thread is done, it will invoke refreshItemList() in fetchDirSizeFinishedFilter().
        if (clearCache) {
            refreshItemList();
            refreshItems();
        }
    } else {
        qDebug() << "FolderSizeItemListModel::refreshDir is not ready. Refresh itemList as-is.";
        // Populate and sort directory content to itemList. Then respond to UI.
        refreshItemList();
        refreshItems();
    }
}

QString FolderSizeItemListModel::getUrl(const QString absPath)
{
    return QUrl::fromLocalFile(absPath).toString();
}

bool FolderSizeItemListModel::isRoot()
{
    QDir dir(m.currentDir());
    return (dir.isRoot());
}

int FolderSizeItemListModel::getSortFlag() const
{
    return m.sortFlag();
}

void FolderSizeItemListModel::setSortFlag(const int sortFlag)
{
    if (m.setSortFlag(sortFlag)) {
        m.sortItemList(itemList);

        // If itemList is actually sorted, refreshItems to emit dataChanged.
        refreshItems();
    }
}

QStringList FolderSizeItemListModel::getNameFilters() const
{
    return m.nameFilters();
}

void FolderSizeItemListModel::setNameFilters(const QStringList nameFilters)
{
    m.setNameFilters(nameFilters);
}

void FolderSizeItemListModel::refreshItems()
{
    beginResetModel();
    emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, columnCount()-1));
    endResetModel();
}

void FolderSizeItemListModel::refreshItem(const int index)
{
    emit dataChanged(createIndex(index,0), createIndex(index, columnCount()-1));
}

void FolderSizeItemListModel::refreshItem(const QString localPath)
{
    int index = getIndexOnCurrentDir(localPath);
    if (index > -1) {
        refreshItem(index);
    }
}

bool FolderSizeItemListModel::removeRow(int row, const QModelIndex &parent)
{
    return removeRows(row, 1, parent);
}

bool FolderSizeItemListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    bool res = true;
    for (int i = row; i < row+count; i++) {
        QString filePath = data(this->index(i,0), AbsolutePathRole).toString();
        bool iRes = false;
        QFileInfo fileInfo(filePath);

        iRes = m.deleteDir(filePath);

        // Remove item from model's itemList.
        if (iRes) {
            beginRemoveRows(parent, i, i);

            // Remote item from itemList.
            removeItem(i);

            // Remove cache is done in m.deleteDir()

            // Emit deleteFinished
            emit deleteFinished(fileInfo.absoluteFilePath());

            endRemoveRows();

            qDebug() << "FolderSizeItemListModel::removeRows" << fileInfo.absoluteFilePath() << " has been removed.";
        } else {
            qDebug() << "FolderSizeItemListModel::removeRows" << fileInfo.absoluteFilePath() << " can't' be removed because it's not empty.";
        }

        res = res && iRes;
    }

    return res;
}

bool FolderSizeItemListModel::deleteFile(const QString absPath)
{
    int index = getIndexOnCurrentDir(absPath);

    if (index >= 0) {
        return removeRow(index);
    } else {
        return m.deleteDir(absPath);
    }
}

bool FolderSizeItemListModel::copy(const QString sourcePath, const QString targetPath)
{
    if (sourcePath == targetPath) {
        emit copyFinished(FolderSizeModelThread::CopyFile, sourcePath, targetPath, "Source and Target path can't be the same.", -3);
    } else if (targetPath.indexOf(sourcePath) != -1) {
        emit copyFinished(FolderSizeModelThread::CopyFile, sourcePath, targetPath, "Target path can't be inside source path.", -4);
    }

    // TODO Show running on targetPath's parent.
    emit copyStarted(FolderSizeModelThread::CopyFile, sourcePath, getDirPath(targetPath), "Show running on targetPath's parent", 0);

    // Enqueue job.
    FolderSizeJob job(createNonce(), FolderSizeModelThread::CopyFile, sourcePath, targetPath);
    m_jobQueue.enqueue(job);

    emit proceedNextJobSignal();

    return true;
}

bool FolderSizeItemListModel::move(const QString sourcePath, const QString targetPath)
{
    if (sourcePath == targetPath) {
        emit copyFinished(FolderSizeModelThread::MoveFile, sourcePath, targetPath, "Source and Target path can't be the same.", -3);
    } else if (targetPath.indexOf(sourcePath) != -1) {
        emit copyFinished(FolderSizeModelThread::MoveFile, sourcePath, targetPath, "Target path can't be inside source path.", -4);
    }

    // TODO Show running on targetPath's parent.
    emit copyStarted(FolderSizeModelThread::MoveFile, sourcePath, getDirPath(targetPath), "Show running on targetPath's parent", 0);

    // Enqueue job.
    FolderSizeJob job(createNonce(), FolderSizeModelThread::MoveFile, sourcePath, targetPath);
    m_jobQueue.enqueue(job);

    emit proceedNextJobSignal();

    return true;
}

bool FolderSizeItemListModel::createDir(const QString name)
{
    QDir dir(currentDir());
    bool res = dir.mkdir(name);
    if (res) {
        emit createFinished(dir.absoluteFilePath(name));
    }
    return res;
}

bool FolderSizeItemListModel::renameFile(const QString fileName, const QString newFileName)
{
    if (fileName == newFileName) return false;

    QDir::setCurrent(currentDir());
    QFileInfo sourceFileInfo(fileName);
    if (!sourceFileInfo.isFile()) return false;

    int res = QFile::rename(fileName, newFileName);
    qDebug() << "FolderSizeItemListModel::renameFile res" << res << "fileName" << fileName << "newFileName" << newFileName;
    if (res) {
        // Emit signal to change CloudDriveItem.
        emit deleteFinished(QDir::current().absoluteFilePath(fileName));
        emit createFinished(QDir::current().absoluteFilePath(newFileName));
    }

    return res;
}

QString FolderSizeItemListModel::getDirPath(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.absolutePath();
}

QStringList FolderSizeItemListModel::getPathToRoot(const QString absFilePath)
{
//    qDebug() << "FolderSizeItemListModel::getPathToRoot" << absFilePath;

    // Try get from cache. It will not 0 if exists.
//    qDebug() << "FolderSizeItemListModel::getPathToRoot absFilePath" << absFilePath << "cache" << m_pathToRootCache.contains(absFilePath);
    if (!m_pathToRootCache.contains(absFilePath)) {
        QStringList paths;
        QDir dir(absFilePath);

        while (!dir.isRoot()) {
            paths << dir.absolutePath();
//            qDebug() << "FolderSizeItemListModel::getPathToRoot append" << dir.absolutePath();
            dir.cdUp();
        }
        paths << dir.absolutePath();
//        qDebug() << "FolderSizeItemListModel::getPathToRoot append" << dir.absolutePath();

        // Insert to cache.
        QString *cachePaths = new QString(paths.join(","));
        m_pathToRootCache.insert(absFilePath, cachePaths);
        qDebug() << "FolderSizeItemListModel::getPathToRoot insert cache" << absFilePath << "=" << cachePaths;

        return paths;
    } else {
        QStringList returnedPaths = m_pathToRootCache.object(absFilePath)->split(",");
//        qDebug() << "FolderSizeItemListModel::getPathToRoot returnedPaths" << returnedPaths;

        return returnedPaths;
    }
}

bool FolderSizeItemListModel::isDir(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.isDir();
}

bool FolderSizeItemListModel::isFile(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.isFile();
}

bool FolderSizeItemListModel::canCopy(const QString sourcePath, const QString targetPath)
{
    QFileInfo sourceFileInfo(sourcePath);
    QFileInfo targetFileInfo(QDir(targetPath).absoluteFilePath(sourceFileInfo.fileName()));

    return !targetFileInfo.exists();
}

QString FolderSizeItemListModel::getFileName(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.fileName();
}

QString FolderSizeItemListModel::getNewFileName(const QString absFilePath, const QString targetPath)
{
    // Get new name as *Copy.* , *Copy 2.*, ...
    QFileInfo file( QDir(targetPath).absoluteFilePath(getFileName(absFilePath)) );
    QStringList caps = splitFileName(file.absoluteFilePath());
    int copyIndex = caps.at(0).lastIndexOf("Copy");
    QString originalFileName = (copyIndex != -1) ? caps.at(0).left(copyIndex) : caps.at(0);
    QString originalFileExtension = caps.at(1);

    int i = 1;
    while (file.exists()) {
        QString newFilePath;
        if (i == 1) {
            newFilePath = originalFileName + QString("_%1").arg("Copy") + (originalFileExtension.isEmpty()?"":("."+originalFileExtension));
        } else {
            newFilePath = originalFileName + QString("_Copy%1").arg(i) + (originalFileExtension.isEmpty()?"":("."+originalFileExtension));
        }
        file = QFileInfo(newFilePath);
        i++;
    }

    return file.fileName();
}

QString FolderSizeItemListModel::getAbsolutePath(const QString dirPath, const QString fileName)
{
    QDir dir(dirPath);
    return dir.absoluteFilePath(fileName);
}

QStringList FolderSizeItemListModel::splitFileName(const QString fileName)
{
    qDebug() << "FolderSizeItemListModel::splitFileName fileName" << fileName;

    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,4})$");
    int pos = rx.indexIn(fileName);

    QStringList caps;
    if (pos > -1) {
        for(int i=0; i<=rx.captureCount(); i++) {
            qDebug() << "FolderSizeItemListModel::splitFileName caps" << i << "=" << rx.cap(i);
        }

        if (rx.captureCount() == 3) {
            caps << rx.cap(1) << rx.cap(3);
        } else {
            caps << fileName << "";
        }
    } else {
        caps << fileName << "";
    }

    return caps;
}

QString FolderSizeItemListModel::getDirContentJson(const QString dirPath)
{
    QList<FolderSizeItem> itemList;
    m.getDirContent(dirPath, itemList);
    QString jsonText = "[ ";
    for (int i=0; i<itemList.length(); i++) {
        FolderSizeItem item = itemList.at(i);
        if (i>0) jsonText.append(", ");
        jsonText.append(item.toJsonText());
    }
    jsonText.append(" ]");

    return jsonText;
}

int FolderSizeItemListModel::getIndexOnCurrentDir(const QString absFilePath)
{
    // TODO implement cache.
    int index = -1;
    if (m_indexOnCurrentDirHash.contains(absFilePath)) {
        index = m_indexOnCurrentDirHash[absFilePath];
//        qDebug() << "FolderSizeItemListModel::getIndexOnCurrentDir cached index for " << absFilePath << "=" << index;
        return index;
    }

    // Not found.
//    qDebug() << "FolderSizeItemListModel::getIndexOnCurrentDir" << absFilePath << " index cache is not found.";
    bool isOnCurrentDir = false;
    if (getDirPath(absFilePath) == currentDir()) {
        isOnCurrentDir = true;
        int i=0;
        foreach (FolderSizeItem item, getItemList()) {
            if (item.absolutePath == absFilePath) {
                index = i;
                break;
            }
            i++;
        }

    }

    index = (isOnCurrentDir && index == -1)?-2:index;
    m_indexOnCurrentDirHash[absFilePath] = index;
    qDebug() << "FolderSizeItemListModel::getIndexOnCurrentDir insert cache for" << absFilePath << "index" << index;

    return index;
}

void FolderSizeItemListModel::removeCache(const QString absPath)
{
    m.removeDirSizeCache(absPath);
}

bool FolderSizeItemListModel::isRunning()
{
    return m.isRunning();
}

void FolderSizeItemListModel::refreshItemList()
{
    emit refreshBegin();
    qDebug() << QTime::currentTime() << "FolderSizeItemListModel::refreshItemList started.";

    // Clear existing itemList.
    itemList.clear();

    // Populate dir content to itemList. m.getDirContent() also sort itemList as current sortFlag.
    m.getDirContent(currentDir(), itemList);

    qDebug() << QTime::currentTime() << "FolderSizeItemListModel::refreshItemList is done.";
    emit refreshCompleted();
}

void FolderSizeItemListModel::removeItem(const int index)
{
    itemList.removeAt(index);
}

FolderSizeItem FolderSizeItemListModel::getItem(const int index)
{
    return itemList.at(index);
}

void FolderSizeItemListModel::setItem(const int index, FolderSizeItem &item)
{
    itemList.replace(index, item);
}

void FolderSizeItemListModel::loadDirSizeCacheFinishedFilter()
{
    refreshDir();
}

void FolderSizeItemListModel::fetchDirSizeFinishedFilter()
{
    refreshItemList();
    emit refreshCompleted();
    emit fetchDirSizeFinished();
}

void FolderSizeItemListModel::copyFinishedFilter(int fileAction, QString sourcePath, QString targetPath, QString msg, int err)
{
    refreshItemList();
    emit copyFinished(fileAction, sourcePath, targetPath, msg, err);
}

void FolderSizeItemListModel::deleteFinishedFilter(QString targetPath)
{
    // Remove item from ListView.
    int i = getIndexOnCurrentDir(targetPath);

    if (i >= 0) {
        beginRemoveRows(createIndex(0,0), i, i);

        // Remote item from itemList.
        removeItem(i);

        // Emit deleteFinished
        emit deleteFinished(targetPath);

        endRemoveRows();
    }
}

void FolderSizeItemListModel::proceedNextJob()
{
    if (m.isRunning()) return;

    // Proceed next job in queue.
    qDebug() << "FolderSizeItemListModel::proceedNextJob waiting runningJobCount" << runningJobCount << "m_jobQueue" << m_jobQueue.count();
    if (runningJobCount >= MaxRunningJobCount || m_jobQueue.isEmpty()) {
        return;
    }

    // Dequeue job and create job thread with signal/slot connections.
    FolderSizeJob job = m_jobQueue.dequeue();

    // Load cache
    m.setRunMethod(job.operation);
    m.setClearCache(job.clearCache);
    m.setCopyPath(job.sourcePath, job.targetPath);
    m.start();

//    FolderSizeModelThread *t;
//    t->setRunMethod(job.operation);
//    t->setCopyPath(job.sourcePath, job.targetPath);
//    connect(&t, SIGNAL(loadDirSizeCacheFinished()), this, SLOT(postLoadSlot()) );
//    connect(&t, SIGNAL(fetchDirSizeStarted()), this, SIGNAL(fetchDirSizeStarted()) );
//    connect(&t, SIGNAL(fetchDirSizeFinished()), this, SLOT(postFetchSlot()) );
//    connect(&t, SIGNAL(fetchDirSizeUpdated(QString)), this, SIGNAL(fetchDirSizeUpdated(QString)) );
//    connect(t, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)), this, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)) );
//    connect(t, SIGNAL(copyFinished(int,QString,QString,QString,int)), this, SIGNAL(copyFinished(int,QString,QString,QString,int)) );
//    connect(t, SIGNAL(deleteFinished(QString)), this, SLOT(deleteFinishedFilter(QString)) );
//    qDebug() << "FolderSizeItemListModel::proceedNextJob t " << t->currentThreadId();
//    t->start();

    runningJobCount++;

//    qDebug() << "QThreadPool::globalInstance() active / max" << QThreadPool::globalInstance()->activeThreadCount()
//             << "/" << QThreadPool::globalInstance()->maxThreadCount() << "runningJobCount" << runningJobCount;

    emit proceedNextJobSignal();
}
