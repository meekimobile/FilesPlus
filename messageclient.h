#ifndef MESSAGECLIENT_H
#define MESSAGECLIENT_H

#include <QDeclarativeItem>
#include <qmessageservice.h>

QTM_USE_NAMESPACE

class MessageClient : public QDeclarativeItem
{
    Q_OBJECT
public:
    explicit MessageClient(QDeclarativeItem *parent = 0);

//    "mailto:" + selectedEmail + "?subject=" + messageSubject + "&body=" + messageBody
//    "sms:" + selectedNumber + "?body=" + messageBody

    Q_INVOKABLE bool sendEmail(const QString &recipientEmail, const QString &subject, const QString &body, const QString localFilePath = "");
    Q_INVOKABLE bool sendSMS(const QString &recipientNumber, const QString &body);
signals:
    
public slots:

private:
    QMessageService *m_msgService;
};

#endif // MESSAGECLIENT_H
