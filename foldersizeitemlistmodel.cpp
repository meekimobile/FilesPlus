#include "foldersizeitemlistmodel.h"
#include <QScriptEngine>
#ifdef Q_OS_SYMBIAN
#include <f32file.h>
#include <e32base.h>
#include <e32debug.h>
#endif

const int FolderSizeItemListModel::TimerInterval = 100;
const int FolderSizeItemListModel::MaxRunningJobCount = 1;
#if defined(Q_WS_HARMATTAN)
const QString FolderSizeItemListModel::DefaultTrashPath = "/home/user/MyDocs/trash";
#else
const QString FolderSizeItemListModel::DefaultTrashPath = "E:/trash";
#endif

FolderSizeItemListModel::FolderSizeItemListModel(QObject *parent)
    : QAbstractListModel(parent)
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
    roles[BaseNameRole] = "baseName";
    roles[FileTypeRole] = "fileType";
    roles[IsRunningRole] = "isRunning";
    roles[RunningOperationRole] = "runningOperation";
    roles[RunningValueRole] = "runningValue";
    roles[RunningMaxValueRole] = "runningMaxValue";
    roles[IsCheckedRole] = "isChecked";
    roles[IsDirtyRole] = "isDirty";
    roles[IsHiddenRole] = "isHidden";
    roles[IsReadOnlyRole] = "isReadOnly";
    setRoleNames(roles);

    // TODO Initialize cache.
    m_pathToRootCache = new QCache<QString, QString>();
    m_indexOnCurrentDirHash = new QHash<QString, int>();

    // Connect job queue chain.
    connect(this, SIGNAL(proceedNextJobSignal()), SLOT(proceedNextJob()) );

    // Connect model class with listModel.
    connect(&m, SIGNAL(loadDirSizeCacheFinished()), this, SLOT(loadDirSizeCacheFinishedFilter()) );
    connect(&m, SIGNAL(initializeDBStarted()), this, SIGNAL(initializeDBStarted()) );
    connect(&m, SIGNAL(initializeDBFinished()), this, SIGNAL(initializeDBFinished()) );
    connect(&m, SIGNAL(fetchDirSizeStarted()), this, SIGNAL(fetchDirSizeStarted()) );
    connect(&m, SIGNAL(fetchDirSizeFinished()), this, SLOT(fetchDirSizeFinishedFilter()) );
    connect(&m, SIGNAL(copyStarted(int,QString,QString,QString,int)), this, SIGNAL(copyStarted(int,QString,QString,QString,int)) );
    connect(&m, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)), this, SLOT(copyProgressFilter(int,QString,QString,qint64,qint64)) , Qt::QueuedConnection);
    connect(&m, SIGNAL(copyFinished(int,QString,QString,QString,int,qint64,qint64,qint64,bool)), this, SLOT(copyFinishedFilter(int,QString,QString,QString,int,qint64,qint64,qint64,bool)) );
    connect(&m, SIGNAL(fetchDirSizeUpdated(QString)), this, SIGNAL(fetchDirSizeUpdated(QString)) );
    connect(&m, SIGNAL(deleteStarted(int,QString)), this, SIGNAL(deleteStarted(int,QString)) );
    connect(&m, SIGNAL(deleteProgress(int,QString,QString,int)), this, SLOT(deleteProgressFilter(int,QString,QString,int)) );
    connect(&m, SIGNAL(deleteFinished(int,QString,QString,int)), this, SLOT(deleteFinishedFilter(int,QString,QString,int)) );
    connect(&m, SIGNAL(trashFinished(int,QString,QString,QString,int)), this, SLOT(trashFinishedFilter(int,QString,QString,QString,int)) );
    connect(&m, SIGNAL(finished()), this, SLOT(jobDone()) );

    // Enqueue jobs. Queued jobs will proceed after folderPage is loaded.
    m_jobQueue.enqueue(FolderSizeJob(createNonce(), FolderSizeModelThread::LoadDirSizeCache, "", ""));
    m_jobQueue.enqueue(FolderSizeJob(createNonce(), FolderSizeModelThread::InitializeDB, "", ""));

    // Initialize FS watcher.
    initializeFSWatcher();
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
    else if (role == BaseNameRole)
        return item.name.mid(0, item.name.lastIndexOf("."));
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
    else if (role == IsDirtyRole)
        return item.isDirty;
    else if (role == IsHiddenRole)
        return item.isHidden;
    else if (role == IsReadOnlyRole)
        return item.isReadOnly;
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

    // Reset m_indexOnCurrentDirHash.
    m_indexOnCurrentDirHash->clear();

    // Invoke background refresh by clearing items while refreshing.
    refreshDir("FolderSizeItemListModel::setCurrentDir", false, true);

//    qDebug() << QTime::currentTime() << "FolderSizeItemListModel::setCurrentDir";
}

QStringList FolderSizeItemListModel::getDriveList()
{
    QStringList driveList;

#if defined(Q_WS_SIMULATOR)
    qDebug() << "FolderSizeItemListModel::getDriveList platform=Q_WS_SIMULATOR";
    QFileInfoList drives = QDir::drives();
    for (int i=0; i<drives.length(); i++) {
        qDebug() << "FolderSizeItemListModel::getDriveList drives" << drives.at(i).absoluteFilePath();
        driveList.append(drives.at(i).absoluteFilePath());
    }
#elif defined(Q_OS_SYMBIAN)
    qDebug() << "FolderSizeItemListModel::getDriveList platform=Q_OS_SYMBIAN";
    QFileInfoList drives = QDir::drives();
    for (int i=0; i<drives.length(); i++) {
        qDebug() << "FolderSizeItemListModel::getDriveList drives" << drives.at(i).absoluteFilePath();
        driveList.append(drives.at(i).absoluteFilePath());
    }
#endif

    return driveList;
}

QStringList FolderSizeItemListModel::getLogicalDriveList() {
    if (!m_driveList.isEmpty()) return m_driveList;

    QSystemStorageInfo storageInfo;
    QStringList simulatedDriveNames = getDriveList();
    QStringList drives = storageInfo.logicalDrives();

    for (int i=0; i<drives.count(); i++)
    {
        // Workaround for QtSimulator
        QString driveName = drives.at(i);
        if (i < simulatedDriveNames.count()) {
            driveName = simulatedDriveNames.at(i);
        }

        m_driveList << driveName;
    }

    return m_driveList;
}

bool FolderSizeItemListModel::isDirSizeCacheExisting()
{
    return m.isDirSizeCacheExisting();
}

bool FolderSizeItemListModel::isReady()
{
    return !m.isRunning();
}

QVariant FolderSizeItemListModel::get(const int index)
{
    if (index >= 0 && index < rowCount()) {
        QMap<QString,QVariant> jsonObj;
        foreach(int role, roleNames().keys()) {
            QString propertyName = QString(roleNames().value(role));
            QVariant propertyValue = data(createIndex(index,0), role);
            jsonObj[propertyName] = propertyValue;
        }
//        qDebug() << "FolderSizeItemListModel::get" << QVariant(jsonObj);
        return QVariant(jsonObj);
    }

    return QVariant();
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
    bool isChanged = false;
    if (index >= 0 && index < rowCount()) {
        // Update to limited properties.
        FolderSizeItem item = getItem(index);
        if (role == IsRunningRole && item.isRunning != value.toBool()) {
            item.isRunning = value.toBool();
            isChanged = true;
        } else if (role == RunningOperationRole && item.runningOperation != value.toInt()) {
            item.runningOperation = value.toInt();
            isChanged = true;
        } else if (role == RunningValueRole && item.runningValue != value.toLongLong()) {
            item.runningValue = value.toLongLong();
            isChanged = true;
        } else if (role == RunningMaxValueRole && item.runningMaxValue != value.toLongLong()) {
            item.runningMaxValue = value.toLongLong();
            isChanged = true;
        } else if (role == IsCheckedRole && item.isChecked != value.toBool()) {
            item.isChecked = value.toBool();
            isChanged = true;
        } else if (role == IsDirtyRole && item.isDirty != value.toBool()) {
            item.isDirty = value.toBool();
            isChanged = true;
        }

        if (isChanged) {
//            qDebug() << "FolderSizeItemListModel::setProperty" << index << roleNames().value(role) << value << "isChanged" << isChanged;
            setItem(index, item);
            refreshItem(index);
        }
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
        // Disable because it causes too much load on UI.
//        refreshItems();
    }
}

void FolderSizeItemListModel::setProperty(const int index, QVariant valueJson)
{
    qDebug() << "FolderSizeItemListModel::setProperty" << index << valueJson << "(not implemented)";
    // TODO Use roleNames.key(roleName) to get role. But it's needed to map and set as in existing setProperty method.
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

void FolderSizeItemListModel::cancelQueuedJobs()
{
    qDebug() << "FolderSizeItemListModel::cancelQueuedJobs";
    m_jobQueue.clear();
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
    qDebug() << "FolderSizeItemListModel::jobDone started runningJobCount" << runningJobCount << "m_jobQueue" << m_jobQueue.count() << "m.isFinished()" << m.isFinished();
    if (!m.isFinished()) {
        m.wait();
    }

    if (runningJobCount > 0) {
        mutex.lock();
        runningJobCount--;
        emit runningJobCountChanged();
        mutex.unlock();
    }

    qDebug() << "FolderSizeItemListModel::jobDone finished runningJobCount" << runningJobCount << "m_jobQueue" << m_jobQueue.count() << "m.isFinished()" << m.isFinished();

    emit proceedNextJobSignal();
}

void FolderSizeItemListModel::fsWatcherDirectoryChangedSlot(const QString &entry)
{
    qDebug() << "FolderSizeItemListModel::fsWatcherDirectoryChangedSlot changed entry" << entry;

    // Remove folder cache to force refresh.
    removeCache(entry);

    // Emit directory changed.
    emit directoryChanged(entry);
}

void FolderSizeItemListModel::changeDir(const QString &name, const int sortFlag)
{
    // m.changeDir() reuses m.setCurrentDir().
    bool isDirChanged = m.changeDir(name, sortFlag);

    if (isDirChanged) {
        // Emit to update currentDir on UI.
        emit currentDirChanged();

        // Reset m_indexOnCurrentDirHash.
        m_indexOnCurrentDirHash->clear();

        // Invoke background refresh
        refreshDir("FolderSizeItemListModel::changeDir");
    }
}

void FolderSizeItemListModel::refreshDir(const QString caller, const bool clearCache, const bool clearItems)
{
    qDebug() << "FolderSizeItemListModel::refreshDir clearCache" << clearCache << "sender" << sender() << "caller" << caller << "currentDir" << currentDir();

    if (currentDir() == "") {
        qDebug() << "FolderSizeItemListModel::refreshDir currentDir is empty. Ignore request.";
        return;
    }

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

        // Clear items while refreshing.
        if (clearItems) {
            itemList.clear();
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
    return isRoot(m.currentDir());
}

bool FolderSizeItemListModel::isRoot(const QString absPath)
{
    if (absPath == "") return true;

    // TODO Impl. Caching.
    QDir dir(absPath);

    QStringList driveList = getLogicalDriveList();
#if defined(Q_OS_SYMBIAN)
    driveList.append(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
#endif
    bool isRootLogicalDrive = driveList.contains(absPath);
//    qDebug() << "FolderSizeItemListModel::isRoot path" << absPath << "dir.isRoot()" << dir.isRoot() << "driveList" << driveList << "isRootLogicalDrive" << isRootLogicalDrive;

    return (dir.isRoot() || isRootLogicalDrive);
}

QString FolderSizeItemListModel::getItemJson(const QString absFilePath)
{
    return m.getItem(QFileInfo(absFilePath)).toJsonText();
}

int FolderSizeItemListModel::getSortFlag() const
{
    return m.sortFlag();
}

void FolderSizeItemListModel::setSortFlag(const int sortFlag, const bool saveSortFlag)
{
    if (m.setSortFlag(sortFlag, saveSortFlag)) {
        m.sortItemList(itemList);

        // Reset m_indexOnCurrentDirHash.
        m_indexOnCurrentDirHash->clear();

        // If itemList is actually sorted, refreshItems to emit dataChanged.
        refreshItems();
    }
}

void FolderSizeItemListModel::revertSortFlag()
{
    setSortFlag(m.getSortFlagFromDB(m.currentDir(), m.sortFlag()));
}

QStringList FolderSizeItemListModel::getNameFilters() const
{
    return m.nameFilters();
}

void FolderSizeItemListModel::setNameFilters(const QStringList nameFilters)
{
    m.setNameFilters(nameFilters);

    // Reset m_indexOnCurrentDirHash.
    if (m_indexOnCurrentDirHash != 0) {
        m_indexOnCurrentDirHash->clear();
    }
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
//    qDebug() << "FolderSizeItemListModel::refreshItem localPath" << localPath << "index" << index;
    if (index == IndexOnCurrentDirButNotFound) {
        m_indexOnCurrentDirHash->remove(localPath);
        index = getIndexOnCurrentDir(localPath);
    }
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
    beginRemoveRows(parent, row, row + count - 1);

    for (int i = row; i < row+count; i++) {
        removeItem(i);
    }

    endRemoveRows();

    return true;
}

bool FolderSizeItemListModel::deleteFile(const QString absPath)
{
    if (m_settings.value("FolderSizeItemListModel.deleteFile.use.trash.enabled", false).toBool()) {
        if (!absPath.startsWith(getTrashPath())) {
            return trash(absPath);
        }
    }

    // Enqueue job.
    FolderSizeJob job(createNonce(), FolderSizeModelThread::DeleteFile, absPath, "");
    m_jobQueue.enqueue(job);

    emit proceedNextJobSignal();

    return true;
}

bool FolderSizeItemListModel::copy(const QString sourcePath, const QString targetPath)
{
    if (sourcePath == targetPath) {
        emit copyFinished(FolderSizeModelThread::CopyFile, sourcePath, targetPath, tr("Source and Target path can't be the same."), -5, 0, -1, 0);
        return false;
    } else if (targetPath.indexOf(sourcePath + "/") != -1) {
        emit copyFinished(FolderSizeModelThread::CopyFile, sourcePath, targetPath, tr("Target path can't be inside source path."), -6, 0, -1, 0);
        return false;
    }

    // TODO Show running on targetPath's parent.
    emit copyStarted(FolderSizeModelThread::CopyFile, sourcePath, getDirPath(targetPath), tr("Show running on targetPath's parent"), 0);

    // Enqueue job.
    FolderSizeJob job(createNonce(), FolderSizeModelThread::CopyFile, sourcePath, targetPath);
    m_jobQueue.enqueue(job);

    emit proceedNextJobSignal();

    return true;
}

bool FolderSizeItemListModel::move(const QString sourcePath, const QString targetPath)
{
    if (sourcePath == targetPath) {
        emit copyFinished(FolderSizeModelThread::MoveFile, sourcePath, targetPath, tr("Source and Target path can't be the same."), -5, 0, -1, 0);
        return false;
    } else if (targetPath.indexOf(sourcePath + "/") != -1) {
        emit copyFinished(FolderSizeModelThread::MoveFile, sourcePath, targetPath, tr("Target path can't be inside source path."), -6, 0, -1, 0);
        return false;
    }

    // TODO Show running on targetPath's parent.
    emit copyStarted(FolderSizeModelThread::MoveFile, sourcePath, getDirPath(targetPath), tr("Show running on targetPath's parent"), 0);

    // Enqueue job.
    FolderSizeJob job(createNonce(), FolderSizeModelThread::MoveFile, sourcePath, targetPath);
    m_jobQueue.enqueue(job);

    emit proceedNextJobSignal();

    return true;
}

bool FolderSizeItemListModel::createDir(const QString name)
{
    if (name.trimmed().isEmpty()) return false;

    QDir dir(currentDir());
    bool res = dir.mkdir(name);
    if (res) {
        emit createFinished(dir.absoluteFilePath(name), tr("Create %1 done.").arg(name), 0);
    } else {
        emit createFinished(currentDir() + "/" + name, tr("Create %1 failed.").arg(name), -1);
    }
    return res;
}

bool FolderSizeItemListModel::createDirPath(const QString absPath)
{
    if (absPath.trimmed().isEmpty()) return false;

    QDir dir(getDirPath(absPath));
    bool res = dir.mkdir(getFileName(absPath));
    if (res) {
        emit createFinished(absPath, tr("Create %1 done.").arg(absPath), 0);
    } else {
        emit createFinished(absPath, tr("Create %1 failed.").arg(absPath), -1);
    }
    return res;
}

bool FolderSizeItemListModel::createEmptyFile(const QString name)
{
    if (name.trimmed().isEmpty()) return false;

    QDir dir(currentDir());

    QString absPath = dir.absoluteFilePath(name);
    qint64 c = -1;
    QFile file(absPath);
    if (file.open(QIODevice::WriteOnly)) {
        qDebug() << "FolderSizeItemListModel::createEmptyFile open file" << absPath << "for write.";
        c = file.write("");
    }
    file.close();

    qDebug() << "FolderSizeItemListModel::createEmptyFile file" << absPath << "size" << c;
    if (c != -1) {
        emit createFinished(dir.absoluteFilePath(name), tr("Create %1 done.").arg(name), 0);
        return true;
    } else {
        emit createFinished(currentDir() + "/" + name, tr("Create %1 failed.").arg(name), -1);
        return false;
    }
}

bool FolderSizeItemListModel::renameFile(const QString fileName, const QString newFileName)
{
    if (fileName == newFileName) return false;

    QDir::setCurrent(currentDir());
    QFileInfo sourceFileInfo(fileName);
    int res = false;
    if (sourceFileInfo.isFile()) {
        res = QFile::rename(fileName, newFileName);
    } else {
        res = QDir::current().rename(fileName, newFileName);
    }

    qDebug() << "FolderSizeItemListModel::renameFile res" << res << "fileName" << fileName << "newFileName" << newFileName;
    if (res) {
        // TODO Move cache and its sub items cache to new names.

        // Emit signal to change CloudDriveItem.
        emit renameFinished( QDir::current().absoluteFilePath(fileName), QDir::current().absoluteFilePath(newFileName), tr("Rename %1 to %2 done.").arg(fileName).arg(newFileName), 0);
    } else {
        // Emit signal to change CloudDriveItem.
        emit renameFinished( QDir::current().absoluteFilePath(fileName), QDir::current().absoluteFilePath(newFileName), tr("Rename %1 to %2 failed.").arg(fileName).arg(newFileName), -1);
    }

    return res;
}

QString FolderSizeItemListModel::getTrashPath()
{
    return m_settings.value("FolderSizeItemListModel.trashPath", DefaultTrashPath).toString();
}

bool FolderSizeItemListModel::createTrashIfNotExists()
{
    QString trashPath = m_settings.value("FolderSizeItemListModel.trashPath", DefaultTrashPath).toString();
    if (!QFileInfo(trashPath).isDir()) {
        if (QDir::home().mkpath(trashPath)) {
            qDebug() << "FolderSizeItemListModel::createTrashIfNotExists" << trashPath << "is created.";
        } else {
            qDebug() << "FolderSizeItemListModel::createTrashIfNotExists" << trashPath << "can't be created.";
            return false;
        }
    }

    return true;
}

QString FolderSizeItemListModel::getTrashJsonText()
{
    if (!createTrashIfNotExists()) {
        return "";
    }

    QString trashPath = m_settings.value("FolderSizeItemListModel.trashPath", DefaultTrashPath).toString();

    return getItemJson(trashPath);
}

qlonglong FolderSizeItemListModel::getMaxTrashSize()
{
    QString trashPath = m_settings.value("FolderSizeItemListModel.trashPath", DefaultTrashPath).toString();
    float maxTrashSizeRatio = m_settings.value("FolderSizeItemListModel.maxTrashSizeRatio", 0.1).toFloat();
    QSystemStorageInfo storageInfo;
    QString logicalDrive = getRoot(trashPath);
#ifdef Q_OS_SYMBIAN
    logicalDrive = logicalDrive.mid(0, 1);
#endif

    return storageInfo.totalDiskSpace(logicalDrive) * maxTrashSizeRatio;
}

bool FolderSizeItemListModel::trash(const QString sourcePath)
{
    if (!createTrashIfNotExists()) {
        return false;
    }

    // TODO Make it restorable by storing to canonical path.
    QString trashPath = m_settings.value("FolderSizeItemListModel.trashPath", DefaultTrashPath).toString();
    QString targetPath = QDir(trashPath).absoluteFilePath(QFileInfo(sourcePath).fileName());
    qDebug() << "FolderSizeItemListModel::trash moving" << sourcePath << "to" << targetPath;

    if (sourcePath == targetPath) {
        qDebug() << "FolderSizeItemListModel::trash error Source and Target path can't be the same.";
        return false;
    } else if (targetPath.indexOf(sourcePath + "/") != -1) {
        qDebug() << "FolderSizeItemListModel::trash error Target path can't be inside source path.";
        return false;
    }

    // Enqueue job.
    FolderSizeJob job(createNonce(), FolderSizeModelThread::TrashFile, sourcePath, targetPath);
    m_jobQueue.enqueue(job);

    emit proceedNextJobSignal();

    return true;
}

void FolderSizeItemListModel::requestTrashStatus()
{
    emit trashChanged();
}

void FolderSizeItemListModel::emptyTrash()
{
    QDir trashDir(getTrashPath());
    trashDir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    suspendNextJob();
    foreach (QString childName, trashDir.entryList()) {
        QApplication::processEvents();

        QString childPath = trashDir.absoluteFilePath(childName);
        qDebug() << "FolderSizeItemListModel::emptyTrash delete childPath" << childPath;
        deleteFile(childPath);
    }
    resumeNextJob();
}

bool FolderSizeItemListModel::setFileAttribute(QString localFilePath, FileAttribute attribute, bool value)
{
#ifdef Q_OS_SYMBIAN
    QString nativeLocalFilePath = QDir::toNativeSeparators(localFilePath);

    TPtrC pLocalFilePath (static_cast<const TUint16*>(nativeLocalFilePath.utf16()), nativeLocalFilePath.length());
    TBuf<100> buf;
    buf.Copy(pLocalFilePath);

    RFs fs;
    TInt res;
    TUint postAtts;
    fs.Connect();

    // NOTE RFs::SetAtt() must use path with native separator.
    if (attribute == ReadOnly) {
//        qDebug() << "FolderSizeItemListModel::setFileAttribute ReadOnly" << localFilePath << value;
        if (value) {
            res = fs.SetAtt(buf, KEntryAttReadOnly | KEntryAttNormal, KEntryAttNormal);
        } else {
            res = fs.SetAtt(buf, KEntryAttNormal, KEntryAttReadOnly);
        }
    } else if (attribute == Hidden) {
//        qDebug() << "FolderSizeItemListModel::setFileAttribute Hidden" << localFilePath << value;
        if (value) {
            res = fs.SetAtt(buf, KEntryAttHidden, KEntryAttNormal);
        } else {
            res = fs.SetAtt(buf, KEntryAttNormal, KEntryAttHidden);
        }
    }
    fs.Att(buf, postAtts);
    RDebug::Print(_L("FolderSizeItemListModel::setFileAttribute %S %d %04x"), &buf, res, postAtts);
    fs.Close();

    return (res >= 0);
#endif
    return false;
}

QString FolderSizeItemListModel::getDirPath(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.absolutePath();
}

QStringList FolderSizeItemListModel::getPathToRoot(const QString absFilePath)
{
//    qDebug() << "FolderSizeItemListModel::getPathToRoot" << absFilePath;
    QStringList paths;

    // Return empty list if absFilePath is not absolute path.
    if (!QDir::isAbsolutePath(absFilePath)) {
        return paths;
    }

    // Try get from cache. It will not 0 if exists.
//    qDebug() << "FolderSizeItemListModel::getPathToRoot absFilePath" << absFilePath << "cache" << m_pathToRootCache->contains(absFilePath);
    if (!m_pathToRootCache->contains(absFilePath)) {
        QDir dir(absFilePath);

        while (!isRoot(dir.absolutePath())) {
            paths << dir.absolutePath();
//            qDebug() << "FolderSizeItemListModel::getPathToRoot append" << dir.absolutePath();
            dir.cdUp();
        }
        paths << dir.absolutePath();
//        qDebug() << "FolderSizeItemListModel::getPathToRoot append" << dir.absolutePath();

        // Insert to cache.
        QString *cachePaths = new QString(paths.join("|"));
        m_pathToRootCache->insert(absFilePath, cachePaths);
//        qDebug() << "FolderSizeItemListModel::getPathToRoot insert cache" << absFilePath << "=" << *cachePaths;

        return paths;
    } else {
        QStringList returnedPaths = m_pathToRootCache->object(absFilePath)->split("|");
//        qDebug() << "FolderSizeItemListModel::getPathToRoot returnedPaths" << returnedPaths;

        return returnedPaths;
    }
}

QString FolderSizeItemListModel::getRoot(const QString absFilePath)
{
    QStringList paths = getPathToRoot(absFilePath);

    return paths.last();
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

QString FolderSizeItemListModel::getNewFileName(const QString fileName, const QString targetPath)
{
    // Get new name as *Copy.* , *Copy 2.*, ...
    QFileInfo file( QDir(targetPath).absoluteFilePath(fileName) );
    QStringList caps = splitFileName(file.absoluteFilePath());
//    qDebug() << "FolderSizeItemListModel::getNewFileName" << absFilePath << targetPath << caps;
    int copyIndex = caps.at(0).indexOf(tr("_Copy"), caps.at(0).lastIndexOf("/"));
    QString originalFileName = (copyIndex != -1) ? caps.at(0).left(copyIndex) : caps.at(0);
    QString originalFileExtension = caps.at(1);
//    qDebug() << "FolderSizeItemListModel::getNewFileName" << copyIndex << originalFileName << originalFileExtension;

    int i = 1;
    while (file.exists()) {
        QString newFilePath;
        if (i == 1) {
            newFilePath = originalFileName + tr("_Copy") + (originalFileExtension.isEmpty()?"":("."+originalFileExtension));
        } else {
            newFilePath = originalFileName + tr("_Copy%1").arg(i) + (originalFileExtension.isEmpty()?"":("."+originalFileExtension));
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

QStringList FolderSizeItemListModel::findSubDirList(QString dirPath)
{
    QStringList subDirList;
    subDirList.append(dirPath);

    QDir dir(dirPath);
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    foreach (QFileInfo subDirInfo, dir.entryInfoList()) {
        subDirList.append(findSubDirList(subDirInfo.absoluteFilePath()));
    }
    qDebug() << "FolderSizeItemListModel::findSubDirList dirPath" << dirPath << "subDirList" << subDirList;
    return subDirList;
}

void FolderSizeItemListModel::initializeFSWatcher()
{
    // TODO Make watched paths configurable.
    m_fsWatcher = new QFileSystemWatcher(this);
    m_fsWatcher->addPath( QDesktopServices::storageLocation( QDesktopServices::PicturesLocation ) );
    m_fsWatcher->addPath( QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) );
    m_fsWatcher->addPath( QDesktopServices::storageLocation( QDesktopServices::MoviesLocation ) );
#ifdef Q_OS_SYMBIAN
//    m_fsWatcher->addPath( QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation ) ); // It's E:/ on Symbian which is too wide scope.
    m_fsWatcher->addPaths(findSubDirList("E:/DCIM")); // Captured images location for Symbian.
    m_fsWatcher->addPaths(findSubDirList("E:/temp")); // Temp location for Symbian.
#elif defined(Q_WS_HARMATTAN)
    m_fsWatcher->addPath( QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation ) );
    m_fsWatcher->addPaths(findSubDirList("/home/user/MyDocs/DCIM")); // Captured images location for Meego.
    m_fsWatcher->addPaths(findSubDirList("/home/user/MyDocs/temp")); // Temp location for Meego.
#endif

    qDebug() << "FolderSizeItemListModel::initializeFSWatcher watched directories" << m_fsWatcher->directories();

    connect(m_fsWatcher, SIGNAL(directoryChanged(QString)), SLOT(fsWatcherDirectoryChangedSlot(QString)) );
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
    int index = IndexNotOnCurrentDir;
    if (m_indexOnCurrentDirHash->contains(absFilePath)) {
        index = m_indexOnCurrentDirHash->value(absFilePath);
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
            // Update index cache while looping.
            m_indexOnCurrentDirHash->insert(item.absolutePath, i);
            i++;
        }
    }

    index = (isOnCurrentDir && index == IndexNotOnCurrentDir)?IndexOnCurrentDirButNotFound:index;
//    m_indexOnCurrentDirHash->insert(absFilePath, index);
//    qDebug() << "FolderSizeItemListModel::getIndexOnCurrentDir insert cache for" << absFilePath << "index" << index;

    return index;
}

void FolderSizeItemListModel::clearIndexOnCurrentDir()
{
    m_indexOnCurrentDirHash->clear();
}

void FolderSizeItemListModel::refreshIndexOnCurrentDir()
{
    m_indexOnCurrentDirHash->clear();
    int i = 0;
    foreach (FolderSizeItem item, itemList) {
        m_indexOnCurrentDirHash->insert(item.absolutePath, i++);
    }
}

void FolderSizeItemListModel::removeCache(const QString absPath, bool removeAll)
{
    // Remove cache up to root by utilizing cache in getPathToRoot().
    foreach (QString path, getPathToRoot(absPath)) {
        if (removeAll) {
            m.removeDirSizeCache(path);
        } else if (QFileInfo(path).isDir() && m.removeDirSizeCache(path) == 0) {
            qDebug() << "FolderSizeItemListModel::removeCache cache of" << path << "is already removed. Operation is ignored.";
            break;
        }
    }
}

bool FolderSizeItemListModel::isRunning()
{
    return m.isRunning();
}

void FolderSizeItemListModel::refreshItemList()
{
    emit refreshBegin();
    qDebug() << QTime::currentTime() << "FolderSizeItemListModel::refreshItemList started.";

    mutex.lock();

    // Clear existing itemList.
    itemList.clear();

    mutex.unlock();

    // Populate dir content to itemList. m.getDirContent() also sort itemList as current sortFlag.
    m.getDirContent(currentDir(), itemList);

    refreshIndexOnCurrentDir();

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

int FolderSizeItemListModel::addItem(const QString absPath)
{
    QFileInfo item(absPath);
    qDebug() << "FolderSizeItemListModel::addItem" << item.absoluteFilePath() << item.isDir() << item.isFile();
    itemList.append(m.getItem(item));
    int index  = itemList.count()-1;
    m_indexOnCurrentDirHash->insert(absPath, index);
    refreshItems();

    return index;
}

void FolderSizeItemListModel::loadDirSizeCacheFinishedFilter()
{
    emit loadDirSizeCacheFinished();

    refreshDir("FolderSizeItemListModel::loadDirSizeCacheFinishedFilter");
}

void FolderSizeItemListModel::fetchDirSizeFinishedFilter()
{
    refreshItemList();
    // Emited in refreshItemList() already.
//    emit refreshCompleted();
    emit fetchDirSizeFinished();
}

void FolderSizeItemListModel::copyProgressFilter(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal)
{
    // TODO Suppress some overflow events.

    emit copyProgress(fileAction, sourcePath, targetPath, bytes, bytesTotal);
}

void FolderSizeItemListModel::copyFinishedFilter(int fileAction, QString sourcePath, QString targetPath, QString msg, int err, qint64 bytes, qint64 totalBytes, qint64 count, bool isSourceRoot)
{    
    // Remove item from ListView.
    if (fileAction == MoveFile) {
        if (err >= 0) {
            int i = getIndexOnCurrentDir(sourcePath);

            if (i >= 0) {
                // Remove item from model.
                removeRow(i);

                // Reset m_indexOnCurrentDirHash.
                m_indexOnCurrentDirHash->clear();
            }
        }

        // Remove cache of SOURCE path up to root.
        if (isSourceRoot) {
            removeCache(sourcePath);
        }
    }

    // Remove cache of TARGET path up to root.
    if (isSourceRoot) {
        removeCache(targetPath);
    }

    emit copyFinished(fileAction, sourcePath, targetPath, msg, err, bytes, totalBytes, count, isSourceRoot);
}

void FolderSizeItemListModel::deleteProgressFilter(int fileAction, QString sourceSubPath, QString msg, int err)
{
    // TODO Suppress some overflow events.

//    qDebug() << "FolderSizeItemListModel::deleteProgressFilter" << sourceSubPath;
    if (err >= 0) {
        int i = getIndexOnCurrentDir(sourceSubPath);

        if (i >= 0) {
            // Remove item from model.
            removeRow(i);

            // Reset m_indexOnCurrentDirHash.
            m_indexOnCurrentDirHash->clear();
        }
    }

    // NOTE Not require as deleteFinishedFilter have done.
    // Remove cache of path up to root.
//    removeCache(sourceSubPath);

    // Emit deleteFinished
    emit deleteProgress(fileAction, sourceSubPath, msg, err);

//    qDebug() << "FolderSizeItemListModel::deleteProgressFilter" << sourceSubPath << "is done.";
}

void FolderSizeItemListModel::deleteFinishedFilter(int fileAction, QString sourcePath, QString msg, int err)
{
//    qDebug() << "FolderSizeItemListModel::deleteFinishedFilter" << sourcePath;

    // NOTE Not require as deleteProgressFilter have done.
    // Remove item from ListView.

    // Remove cache of path up to root.
    removeCache(sourcePath);

    // Emit deleteFinished
    emit deleteFinished(fileAction, sourcePath, msg, err);

    //    qDebug() << "FolderSizeItemListModel::deleteFinishedFilter" << sourcePath << "is done.";
}

void FolderSizeItemListModel::trashFinishedFilter(int fileAction, QString sourcePath, QString targetPath, QString msg, int err)
{
    if (err >= 0) {
        // Remove item from ListView.
        int i = getIndexOnCurrentDir(sourcePath);
//        qDebug() << "FolderSizeItemListModel::trashFinishedFilter" << sourcePath << "is at index" << i;

        if (i >= 0) {
            // Remove item from model.
            removeRow(i);

            // Reset m_indexOnCurrentDirHash.
            m_indexOnCurrentDirHash->clear();
        }

        // Remove cache of SOURCE path up to root.
        removeCache(sourcePath);

        // Remove cache of TARGET path up to root.
        removeCache(targetPath);
    }

    emit trashFinished(fileAction, sourcePath, targetPath, msg, err);
}

void FolderSizeItemListModel::proceedNextJob()
{
    if (m.isRunning()) return;

    // Proceed next job in queue.
    qDebug() << "FolderSizeItemListModel::proceedNextJob waiting runningJobCount" << runningJobCount << "MaxRunningJobCount" << MaxRunningJobCount << "m_jobQueue" << m_jobQueue.count() << (runningJobCount >= MaxRunningJobCount || m_jobQueue.count() <= 0 || m_isSuspended);
    if (runningJobCount >= MaxRunningJobCount || m_jobQueue.count() <= 0 || m_isSuspended) {
        return;
    }

    // Dequeue job and create job thread with signal/slot connections.
    FolderSizeJob job = m_jobQueue.dequeue();

    // Load cache
    m.setRunMethod(job.operation);
    m.setClearCache(job.clearCache);
    m.setSourcePath(job.sourcePath);
    m.setTargetPath(job.targetPath);
    m.start(QThread::LowPriority);

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

    mutex.lock();
    runningJobCount++;
    emit runningJobCountChanged();
    mutex.unlock();

    qDebug() << "FolderSizeItemListModel::proceedNextJob start job" << job.jobId << "runningJobCount" << runningJobCount << "m_jobQueue" << m_jobQueue.count();

//    qDebug() << "QThreadPool::globalInstance() active / max" << QThreadPool::globalInstance()->activeThreadCount()
//             << "/" << QThreadPool::globalInstance()->maxThreadCount() << "runningJobCount" << runningJobCount;

    emit proceedNextJobSignal();
}

void FolderSizeItemListModel::suspendNextJob()
{
    m_isSuspended = true;
}

void FolderSizeItemListModel::resumeNextJob()
{
    m_isSuspended = false;

    proceedNextJob();
}


int FolderSizeItemListModel::getQueuedJobCount() const
{
    return m_jobQueue.count();
}

int FolderSizeItemListModel::getRunningJobCount() const
{
    return runningJobCount;
}

void FolderSizeItemListModel::abortThread(bool rollbackFlag)
{
    m.setAbortFlag(true);
    m.setRollbackFlag(rollbackFlag);
}
