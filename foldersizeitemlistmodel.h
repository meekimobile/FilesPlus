#ifndef FOLDERSIZEITEMLISTMODEL_H
#define FOLDERSIZEITEMLISTMODEL_H

#include <QAbstractListModel>
#include <QDeclarativeParserStatus>
#include <QTimer>
#include "foldersizeitem.h"
#include "foldersizemodel.h"

class FolderSizeItemListModel : public QAbstractListModel, public QDeclarativeParserStatus
{
    Q_OBJECT
    Q_ENUMS(SortFlags)
    Q_INTERFACES(QDeclarativeParserStatus)
    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(int sortFlag READ getSortFlag WRITE setSortFlag)
public:
    static const int TimerInterval;

    enum SortFlags {
        SortByName,
        SortByTime,
        SortBySize,
        SortByType
    };

    enum FolderSizeItemRoles {
        NameRole = Qt::UserRole + 1,
        AbsolutePathRole,
        SizeRole,
        LastModifiedRole,
        IsDirRole,
        SubDirCountRole,
        SubFileCountRole
    };

    explicit FolderSizeItemListModel(QObject *parent = 0);
    ~FolderSizeItemListModel();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    QString currentDir() const;
    void setCurrentDir(const QString &path);
    void loadDirSizeCache();
    void saveDirSizeCache();
    bool isDirSizeCacheExisting();
    bool isReady();

    Q_INVOKABLE QStringList getDriveList();
    Q_INVOKABLE QString formatFileSize(double size);
    Q_INVOKABLE void changeDir(const QString &name);
    Q_INVOKABLE void refreshDir(const bool clearCache = false);
    Q_INVOKABLE QString getUrl(const QString absPath);
    Q_INVOKABLE bool isRoot();

    Q_INVOKABLE int getSortFlag() const;
    Q_INVOKABLE void setSortFlag(const int sortFlag);

    Q_INVOKABLE bool removeRow(int row, const QModelIndex & parent = QModelIndex());
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    Q_INVOKABLE bool deleteFile(const QString absPath);
    Q_INVOKABLE bool copyFile(const QString sourceAbsFilePath, const QString targetPath);
    Q_INVOKABLE QString getDirPath(const QString absFilePath);

    void classBegin();
    void componentComplete();
private:
    Q_DISABLE_COPY(FolderSizeItemListModel)
    FolderSizeModel m;
    QTimer *timer;
public slots:
    void checkRunnable();
Q_SIGNALS:
    void currentDirChanged();
    void refreshBegin();
    void refreshCompleted();
    void requestResetCache();
};

#endif // FOLDERSIZEITEMLISTMODEL_H
