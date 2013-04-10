#include "contenttypehelper.h"
#include <QDir>
#include <QDebug>
#include <QRegExp>

ContentTypeHelper::ContentTypeHelper()
{
}

QHash<QString, QString> ContentTypeHelper::parseContentTypeHash(const QString &localPath)
{
    qDebug() << "ContentTypeHelper::parseContentTypeHash localPath " << localPath;

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
                    qDebug() << "ContentTypeHelper::parseContentTypeHash accepted line" << line << tokens;
                    for (int i = 1; i < tokens.count(); i++) {
                        contentTypeHash.insert(tokens.at(i), tokens.at(0));
                        qDebug() << "ContentTypeHelper::parseContentTypeHash inserted" << tokens.at(i) << "=" << tokens.at(0);
                    }
                } else {
//                    qDebug() << "ContentTypeHelper::parseContentTypeHash skipped line" << line << tokens;
                }
            }

            // Read next line.
            line = in.readLine();
        }
    }
    file.close();

    qDebug() << "ContentTypeHelper::parseContentTypeHash contentTypeHash.count()" << contentTypeHash.count();

    return contentTypeHash;
}
