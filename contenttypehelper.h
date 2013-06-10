#ifndef CONTENTTYPEHELPER_H
#define CONTENTTYPEHELPER_H

#include <QHash>

class ContentTypeHelper
{
public:
    // For singleton.
    static ContentTypeHelper* getInstance()
    {
        static ContentTypeHelper singleton;

        return &singleton;
    }

    QHash<QString, QString> m_contentTypeHash;

    static QHash<QString, QString> parseContentTypeHash(const QString &localPath);
    static QString getConfigPath();
    static QString getContentType(QString fileName);
private:
    // For singleton.
    ContentTypeHelper() {}
    ContentTypeHelper(ContentTypeHelper const&);
    void operator=(ContentTypeHelper const&);
};

#endif // CONTENTTYPEHELPER_H
