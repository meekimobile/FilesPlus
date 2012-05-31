#include "tokenpair.h"

TokenPair::TokenPair()
{
}

QDataStream &operator<<(QDataStream &out, const TokenPair &t)
{
    out << t.token << t.secret;

    return out;
}

QDataStream &operator>>(QDataStream &in, TokenPair &t)
{
    t = TokenPair();
    in >> t.token >> t.secret;

    return in;
}

QDebug &operator<<(QDebug &out, const TokenPair &t)
{
    out << "TokenPair(" << t.token << "," << t.secret << ")";

    return out;
}
