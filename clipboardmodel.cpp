#include "clipboardmodel.h"

ClipboardModel::ClipboardModel(QDeclarativeItem *parent) :
    QDeclarativeItem(parent), m_hash(new QHash<QString, QString>())
{
}

int ClipboardModel::count() const
{
    return m_hash->count();
}

int ClipboardModel::countByAction(const QString action)
{
    int c = 0;
    QMap<QString, QString> itemMap;
    QHashIterator<QString, QString> i(*m_hash);
    while (i.hasNext()) {
        i.next();
        QString itemJsonText = i.value();
        itemMap = createMapFromJson(itemJsonText);

        if (itemMap.value("action") == action) {
            // Remove current entry will make next entry takeover current index. It needs not increase index.
            qDebug() << "ClipboardModel::countByAction" << action << "item" << itemMap;
            c++;
        }
    }

    return c;
}

int ClipboardModel::clear()
{
    m_hash->clear();

    emit countChanged();
}

void ClipboardModel::emitCountChanged()
{
    emit countChanged();
}

bool ClipboardModel::addClipboardItem(const QString localPath, const QString jsonText, const bool suppressCountChanged)
{
    if (localPath == "" || jsonText == "") return false;

//    qDebug() << QDateTime::currentDateTime().toString("d MMM yyyy h:mm:ss.zzz ap") << "ClipboardModel::addClipboardItem adding" << localPath << jsonText;

    m_hash->insert(localPath, jsonText);

    // Issue:  Emit signal below will slow down adding. But it's needed for showing icon.
    // Workaround: Suppress signal during massive adding, emit signal after all are added.
    if (!suppressCountChanged) {
        emit countChanged();
    }

//    qDebug() << QDateTime::currentDateTime().toString("d MMM yyyy h:mm:ss.zzz ap") << "ClipboardModel::addClipboardItem added" << localPath << jsonText;

    return true;
}

int ClipboardModel::removeClipboardItem(const QString localPath)
{
    return m_hash->remove(localPath);
}

QString ClipboardModel::getItemJsonText(const int index)
{
    return m_hash->values().at(index);
}

QString ClipboardModel::getItemJsonText(const QString localPath)
{
    return m_hash->value(localPath);
}

QMap<QString, QString> ClipboardModel::createMapFromJson(QString jsonText)
{
    qDebug() << "ClipboardModel::createMapFromJson " << jsonText;

    QMap<QString, QString> map;
    QScriptEngine engine;
    QScriptValue sc;
    sc = engine.evaluate("(" + jsonText + ")");

    QScriptValueIterator it(sc);
    while (it.hasNext()) {
        it.next();
        map[it.name()] = it.value().toString();
    }

    qDebug() << "ClipboardModel::createMapFromJson " << map;

    return map;
}

QMap<QString, QString> ClipboardModel::getItemMap(const int index)
{
    return createMapFromJson(getItemJsonText(index));
}

void ClipboardModel::clearDeleteActions()
{
    QMap<QString, QString> itemMap;
    QMutableHashIterator<QString, QString> i(*m_hash);
    while (i.hasNext()) {
        i.next();
        QString itemJsonText = i.value();
        itemMap = createMapFromJson(itemJsonText);

        if (itemMap.value("action") == "delete") {
            // Remove current entry will make next entry takeover current index. It needs not increase index.
            qDebug() << "ClipboardModel::clearDeleteActions remove item" << itemMap;
            i.remove();

            emit countChanged();
        }
    }
}

