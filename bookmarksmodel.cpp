#include "bookmarksmodel.h"
#include <QtSql>
#include <QDebug>

BookmarksModel::BookmarksModel(QObject *parent) :
    QSqlTableModel(parent)
{
    initializeDB();
}

void BookmarksModel::initializeDB()
{
    setTable("bookmarks");
    setSort(fieldIndex("title"), Qt::AscendingOrder);
    select();
    generateRoleNames();
}

void BookmarksModel::generateRoleNames()
{
    QHash<int, QByteArray> roles;
    for (int i = 0; i < this->columnCount(); i++) {
        roles[Qt::UserRole + i + 1] = QVariant(this->headerData(i, Qt::Horizontal).toString()).toByteArray();
    }
    setRoleNames(roles);
    qDebug() << "BookmarksModel::generateRoleNames" << roleNames();
}

QVariant BookmarksModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= rowCount()) {
        return QString("");
    }
    if (role < Qt::UserRole) {
        return QSqlQueryModel::data(index, role);
    } else {
        return QSqlQueryModel::data(this->index(index.row(), role - Qt::UserRole - 1), Qt::DisplayRole);
    }
}

QVariant BookmarksModel::get(const int index)
{
    if (index >= 0 && index < rowCount()) {
        QMap<QString,QVariant> jsonObj;
        foreach(int role, roleNames().keys()) {
            QString propertyName = QString(roleNames().value(role));
            QVariant propertyValue = data(createIndex(index,0), role);
            jsonObj[propertyName] = propertyValue;
//            qDebug() << "BookmarksModel::get index" << index << propertyName << propertyValue;
        }
        return QVariant(jsonObj);
    }

    return QVariant();
}

QVariant BookmarksModel::getProperty(const int index, QString roleName)
{
    if (index >= 0 && index < rowCount()) {
        foreach(int role, roleNames().keys()) {
            if (roleName == QString(roleNames().value(role))) {
                return data(createIndex(index,0), role);
            }
        }
    }

    return QVariant();
}

void BookmarksModel::setProperty(const int index, QString roleName, QVariant value)
{
    qDebug() << "BookmarksModel::setProperty" << index << roleName << value;
    if (index >= 0 && index < rowCount()) {
        foreach(int role, roleNames().keys()) {
//            qDebug() << "BookmarksModel::setProperty" << index << roleName << value << role << roleNames().value(role);
            if (roleName == QString(roleNames().value(role))) {
                bool res = setData(createIndex(index, role - Qt::UserRole - 1), value, Qt::EditRole);
                qDebug() << "BookmarksModel::setProperty" << index << roleName << value << role << roleNames().value(role) << "res" << res;
            }
        }
    }
}

bool BookmarksModel::addBookmark(const int type, const QString uid, const QString path, const QString title)
{
    qDebug() << "BookmarksModel::addBookmark" << type << uid << path << title;

    QSqlQuery query;
    bool res;

    res = query.prepare("INSERT INTO bookmarks (type, uid, path, title) VALUES (:type, :uid, :path, :title);");
    query.bindValue(":type", type);
    query.bindValue(":uid", uid);
    query.bindValue(":path", path);
    query.bindValue(":title", title);
    res = query.exec();
    if (res) {
        qDebug() << "BookmarksModel::addBookmark is done.";
        QSqlDatabase::database().commit();
    } else {
        qDebug() << "BookmarksModel::addBookmark failed. Error" << query.lastError();
        QSqlDatabase::database().rollback();
        emit addErrorSignal(query.lastError().type(), query.lastError().text());
    }

    return res;
}

bool BookmarksModel::insertTestData()
{
    QSqlQuery query;
    bool res;

    res = query.exec("INSERT INTO bookmarks (type, uid, path, title) VALUES (0, 'AAA', '/A', 'test A');");
    if (res) {
        qDebug() << "BookmarksModel::initializeDB insert test data is done.";
    } else {
        qDebug() << "BookmarksModel::initializeDB insert test data is failed. Error" << query.lastError();
    }
    res = query.exec("INSERT INTO bookmarks (type, uid, path, title) VALUES (0, 'AAA', '/B', 'test B');");
    if (res) {
        qDebug() << "BookmarksModel::initializeDB insert test data is done.";
    } else {
        qDebug() << "BookmarksModel::initializeDB insert test data is failed. Error" << query.lastError();
    }

    QSqlDatabase::database().commit();
}

bool BookmarksModel::deleteRow(int row)
{
    return removeRow(row);
}

bool BookmarksModel::refresh()
{
    return select();
}

bool BookmarksModel::save()
{
    return submitAll();
}
