#ifndef CUSTOMQNETWORKACCESSMANAGER_H
#define CUSTOMQNETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>

class CustomQNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit CustomQNetworkAccessManager(QObject *parent = 0);
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData);
signals:
    
public slots:

private:
    QSettings m_settings;
};

#endif // CUSTOMQNETWORKACCESSMANAGER_H
