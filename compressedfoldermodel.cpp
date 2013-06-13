#include "compressedfoldermodel.h"
#include "compressedfoldermodelworker.h"
#include <JlCompress.h>
#include <QApplication>
#include <QDebug>

CompressedFolderModel::CompressedFolderModel(QObject *parent) :
    QAbstractListModel(parent)
{
    m_thread = new QThread(this);

    // Initialize cloud drive model item list.
    QHash<int, QByteArray> roles;
    int i = Qt::UserRole; // Start from user role.
    roles[++i] = "name";
    roles[++i] = "size";
    roles[++i] = "compressedSize";
    roles[++i] = "lastModified";
    roles[++i] = "isDir";
    roles[++i] = "isChecked";
    setRoleNames(roles);

    m_modelItemList = new QList<CompressedFolderModelItem>();
}

int CompressedFolderModel::rowCount(const QModelIndex &parent) const
{
    return m_modelItemList->count();
}

QVariant CompressedFolderModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= rowCount()) {
        return QString("");
    }

    CompressedFolderModelItem item = m_modelItemList->at(index.row());
    QString roleName = roleNames().value(role);
    if (roleName == "name") {
        return item.name;
    } else if (roleName == "size") {
        return item.size;
    } else if (roleName == "compressedSize") {
        return item.compressedSize;
    } else if (roleName == "lastModified") {
        return item.lastModified;
    } else if (roleName == "isDir") {
        return item.isDir;
    } else if (roleName == "isChecked") {
        return item.isChecked;
    }

    return QVariant();
}

QVariant CompressedFolderModel::get(const int index)
{
    if (index >= 0 && index < rowCount()) {
        QMap<QString,QVariant> jsonObj;
        foreach(int role, roleNames().keys()) {
            QString propertyName = QString(roleNames().value(role));
            QVariant propertyValue = data(createIndex(index,0), role);
            jsonObj[propertyName] = propertyValue;
        }
//        qDebug() << "CompressedFolderModel::get" << QVariant(jsonObj);
        return QVariant(jsonObj);
    }

    return QVariant();
}

void CompressedFolderModel::setProperty(const int index, QString roleName, QVariant value)
{
    bool isChanged = false;
    if (index >= 0 && index < rowCount()) {
        CompressedFolderModelItem item = m_modelItemList->at(index);
        if (roleName == "name") {
            item.name = value.toString();
            isChanged = true;
        } else if (roleName == "size") {
            item.size = value.toUInt();
            isChanged = true;
        } else if (roleName == "compressedSize") {
            item.compressedSize = value.toUInt();
            isChanged = true;
        } else if (roleName == "lastModified") {
            item.lastModified = value.toDateTime();
            isChanged = true;
        } else if (roleName == "isDir") {
            item.isDir = value.toBool();
            isChanged = true;
        } else if (roleName == "isChecked") {
            item.isChecked = value.toBool();
            isChanged = true;
        }

        if (isChanged) {
//            qDebug() << "CompressedFolderModel::setProperty" << index << roleName << value << "isChanged" << isChanged;
            m_modelItemList->replace(index, item);
            // Emit data changed.
            emit dataChanged(createIndex(index,0), createIndex(index, 0));
        }
    } else {
        // TODO do nothing.
    }
}

bool CompressedFolderModel::list(QString compressedFilePath)
{
    if (!m_thread->isRunning()) {
        // NOTE Worker is created in main thread without parent. So it can be moved to worker thread.
        CompressedFolderModelWorker *worker = new CompressedFolderModelWorker();
        worker->action = CompressedFolderModelWorker::List;
        worker->compressedFilePath = compressedFilePath;
        worker->sourcePathList = QStringList();
        worker->itemList = m_modelItemList;
        worker->moveToThread(m_thread);

        connect(m_thread, SIGNAL(started()), worker, SLOT(start()));
        connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
        connect(worker, SIGNAL(finished()), m_thread, SLOT(quit()));
        connect(worker, SIGNAL(listStarted(QString)), this, SIGNAL(listStarted(QString)));
        connect(worker, SIGNAL(listFinished(QString,int)), this, SLOT(listFinishedFilter(QString,int)));

        m_thread->start(QThread::LowPriority);

        return true;
    } else {
        qDebug() << "CompressedFolderModel::list worker thread is busy. Operation is ignored.";

        return false;
    }
}

bool CompressedFolderModel::compress(QString sourcePath)
{
    return compress(getDefaultCompressedFilePath(sourcePath), sourcePath);
}

bool CompressedFolderModel::compress(QString compressedFilePath, QString sourcePath)
{
    return compress(compressedFilePath, QStringList(sourcePath));
}

bool CompressedFolderModel::compress(QString compressedFilePath, QStringList sourceFilePathList)
{
    if (!m_thread->isRunning()) {
        // NOTE Worker is created in main thread without parent. So it can be moved to worker thread.
        CompressedFolderModelWorker *worker = new CompressedFolderModelWorker();
        worker->action = CompressedFolderModelWorker::Compress;
        worker->compressedFilePath = compressedFilePath;
        worker->sourcePathList = sourceFilePathList;
        worker->moveToThread(m_thread);

        connect(m_thread, SIGNAL(started()), worker, SLOT(start()));
        connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
        connect(worker, SIGNAL(finished()), m_thread, SLOT(quit()));
        connect(worker, SIGNAL(compressStarted(QString)), this, SIGNAL(compressStarted(QString)));
        connect(worker, SIGNAL(compressFinished(QString,int)), this, SIGNAL(compressFinished(QString,int)));

        m_thread->start(QThread::LowPriority);

        return true;
    } else {
        qDebug() << "CompressedFolderModel::compress worker thread is busy. Operation is ignored.";

        return false;
    }
}

bool CompressedFolderModel::extract(QString compressedFilePath)
{
    return extract(compressedFilePath, QStringList(), getDefaultExtractedPath(compressedFilePath));
}

bool CompressedFolderModel::extract(QString compressedFilePath, QString sourcePath, QString targetPath)
{
    if (sourcePath == "" || targetPath == "") return false;

    if (!m_thread->isRunning()) {
        // NOTE Worker is created in main thread without parent. So it can be moved to worker thread.
        CompressedFolderModelWorker *worker = new CompressedFolderModelWorker();
        worker->action = CompressedFolderModelWorker::Extract;
        worker->compressedFilePath = compressedFilePath;
        worker->sourcePath = sourcePath;
        worker->targetPath = targetPath;
        worker->moveToThread(m_thread);

        connect(m_thread, SIGNAL(started()), worker, SLOT(start()));
        connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
        connect(worker, SIGNAL(finished()), m_thread, SLOT(quit()));
        connect(worker, SIGNAL(extractStarted(QString)), this, SIGNAL(extractStarted(QString)));
        connect(worker, SIGNAL(extractFinished(QString,int,QStringList)), this, SIGNAL(extractFinished(QString,int,QStringList)));

        m_thread->start(QThread::LowPriority);

        return true;
    } else {
        qDebug() << "CompressedFolderModel::extract worker thread is busy. Operation is ignored.";

        return false;
    }
}

bool CompressedFolderModel::extract(QString compressedFilePath, QStringList sourcePathList, QString targetParentPath)
{
//    qDebug() << "CompressedFolderModel::extract" << compressedFilePath << sourcePathList << targetParentPath;

    if (!m_thread->isRunning()) {
        // NOTE Worker is created in main thread without parent. So it can be moved to worker thread.
        CompressedFolderModelWorker *worker = new CompressedFolderModelWorker();
        worker->action = CompressedFolderModelWorker::Extract;
        worker->compressedFilePath = compressedFilePath;
        worker->sourcePathList = sourcePathList;
        worker->targetParentPath = targetParentPath;
        worker->moveToThread(m_thread);

        connect(m_thread, SIGNAL(started()), worker, SLOT(start()));
        connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
        connect(worker, SIGNAL(finished()), m_thread, SLOT(quit()));
        connect(worker, SIGNAL(extractStarted(QString)), this, SIGNAL(extractStarted(QString)));
        connect(worker, SIGNAL(extractFinished(QString,int,QStringList)), this, SIGNAL(extractFinished(QString,int,QStringList)));

        m_thread->start(QThread::LowPriority);

        return true;
    } else {
        qDebug() << "CompressedFolderModel::extract worker thread is busy. Operation is ignored.";

        return false;
    }
}

QString CompressedFolderModel::getDefaultCompressedFilePath(QString sourcePath)
{
    QFileInfo sourceInfo(sourcePath);

    return sourceInfo.absoluteDir().absoluteFilePath(sourceInfo.baseName() + ".zip");
}

bool CompressedFolderModel::isCompressedFile(QString sourcePath)
{
    QFileInfo sourceInfo(sourcePath);

    return sourceInfo.isFile() && sourceInfo.suffix().toLower() == "zip";
}

QString CompressedFolderModel::getDefaultExtractedPath(QString compressedFilePath)
{
    QFileInfo info(compressedFilePath);

    return info.absoluteDir().absoluteFilePath(info.baseName());
}

bool CompressedFolderModel::isAnyItemChecked()
{
    for (int i = 0; i < m_modelItemList->count(); i++) {
        QApplication::processEvents();
        CompressedFolderModelItem item = m_modelItemList->at(i);
        if (item.isChecked) {
            return true;
        }
    }

    return false;
}

bool CompressedFolderModel::areAllItemChecked()
{
    for (int i = 0; i < m_modelItemList->count(); i++) {
        QApplication::processEvents();
        CompressedFolderModelItem item = m_modelItemList->at(i);
        if (!item.isChecked) {
            return false;
        }
    }

    return (m_modelItemList->count() > 0);
}

void CompressedFolderModel::markAll()
{
    for (int i = 0; i < m_modelItemList->count(); i++) {
        QApplication::processEvents();
        CompressedFolderModelItem item = m_modelItemList->at(i);
        item.isChecked = true;
        m_modelItemList->replace(i, item);
    }

    refreshItems();
}

void CompressedFolderModel::markAllFiles()
{
    for (int i = 0; i < m_modelItemList->count(); i++) {
        QApplication::processEvents();
        CompressedFolderModelItem item = m_modelItemList->at(i);
        if (!item.isDir) {
            item.isChecked = true;
            m_modelItemList->replace(i, item);
        }
    }

    refreshItems();
}

void CompressedFolderModel::markAllFolders()
{
    for (int i = 0; i < m_modelItemList->count(); i++) {
        QApplication::processEvents();
        CompressedFolderModelItem item = m_modelItemList->at(i);
        if (item.isDir) {
            item.isChecked = true;
            m_modelItemList->replace(i, item);
        }
    }

    refreshItems();
}

void CompressedFolderModel::unmarkAll()
{
    for (int i = 0; i < m_modelItemList->count(); i++) {
        QApplication::processEvents();
        CompressedFolderModelItem item = m_modelItemList->at(i);
        item.isChecked = false;
        m_modelItemList->replace(i, item);
    }

    refreshItems();
}

void CompressedFolderModel::refreshItems()
{
    // Emit data changed.
    beginResetModel();
    emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
    endResetModel();
}

void CompressedFolderModel::listFinishedFilter(QString compressedFilePath, int err)
{
    emit rowCountChanged();

    refreshItems();

    emit listFinished(compressedFilePath, err);
}

