#include "messageclient.h"

MessageClient::MessageClient(QDeclarativeItem *parent) :
    QDeclarativeItem(parent), m_msgService(new QMessageService())
{
}

bool MessageClient::sendEmail(const QString &recipientEmail, const QString &subject, const QString &body, const QString localFilePath)
{
    QMessage msg;
    msg.setType(QMessage::Email);
    msg.setParentAccountId(QMessageAccount::defaultAccount(msg.type()));
    if (recipientEmail != "") msg.setTo(QMessageAddress(QMessageAddress::Email, recipientEmail));
    msg.setSubject(subject);
    msg.setBody(body);

    m_msgService->compose(msg);
}

bool MessageClient::sendSMS(const QString &recipientNumber, const QString &body)
{
    QMessage msg;
    msg.setType(QMessage::Sms);
    msg.setParentAccountId(QMessageAccount::defaultAccount(msg.type()));
    if (recipientNumber != "") msg.setTo(QMessageAddress(QMessageAddress::Phone, recipientNumber));
    msg.setBody(body);

    m_msgService->compose(msg);
}
