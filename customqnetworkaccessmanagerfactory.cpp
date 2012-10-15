#include "customqnetworkaccessmanagerfactory.h"

QNetworkAccessManager *CustomQNetworkAccessManagerFactory::create(QObject *parent)
{
    CustomQNetworkAccessManager *nam = new CustomQNetworkAccessManager(parent);

    return nam;
}
