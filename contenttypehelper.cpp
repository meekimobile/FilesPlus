#include "contenttypehelper.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>

ContentTypeHelper::ContentTypeHelper()
{
}

QHash<QString, QString> ContentTypeHelper::loadMimeTypes(const QString &localPath)
{
    qDebug() << "ContentTypeHelper::loadMimeTypes localPath " << localPath;

    if (localPath.isEmpty()) return QHash<QString, QString>();

    QHash<QString, QString> contentTypeHash;
    QFile file(localPath);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString line;
        QStringList tokens;
        line = in.readLine();
        while (!line.isNull()) {
            // Skip commented line.
            if (!line.startsWith("#")) {
                // Parse line.
                tokens = line.split(QRegExp("\\s+"));
                if (tokens.count() > 1) {
                    qDebug() << "ContentTypeHelper::loadMimeTypes accepted line" << line << tokens;
                    for (int i = 1; i < tokens.count(); i++) {
                        contentTypeHash.insert(tokens.at(i), tokens.at(0));
                        qDebug() << "ContentTypeHelper::loadMimeTypes inserted" << tokens.at(i) << "=" << tokens.at(0);
                    }
                } else {
                    qDebug() << "ContentTypeHelper::loadMimeTypes skipped line" << line << tokens;
                }
            }

            // Read next line.
            line = in.readLine();
        }
    }
    file.close();

    qDebug() << "ContentTypeHelper::loadMimeTypes contentTypeHash.count()" << contentTypeHash.count();

    return contentTypeHash;
}
