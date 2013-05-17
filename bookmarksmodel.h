#ifndef BOOKMARKSMODEL_H
#define BOOKMARKSMODEL_H

#include <QSqlTableModel>

class BookmarksModel : public QSqlTableModel
{
    Q_OBJECT
public:
    static const QString DB_PATH;
    static const QString DB_CONNECTION_NAME;

    explicit BookmarksModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    Q_INVOKABLE bool insertTestData();

    Q_INVOKABLE bool deleteRow(int row);
    Q_INVOKABLE bool refresh();
    Q_INVOKABLE bool save();
    Q_INVOKABLE QVariant get(const int index);
    Q_INVOKABLE QVariant getProperty(const int index, QString roleName);
    Q_INVOKABLE void setProperty(const int index, QString roleName, QVariant value);
signals:
    
public slots:
    
private:
    void initializeDB();
    void generateRoleNames();
};

#endif // BOOKMARKSMODEL_H
