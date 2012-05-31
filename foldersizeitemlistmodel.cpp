#include "foldersizeitemlistmodel.h"
#include <QDebug>
#include <QThreadPool>
#include <QUrl>

const int FolderSizeItemListModel::TimerInterval = 500;

FolderSizeItemListModel::FolderSizeItemListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[AbsolutePathRole] = "absolutePath";
    roles[SizeRole] = "size";
    roles[LastModifiedRole] = "lastModified";
    roles[IsDirRole] = "isDir";
    roles[SubDirCountRole] = "subDirCount";
    roles[SubFileCountRole] = "subFileCount";
    setRoleNames(roles);

    // Load cache
    m.loadDirSizeCache();
}

FolderSizeItemListModel::~FolderSizeItemListModel()
{
    // Save cache
    m.saveDirSizeCache();
}

int FolderSizeItemListModel::rowCount(const QModelIndex & parent) const {
//    qDebug() << "FolderSizeItemListModel::rowCount " + QString("%1").arg(m.getDirContent().count());

    return m.getDirContent().count();
}

QVariant FolderSizeItemListModel::data(const QModelIndex & index, int role) const {
//    qDebug() << "FolderSizeItemListModel::data " + QString("%1 %2").arg(index.row()).arg(role);

    if (index.row() < 0 || index.row() > m.getDirContent().count())
        return QVariant();

    const FolderSizeItem &item = m.getDirContent().at(index.row());
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
    return QVariant();
}

QString FolderSizeItemListModel::currentDir() const
{
    return m.currentDir();
}

void FolderSizeItemListModel::setCurrentDir(const QString &path)
{
    if (isReady()) {
        m.setCurrentDir(path);

        emit currentDirChanged();
    }
    qDebug() << QTime::currentTime() << "FolderSizeItemListModel::setCurrentDir";
}

QStringList FolderSizeItemListModel::getDriveList()
{
    return m.getDriveList();
}

void FolderSizeItemListModel::loadDirSizeCache()
{
    m.loadDirSizeCache();
}

void FolderSizeItemListModel::saveDirSizeCache()
{
    m.saveDirSizeCache();
}

bool FolderSizeItemListModel::isDirSizeCacheExisting()
{
    return m.isDirSizeCacheExisting();
}

bool FolderSizeItemListModel::isReady()
{
    return m.isReady();
}

QString FolderSizeItemListModel::formatFileSize(double size)
{
    return m.formatFileSize(size);
}

void FolderSizeItemListModel::changeDir(const QString &name)
{
    if (isReady()) {
        bool isDirChanged = m.changeDir(name);
        if (isDirChanged) {
            emit currentDirChanged();

            // Invoke background refresh
            refreshDir(false);
        }
    }
}

void FolderSizeItemListModel::refreshDir(const bool clearCache)
{
    if (!clearCache && !isDirSizeCacheExisting()) {
        // If UI invoke general refresh but there is no existing cache, ask user for confirmation.
        emit requestResetCache();
    } else {
        // If UI invoke reset cache or their is existing cache, proceed as user requested.
        if (isReady()) {
            emit refreshBegin();
            m.setClearCache(clearCache);
            m.setAutoDelete(false);
            QThreadPool::globalInstance()->start(&m);

            // Set polling timer
            if (!timer) {
                timer = new QTimer();
                connect(timer, SIGNAL(timeout()), this, SLOT(checkRunnable()));
            }
            timer->start(FolderSizeItemListModel::TimerInterval);
        }
    }
}

QString FolderSizeItemListModel::getUrl(const QString absPath)
{
    return QUrl::fromLocalFile(absPath).toString();
}

bool FolderSizeItemListModel::isRoot()
{
    return m.isRoot();
}

int FolderSizeItemListModel::getSortFlag() const
{
    return m.sortFlag();
}

void FolderSizeItemListModel::setSortFlag(const int sortFlag)
{
    m.setSortFlag(sortFlag);

    emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
}

bool FolderSizeItemListModel::removeRow(int row, const QModelIndex &parent)
{
    return removeRows(row, 1, parent);
}

bool FolderSizeItemListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    bool res = true;
    for (int i = row; i < row+count; i++) {
        QFile file(data(this->index(i,0), AbsolutePathRole).toString());
        QFileInfo fileInfo(file);
        // Remove file from file system.
        bool iRes = file.remove();
        // Remove item from model's itemList.
        if (iRes) {
            m.removeItem(i);
            // Remove deleted file path cache.
            m.removeDirSizeCache(fileInfo.absolutePath());
        }

        res = res && iRes;
    }

    endRemoveRows();

    return res;
}

bool FolderSizeItemListModel::deleteFile(const QString absPath)
{
    QFile file(absPath);
    QFileInfo fileInfo(file);

    // Remove file from file system.
    bool res = file.remove();

    if (res) {
        // Remove deleted file path cache.
        m.removeDirSizeCache(fileInfo.absolutePath());
    }

    return res;
}

bool FolderSizeItemListModel::copyFile(const QString sourceAbsFilePath, const QString targetPath)
{
    QFile sourceFile(sourceAbsFilePath);
    QFileInfo sourceFileInfo(sourceAbsFilePath);
    QString targetAbsFilePath = targetPath + "/" + sourceFileInfo.fileName();

    // TODO copy with feedback progress to QML.
    bool res = sourceFile.copy(targetAbsFilePath);

    if (res) {
        // Remove target path cache.
        m.removeDirSizeCache(targetPath);
    }

    return res;
}

QString FolderSizeItemListModel::getDirPath(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.absolutePath();
}

void FolderSizeItemListModel::checkRunnable()
{
    if (m.isReady()) {
        qDebug() << QTime::currentTime() << "FolderSizeItemListModel::checkRunnable m.isReady " << m.isReady();

        timer->stop();
        m.setClearCache(false);
        emit refreshCompleted();

        reset();
        emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
    }
}

void FolderSizeItemListModel::classBegin()
{
    qDebug() <<  QTime::currentTime() << "FolderSizeItemListModel::classBegin";
}

void FolderSizeItemListModel::componentComplete()
{
    qDebug() <<  QTime::currentTime() << "FolderSizeItemListModel::componentComplete";
}

