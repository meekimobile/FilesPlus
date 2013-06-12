#ifndef COMPRESSEDFOLDERMODEL_H
#define COMPRESSEDFOLDERMODEL_H

#include <QAbstractListModel>
#include <QThread>
#include <QList>
#include <QStringList>
#include "compressedfoldermodelitem.h"

class CompressedFolderModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)
public:
    explicit CompressedFolderModel(QObject *parent = 0);

    // List model methods.
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    Q_INVOKABLE QVariant get(const int index);
    Q_INVOKABLE void setProperty(const int index, QString roleName, QVariant value);

    Q_INVOKABLE bool list(QString compressedFilePath);
    Q_INVOKABLE bool compress(QString sourcePath);
    Q_INVOKABLE bool compress(QString compressedFilePath, QString sourcePath);
    Q_INVOKABLE bool compress(QString compressedFilePath, QStringList sourceFilePathList);
    Q_INVOKABLE bool extract(QString compressedFilePath);
    Q_INVOKABLE bool extract(QString compressedFilePath, QString sourcePath, QString targetPath);
    Q_INVOKABLE bool extract(QString compressedFilePath, QStringList sourcePathList, QString targetParentPath);

    Q_INVOKABLE QString getDefaultCompressedFilePath(QString sourcePath);
    Q_INVOKABLE bool isCompressedFile(QString sourcePath);
    Q_INVOKABLE QString getDefaultExtractedPath(QString compressedFilePath);

    Q_INVOKABLE bool isAnyItemChecked();
    Q_INVOKABLE bool areAllItemChecked();
    Q_INVOKABLE void markAll();
    Q_INVOKABLE void markAllFiles();
    Q_INVOKABLE void markAllFolders();
    Q_INVOKABLE void unmarkAll();

    Q_INVOKABLE void refreshItems();
signals:
    void rowCountChanged();

    void compressStarted(QString compressedFilePath);
    void compressFinished(QString compressedFilePath, int err);
    void extractStarted(QString compressedFilePath);
    void extractFinished(QString compressedFilePath, int err);
    void listStarted(QString compressedFilePath);
    void listFinished(QString compressedFilePath, int err);
public slots:
    void listFinishedFilter(QString compressedFilePath, int err);
private:
    QThread *m_thread;
    QList<CompressedFolderModelItem> *m_modelItemList;
};

#endif // COMPRESSEDFOLDERMODEL_H
