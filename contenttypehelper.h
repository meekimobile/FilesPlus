#ifndef CONTENTTYPEHELPER_H
#define CONTENTTYPEHELPER_H

#include <QHash>

class ContentTypeHelper
{
public:
    ContentTypeHelper();

    QHash<QString, QString> loadMimeTypes(const QString &localPath);
};

#endif // CONTENTTYPEHELPER_H
