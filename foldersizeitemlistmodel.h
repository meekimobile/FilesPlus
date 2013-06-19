#ifndef FOLDERSIZEITEMLISTMODEL_H
#define FOLDERSIZEITEMLISTMODEL_H

#include <QAbstractListModel>
#include <QTimer>
#include <QtCore>
#include <QDebug>
#include <QThreadPool>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QDesktopServices>
#include <qsystemstorageinfo.h>
#include "foldersizeitem.h"
#include "foldersizemodelthread.h"
#include "foldersizejob.h"

using namespace QtMobility;

class FolderSizeItemListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(SortFlags)
    Q_ENUMS(RunnableMethods)
    Q_ENUMS(FolderSizeItemRoles)
    Q_ENUMS(RunningOperations)
    Q_ENUMS(IndexOnCurrentDir)
    Q_ENUMS(FileAttribute)
    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(int sortFlag READ getSortFlag WRITE setSortFlag NOTIFY sortFlagChanged)
    Q_PROPERTY(int count READ rowCount)
    Q_PROPERTY(QStringList nameFilters READ getNameFilters WRITE setNameFilters)
    Q_PROPERTY(int runningJobCount READ getRunningJobCount NOTIFY runningJobCountChanged)
    Q_PROPERTY(int running READ isRunning NOTIFY isRunningChanged)
public:
    static const int TimerInterval;
    static const int MaxRunningJobCount;
    static const QString DefaultTrashPath;

    enum SortFlags {
        SortByName,
        SortByTime,
        SortBySize,
        SortByType,
        SortByNameWithDirectoryFirst
    };

    // NOTE Always needs to be the same as FolderSizeModelThread.
    enum RunnableMethods {
        FetchDirSize,
        LoadDirSizeCache,
        CopyFile,
        MoveFile,
        DeleteFile,
        InitializeDB,
        TrashFile
    };

    enum FolderSizeItemRoles {
        NameRole = Qt::UserRole + 1,
        AbsolutePathRole,
        SizeRole,
        LastModifiedRole,
        IsDirRole,
        SubDirCountRole,
        SubFileCountRole,
        BaseNameRole,
        FileTypeRole,
        IsRunningRole,
        RunningOperationRole,
        RunningValueRole,
        RunningMaxValueRole,
        IsCheckedRole,
        IsDirtyRole,
        IsHiddenRole,
        IsReadOnlyRole
    };

    enum RunningOperations {
        NoOperation,
        ReadOperation,
        WriteOperation,
        SyncOperation,
        UploadOperation,
        DownloadOperation
    };

    enum IndexOnCurrentDir {
        IndexNotOnCurrentDir = 100001,
        IndexOnCurrentDirButNotFound = 100002
    };

    enum FileAttribute {
        Hidden,
        System,
        ReadOnly
    };

    explicit FolderSizeItemListModel(QObject *parent = 0);
    ~FolderSizeItemListModel();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    QString currentDir() const;
    void setCurrentDir(const QString &path);
    int getSortFlag() const;
    Q_INVOKABLE bool setSortFlag(const int sortFlag, const bool saveSortFlag = true);
    Q_INVOKABLE bool revertSortFlag();
    QStringList getNameFilters() const;
    void setNameFilters(const QStringList nameFilters);   
    Q_INVOKABLE QVariant get(const int index);
    Q_INVOKABLE QVariant getProperty(const int index, FolderSizeItemRoles role);
    Q_INVOKABLE void setProperty(const int index, FolderSizeItemRoles role, QVariant value);
    Q_INVOKABLE void setProperty(const QString localPath, FolderSizeItemRoles role, QVariant value);
    Q_INVOKABLE void setProperty(const int index, QVariant valueJson);

    Q_INVOKABLE void refreshDir(const QString caller, const bool clearCache = false, const bool clearItems = false);
    Q_INVOKABLE void changeDir(const QString &name, const int sortFlag = -1);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QString getUrl(const QString absPath);
    Q_INVOKABLE bool isRoot(); // Overload method for verifying on currentDir.
    Q_INVOKABLE bool isRoot(const QString absPath);
    Q_INVOKABLE QString getItemJson(const QString absFilePath);
    Q_INVOKABLE QString getDirContentJson(const QString dirPath);
    Q_INVOKABLE int getIndexOnCurrentDir(const QString absFilePath);
    Q_INVOKABLE void clearIndexOnCurrentDir();
    void refreshIndexOnCurrentDir();
    Q_INVOKABLE void removeCache(const QString absPath, bool removeAll = false);
    Q_INVOKABLE bool isRunning();

    // File/Dir manipulation methods.
    Q_INVOKABLE bool removeRow(int row, const QModelIndex & parent = QModelIndex());
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    Q_INVOKABLE bool deleteFile(const QString absPath);
    Q_INVOKABLE bool copy(const QString sourcePath, const QString targetPath);
    Q_INVOKABLE bool move(const QString sourcePath, const QString targetPath);
    Q_INVOKABLE bool createDir(const QString name);
    Q_INVOKABLE bool createDirPath(const QString absPath);
    Q_INVOKABLE bool createEmptyFile(const QString name);
    Q_INVOKABLE bool renameFile(const QString fileName, const QString newFileName);

    // Trash related methods.
    Q_INVOKABLE QString getTrashPath();
    bool createTrashIfNotExists();
    Q_INVOKABLE QString getTrashJsonText();
    Q_INVOKABLE qlonglong getMaxTrashSize();
    Q_INVOKABLE bool trash(const QString sourcePath);
    Q_INVOKABLE void requestTrashStatus();
    Q_INVOKABLE void emptyTrash();

    // Modify file attributes (for Symbian).
    Q_INVOKABLE bool setFileAttribute(QString localFilePath, FileAttribute attribute, bool value);

    // Informative methods which don't use FolderSizeModelThread.
    Q_INVOKABLE void refreshItems();
    Q_INVOKABLE void refreshItem(const int index);
    Q_INVOKABLE void refreshItem(const QString localPath);
    Q_INVOKABLE QString getDirPath(const QString absFilePath);
    Q_INVOKABLE QStringList getPathToRoot(const QString absFilePath);
    Q_INVOKABLE QString getRoot(const QString absFilePath);
    Q_INVOKABLE bool isDir(const QString absFilePath);
    Q_INVOKABLE bool isFile(const QString absFilePath);
    Q_INVOKABLE bool canCopy(const QString sourcePath, const QString targetPath);
    Q_INVOKABLE QString getFileName(const QString absFilePath);
    Q_INVOKABLE QString getNewFileName(const QString fileName, const QString targetPath);
    Q_INVOKABLE QString getAbsolutePath(const QString dirPath, const QString fileName);
    Q_INVOKABLE QStringList getDriveList();
    Q_INVOKABLE QStringList getLogicalDriveList();
    Q_INVOKABLE QString formatFileSize(double size);
    Q_INVOKABLE void cancelQueuedJobs();
    Q_INVOKABLE int getQueuedJobCount() const;
    Q_INVOKABLE int getRunningJobCount() const;
    Q_INVOKABLE void abortThread(bool rollbackFlag = true);

    QList<FolderSizeItem> getItemList() const;
    void removeItem(const int index);
    FolderSizeItem getItem(const int index);
    void setItem(const int index, FolderSizeItem &item);
    Q_INVOKABLE int addItem(const QString absPath);

    Q_INVOKABLE void proceedNextJob();
    Q_INVOKABLE void suspendNextJob();
    Q_INVOKABLE void resumeNextJob();

    // Selection functions.
    Q_INVOKABLE bool isAnyItemChecked();
    Q_INVOKABLE bool areAllItemChecked();
    Q_INVOKABLE void markAll();
    Q_INVOKABLE void markAllFiles();
    Q_INVOKABLE void markAllFolders();
    Q_INVOKABLE void unmarkAll();
private:
    Q_DISABLE_COPY(FolderSizeItemListModel)
    FolderSizeModelThread m;

    QSettings m_settings;

    QQueue<FolderSizeJob> m_jobQueue;
    int runningJobCount;
    QString createNonce();
    bool m_isSuspended;

    QList<FolderSizeItem> itemList; // TODO Move to heap.
    void refreshItemList();

    bool isDirSizeCacheExisting();
    bool isReady();
    QStringList splitFileName(const QString fileName);

    QCache<QString, QString> *m_pathToRootCache;
    QHash<QString, int> *m_indexOnCurrentDirHash;

    QStringList m_driveList;

    QMutex mutex;

    QFileSystemWatcher *m_fsWatcher;
    void initializeFSWatcher();
    QStringList findSubDirList(QString dirPath);
    void addWatchedDirPath(QString dirPath);
    void removeWatchedDirPath(QString dirPath);
public slots:
    void loadDirSizeCacheFinishedFilter();
    void fetchDirSizeFinishedFilter(QString sourcePath);
    void copyProgressFilter(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinishedFilter(int fileAction, QString sourcePath, QString targetPath, QString msg, int err, qint64 bytes, qint64 totalBytes, qint64 count, bool isSourceRoot);
    void deleteProgressFilter(int fileAction, QString sourceSubPath, QString msg, int err);
    void deleteFinishedFilter(int fileAction, QString sourcePath, QString msg, int err);
    void trashFinishedFilter(int fileAction, QString sourcePath, QString targetPath, QString msg, int err);
    void emptyDirFinishedFilter(int fileAction, QString sourcePath, QString msg, int err);
    void jobDone();
    void fsWatcherDirectoryChangedSlot(const QString &entry);
Q_SIGNALS:
    void loadDirSizeCacheFinished();
    void initializeDBStarted();
    void initializeDBFinished();
    void currentDirChanged();
    void refreshBegin();
    void refreshCompleted();
    void requestResetCache();
    void copyStarted(int fileAction, QString sourcePath, QString targetPath, QString msg, int err);
    void copyProgress(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinished(int fileAction, QString sourcePath, QString targetPath, QString msg, int err, qint64 bytes, qint64 totalBytes, qint64 count, bool isSourceRoot = true);
    void deleteStarted(int fileAction, QString sourcePath);
    void deleteProgress(int fileAction, QString sourceSubPath, QString msg, int err);
    void deleteFinished(int fileAction, QString sourcePath, QString msg, int err);
    void trashFinished(int fileAction, QString sourcePath, QString targetPath, QString msg, int err);
    void emptyDirFinished(int fileAction, QString sourcePath, QString msg, int err);
    void createFinished(QString targetPath, QString msg, int err);
    void renameFinished(QString sourcePath, QString targetPath, QString msg, int err);
    void fetchDirSizeStarted(QString dirPath);
    void fetchDirSizeFinished(QString dirPath);
    void fetchDirSizeUpdated(QString dirPath);
    void proceedNextJobSignal();
    void directoryChanged(QString dirPath);
    void runningJobCountChanged();
    void sortFlagChanged();
    void trashChanged();
    void isRunningChanged();
};

#endif // FOLDERSIZEITEMLISTMODEL_H
