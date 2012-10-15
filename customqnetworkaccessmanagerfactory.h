#ifndef CUSTOMQNETWORKACCESSMANAGERFACTORY_H
#define CUSTOMQNETWORKACCESSMANAGERFACTORY_H

#include <QDeclarativeNetworkAccessManagerFactory>
#include "customqnetworkaccessmanager.h"

class CustomQNetworkAccessManagerFactory : public QDeclarativeNetworkAccessManagerFactory
{
public:
    virtual QNetworkAccessManager *create(QObject *parent);
};

#endif // CUSTOMQNETWORKACCESSMANAGERFACTORY_H
