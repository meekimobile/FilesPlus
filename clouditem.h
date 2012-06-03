#ifndef CloudItem_H
#define CloudItem_H

#include <QDataStream>
#include <QDebug>

class CloudItem
{
public:
    enum ClientTypes {
        Dropbox,
        GoogleDrive,
        SkyDrive,
        SugarSync
    };

    CloudItem();
    CloudItem(int type, QString uid, QString localPath, QString remotePath, QString hash);

    bool operator==(const CloudItem &item);

    int type;
    QString uid;
    QString localPath;
    QString remotePath;
    QString hash;
};

QDataStream &operator<<(QDataStream &out, const CloudItem &item);
QDataStream &operator>>(QDataStream &in, CloudItem &item);
QDebug &operator<<(QDebug &out, const CloudItem &t);

#endif // CloudItem_H
