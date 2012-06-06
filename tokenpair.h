#ifndef TOKENPAIR_H
#define TOKENPAIR_H

#include <QDataStream>
#include <QDebug>

class TokenPair
{
public:
    TokenPair();

    QString token;
    QString secret;
    QString email;

    QString toJsonText();
};

QDataStream &operator<<(QDataStream &out, const TokenPair &t);
QDataStream &operator>>(QDataStream &in, TokenPair &t);
QDebug &operator<<(QDebug &out, const TokenPair &t);

#endif // TOKENPAIR_H
