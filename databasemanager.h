#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QDebug>
#include <QtSql>

class DatabaseManager
{
public:
    static DatabaseManager& defaultManager()
    {
        static DatabaseManager m_instance;
        return m_instance;
    }

    void initializeDB();
    QSqlDatabase getDB();

private:
    DatabaseManager(){} // Private so that it can  not be called
    DatabaseManager(const DatabaseManager&){} // copy constructor is private
    DatabaseManager& operator=(DatabaseManager const&){} // assignment operator is private

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H
