#ifndef CLIPBOARDMODEL_H
#define CLIPBOARDMODEL_H

#include <QDeclarativeItem>
#include <QHash>
#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptValueIterator>
#include <QDateTime>
#include <QApplication>

class ClipboardModel : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit ClipboardModel(QDeclarativeItem *parent = 0);
    
signals:
    void countChanged();
public slots:
    Q_INVOKABLE int count() const;
    Q_INVOKABLE int countByAction(const QString action);
    Q_INVOKABLE int clear();
    Q_INVOKABLE void emitCountChanged();
    Q_INVOKABLE bool addClipboardItem(const QString localPath, const QString jsonText, const bool suppressCountChanged = false);
    Q_INVOKABLE int removeClipboardItem(const QString localPath);
    Q_INVOKABLE QString getItemJsonText(const int index);
    Q_INVOKABLE QString getItemJsonText(const QString localPath);

    Q_INVOKABLE void clearDeleteActions();
private:
    QHash<QString, QString> *m_hash;

    QMap<QString, QString> createMapFromJson(QString jsonText);
    QMap<QString, QString> getItemMap(const int index);
};

#endif // CLIPBOARDMODEL_H
