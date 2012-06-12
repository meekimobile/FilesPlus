#ifndef FOLDERSIZEMODEL_H
#define FOLDERSIZEMODEL_H

#include "foldersizeitem.h"
#include <QDir>
#include <QHash>
#include <QThread>

class FolderSizeModel : public QThread
{
    Q_OBJECT
public:
    static const QString CACHE_FILE_PATH;
    static const QString DEFAULT_CURRENT_DIR;

    enum SortFlags {
        SortByName,
        SortByTime,
        SortBySize,
        SortByType
    };

    enum RunnableMethods {
        FetchDirSize,
        LoadDirSizeCache
    };

    FolderSizeModel(QObject *parent = 0);

    QString currentDir() const;
    void setCurrentDir(const QString currentDir);
    int sortFlag() const;
    bool setSortFlag(int sortFlag);
    bool clearCache();
    void setClearCache(bool clearCache);

    void fetchDirSize(const bool clearCache = false);
    void loadDirSizeCache();
    void saveDirSizeCache();
    QList<FolderSizeItem> getDirContent() const;
    QList<FolderSizeItem> getDirContent(const QString dirPath);
    bool isDirSizeCacheExisting();
    void removeDirSizeCache(const QString key);
    bool changeDir(const QString dirName);
    void removeItem(const int index);
    FolderSizeItem getItem(const int index);
    void updateItem(const int index, FolderSizeItem &item);

    QString formatFileSize(double size);
    QStringList getDriveList();
    bool isReady();
    bool isRoot();

    void setRunMethod(int method);
    void run();
signals:
    void loadDirSizeCacheFinished();
    void fetchDirSizeFinished();
private:
    FolderSizeItem getCachedDir(const QFileInfo dir, const bool clearCache = false);
    FolderSizeItem getFileItem(const QFileInfo fileInfo);
    void sortItemList(QList<FolderSizeItem> &itemList);

    QHash<QString, FolderSizeItem> dirSizeCache;
    QList<FolderSizeItem> itemList;
    QString m_currentDir;
    bool m_isReady;
    bool m_clearCache;
    int m_sortFlag;

    int m_runMethod;
};

#endif // FOLDERSIZEMODEL_H
