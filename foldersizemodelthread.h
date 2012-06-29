#ifndef FOLDERSIZEMODELTHREAD_H
#define FOLDERSIZEMODELTHREAD_H

#include "foldersizeitem.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QThread>

class FolderSizeModelThread : public QThread
{
    Q_OBJECT
public:
    static const QString CACHE_FILE_PATH;
    static const QString DEFAULT_CURRENT_DIR;
    static const int FILE_READ_BUFFER;

    enum SortFlags {
        SortByName,
        SortByTime,
        SortBySize,
        SortByType
    };

    enum RunnableMethods {
        FetchDirSize,
        LoadDirSizeCache,
        CopyFile,
        MoveFile,
        DeleteFile
    };

    FolderSizeModelThread(QObject *parent = 0);

    QString currentDir() const;
    void setCurrentDir(const QString currentDir);
    int sortFlag() const;
    bool setSortFlag(int sortFlag);
    QStringList nameFilters() const;
    void setNameFilters(const QStringList nameFilters);
    bool clearCache() const;
    void setClearCache(bool clearCache);
    bool abortFlag() const;
    void setAbortFlag(bool flag);
    bool rollbackFlag() const;
    void setRollbackFlag(bool flag);

    void saveDirSizeCache();

    void getDirContent(const QString dirPath, QList<FolderSizeItem> &itemList);
    void sortItemList(QList<FolderSizeItem> &itemList);
    bool isDirSizeCacheExisting();
    void removeDirSizeCache(const QString key);
    bool changeDir(const QString dirName);
    FolderSizeItem getItem(const QFileInfo fileInfo) const;

    void setRunMethod(int method);
    void setSourcePath(const QString sourcePath);
    void setTargetPath(const QString targetPath);
    void run();
signals:
    void loadDirSizeCacheFinished();
    void fetchDirSizeStarted();
    void fetchDirSizeFinished();
    void copyStarted(int fileAction, QString sourcePath, QString targetPath, QString msg, int err);
    void copyProgress(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinished(int fileAction, QString sourcePath, QString targetPath, QString msg, int err, qint64 bytes, qint64 bytesTotal);
    void fetchDirSizeUpdated(QString dirPath);
    void deleteStarted(int fileAction, QString localPath);
    void deleteFinished(int fileAction, QString localPath, QString msg, int err);
private:
    FolderSizeItem getCachedDir(const QFileInfo dir, const bool clearCache = false);
    FolderSizeItem getFileItem(const QFileInfo fileInfo) const;
    FolderSizeItem getDirItem(const QFileInfo dirInfo) const;

    // Thread methods.
    void loadDirSizeCache();
    void fetchDirSize(const bool clearCache = false);
    bool copy(int method, const QString sourcePath, const QString targetPath);
    bool copyFile(int method, const QString sourcePath, const QString targetPath);
    bool deleteDir(const QString sourcePath);

    QHash<QString, FolderSizeItem> *dirSizeCache;
    QString m_currentDir;
    bool m_clearCache;
    int m_sortFlag;
    QStringList m_nameFilters;

    int m_runMethod;
    QString m_sourcePath;
    QString m_targetPath;

    bool m_abortFlag;
    bool m_rollbackFlag;
};

#endif // FOLDERSIZEMODELTHREAD_H
