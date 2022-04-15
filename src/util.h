#ifndef UTIL_H
#define UTIL_H

#include <QString>

class Util
{
public:
    Util();
    static QString unescape(const QString& text);
private:
    static const char unescapeTable[];
    static int unescapeHelper(QStringRef text, int &result, int baseBits);
};

#endif // UTIL_H
