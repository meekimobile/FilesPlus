#ifndef CLOUDDRIVEMODELTHREAD_H
#define CLOUDDRIVEMODELTHREAD_H

#include "clouddriveitem.h"
#include "clouddrivemodel.h"

class CloudDriveModelThread : public QObject, public QRunnable
{
    Q_OBJECT
public:
    CloudDriveModelThread(QObject *parent = 0);

    void setNonce(QString nonce);
    QString getNonce();
    void setDirectInvokation(bool flag);
    bool isDirectInvokation();

    void run();
signals:

public slots:
    void start();
private:
    QString m_nonce;
    bool m_isDirectInvokation;
};

#endif // CLOUDDRIVEMODELTHREAD_H
