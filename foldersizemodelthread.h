#ifndef FOLDERSIZEMODELTHREAD_H
#define FOLDERSIZEMODELTHREAD_H

#include "foldersizeitem.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QThread>
#include <QApplication>
#include <QtSql>

class FolderSizeModelThread : public QThread
{
    Q_OBJECT
public:
    static const QString CACHE_FILE_PATH;
    static const QString CACHE_DB_PATH;
    static const QString CACHE_DB_CONNECTION_NAME;
    static const QString DEFAULT_CURRENT_DIR;
    static const int FILE_READ_BUFFER;
    static const int FILE_COPY_DELAY;
    static const int FILE_DELETE_DELAY;

    enum SortFlags {
        SortByName,
        SortByTime,
        SortBySize,
        SortByType,
        SortByNameWithDirectoryFirst
    };

    enum RunnableMethods {
        FetchDirSize,
        LoadDirSizeCache,
        CopyFile,
        MoveFile,
        DeleteFile,
        InitializeDB
    };

    FolderSizeModelThread(QObject *parent = 0);
    ~FolderSizeModelThread();

    QString currentDir() const;
    void setCurrentDir(const QString currentDir, const int sortFlag = -1);
    int sortFlag() const;
    bool setSortFlag(int sortFlag, bool saveSortFlag = true);
    QStringList nameFilters() const;
    void setNameFilters(const QStringList nameFilters);
    bool clearCache() const;
    void setClearCache(bool clearCache);
    bool abortFlag() const;
    void setAbortFlag(bool flag);
    bool rollbackFlag() const;
    void setRollbackFlag(bool flag);

    int getSortFlagFromDB(const QString absolutePath, const int defaultSortFlag);

    void saveDirSizeCache();

    void getDirContent(const QString dirPath, QList<FolderSizeItem> &itemList);
    void sortItemList(QList<FolderSizeItem> &itemList, bool sortAll = true);
    bool isDirSizeCacheExisting();
    int removeDirSizeCache(const QString key);
    bool changeDir(const QString dirName, const int sortFlag = -1);
    FolderSizeItem getItem(const QFileInfo fileInfo);

    QString getSourcePath();
    void setRunMethod(int method);
    void setSourcePath(const QString sourcePath);
    void setTargetPath(const QString targetPath);
    void run();
signals:
    void loadDirSizeCacheFinished();
    void initializeDBStarted();
    void initializeDBFinished();
    void fetchDirSizeStarted();
    void fetchDirSizeFinished();
    void copyStarted(int fileAction, QString sourcePath, QString targetPath, QString msg, int err);
    void copyProgress(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinished(int fileAction, QString sourcePath, QString targetPath, QString msg, int err, qint64 bytes, qint64 bytesTotal, qint64 count, bool isSourceRoot);
    void fetchDirSizeUpdated(QString dirPath);
    void deleteStarted(int fileAction, QString localPath);
    void deleteProgress(int fileAction, QString subLocalPath, QString msg, int err);
    void deleteFinished(int fileAction, QString localPath, QString msg, int err);
private:
    FolderSizeItem getCachedDir(const QFileInfo dir, const bool clearCache = false);
    FolderSizeItem getFileItem(const QFileInfo fileInfo);
    FolderSizeItem getDirItem(const QFileInfo dirInfo);

    QSqlDatabase m_db;
    QSqlQuery m_selectPS;
    QSqlQuery m_insertPS;
    QSqlQuery m_updatePS;
    QSqlQuery m_deletePS;
    QSqlQuery m_countPS;

    void initializeDB();
    void closeDB();
    void fixDamagedDB();
    FolderSizeItem selectDirSizeCacheFromDB(const QString id);
    int insertDirSizeCacheToDB(const FolderSizeItem item);
    int updateDirSizeCacheToDB(const FolderSizeItem item);
    int updateDirSizeCacheTreeToDB(const QString oldAbsPath, const QString newAbsPath);
    int deleteDirSizeCacheToDB(const QString id);
    int deleteDirSizeCacheTreeToDB(const QString id);
    int countDirSizeCacheDB();

    // Thread methods.
    void loadDirSizeCache();
    void fetchDirSize(const QString startPath, const bool clearCache = false);
    bool move(const QString sourcePath, const QString targetPath);
    bool copy(int method, const QString sourcePath, const QString targetPath);
    bool copyFile(int method, const QString sourcePath, const QString targetPath);
    bool deleteDir(const QString sourcePath);
    void delay(const int interval = 100);

    void cleanItems();
    bool cleanItem(const FolderSizeItem &item);

    QHash<QString, FolderSizeItem> *dirSizeCache;
    QHash<QString, FolderSizeItem> *m_itemCache;
    QString m_currentDir;
    bool m_clearCache;
    int m_sortFlag;
    QStringList m_nameFilters;

    int m_runMethod;
    QString m_sourcePath;
    QString m_targetPath;

    bool m_abortFlag;
    bool m_rollbackFlag;

    bool m_isFileCached;
};

#endif // FOLDERSIZEMODELTHREAD_H
