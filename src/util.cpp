#include "util.h"

#include "QDebug"
#include <QString>

Util::Util()
{

}

const char Util::unescapeTable[] = "a\007b\010e\033f\014n\012r\015t\011v\013\\\134\'\047\"\042\?\077";

int Util::unescapeHelper(QStringRef text, int& result, int baseBits)
{
    int i, n = 0;
    result = 0;
    for(i = 0; i < text.size(); i++)
    {
        n = text[i].toLatin1();
        if(n >= 'a' && n <= 'z' && n - 'a' + 10 < (1 << baseBits))
        {
            result <<= baseBits;
            result += n - 'a' + 10;
        }
        else if(n >= 'A' && n <= 'Z' && n - 'A' + 10 < (1 << baseBits))
        {
            result <<= baseBits;
            result += n - 'A' + 10;
        }
        else if(n >= '0' && n <= '9' && n - '0' < (1 << baseBits))
        {
            result <<= baseBits;
            result += n - '0';
        }
        else
            break;
    }
    return qMin(i, text.size());
}

QString Util::unescape(const QString &text)
{
    QString result;

    for(int i = 0; i < text.size(); i++)
    {
        // keep the Qchar unchanged
        if(text[i] != '\\' || i + 1 >= text.size())
        {
            result += text[i];
            continue;
        }
        // '\' is not at the end, process
        // see https://en.wikipedia.org/wiki/Escape_sequences_in_C#Table_of_escape_sequences

        // 1. \a, \b, \e, \f, \n, \r, \t, \v, \\, \', \", \?
        size_t j;
        for(j = 0; j < sizeof(unescapeTable) / 2; j++)
        {
            if(text[i + 1] == unescapeTable[j * 2])
            {
                result += unescapeTable[j * 2 + 1];
                i++;
                break;
            }
        }
        if(j < sizeof(unescapeTable) / 2) // processed
            continue;

        int handled, ch;
        // 2. \nnn, 1~3 octal digits
        if(i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '7')
        {
            handled = unescapeHelper(text.midRef(i + 1, qMin(3, text.size() - (i + 1))), ch, 3);
            i += handled;
            if(handled)
                result += QChar(ch & 0xFF);
        }
        // 3. \xHH, 1~2 hexadecimal digits
        else if(i + 2 < text.size() && text[i + 1] == 'x')
        {
            handled = unescapeHelper(text.midRef(i + 2, qMin(2, text.size() - (i + 2))), ch, 4);
            if(handled)
            {
                result += QChar(ch);
                i += handled + 1; // including 'x'
            }
        }
        // 4. \uHHHH, exact 4 hexadecimal digits
        else if(i + 5 < text.size() && text[i + 1] == 'u')
        {
            bool isOk;
            ch = text.midRef(i + 2, 4).toInt(&isOk, 16);
            if(isOk)
            {
                result += QChar(ch);
                i += 5;
            }
        }
        // 5. invalid \, keep it unchanged
        else
        {
            result += text[i];
        }
    }
    qDebug() << result.toUtf8().toHex(' ');
    qDebug() << result.toLatin1().toHex(' ');
    qDebug() << result;
    return result;
}
