#include "util.h"

#include "QDebug"
#include <QString>
#include <QGestureEvent>
#include <QTouchEvent>
#include <QContextMenuEvent>
#include <QApplication>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QFileInfo>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#endif

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

QByteArray Util::unescape(const QString &text, QTextCodec* codec)
{
    QByteArray result;

    for(int i = 0; i < text.size(); i++)
    {
        // keep the normal string unchanged
        if(text[i] != '\\' || i + 1 >= text.size())
        {
            int end = i;
            while(end < text.size() && text[end] != '\\')
                end++;
            result += codec->fromUnicode(QStringRef(&text, i, end - i));
            i = end - 1;
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
        // 2. \nnn, 1~3 octal digits for a byte
        if(i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '7')
        {
            handled = unescapeHelper(text.midRef(i + 1, qMin(3, text.size() - (i + 1))), ch, 3);
            i += handled;
            if(handled)
                result += ch & 0xFF;
        }
        // 3. \xHH, 1~2 hexadecimal digits for a byte
        else if(i + 2 < text.size() && text[i + 1] == 'x')
        {
            handled = unescapeHelper(text.midRef(i + 2, qMin(2, text.size() - (i + 2))), ch, 4);
            if(handled)
            {
                result += ch & 0xFF;
                i += handled + 1; // including 'x'
            }
        }
        // 4. \uHHHH, exact 4 hexadecimal digits for a unicode char
        // might be slow
        else if(i + 5 < text.size() && text[i + 1] == 'u')
        {
            bool isOk;
            ch = text.midRef(i + 2, 4).toInt(&isOk, 16);
            if(isOk)
            {
                QChar qch(ch); // treat it like QString with length=1
                result += codec->fromUnicode(QStringView(&qch, 1));
                i += 5;
            }
        }
        // 5. invalid \, keep it unchanged
        else
        {
            result += codec->fromUnicode(QStringRef(&text, i, 1));
        }
    }
//    qDebug() << result.toHex(' ');
//    qDebug() << result;
    return result;
}

void Util::disableItem(QStandardItemModel* model, int id, bool enabled)
{
    if(model == nullptr)
        return;
    QStandardItem *item = model->item(id);
    Qt::ItemFlags flags = item->flags();
    flags.setFlag(Qt::ItemIsEnabled, enabled);
    item->setFlags(flags);
}


#ifdef Q_OS_ANDROID
void Util::showToast(const QString& message, bool isLong)
{
    // all the magic must happen on Android UI thread
    // don't capture by reference there
    QtAndroid::runOnAndroidThread([ = ]
    {
        QAndroidJniObject javaString = QAndroidJniObject::fromString(message);
        QAndroidJniObject toast = QAndroidJniObject::callStaticObjectMethod("android/widget/Toast", "makeText",
                "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;",
                QtAndroid::androidActivity().object(),
                javaString.object(),
                jint(isLong ? 1 : 0));
        toast.callMethod<void>("show");
    });
}
#endif

QString Util::getValidLocalFilename(const QList<QUrl>& urlList)
{
    for(auto url : urlList)
    {
        if(url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile())
            return url.toLocalFile();
    }
    return QString();
}

// use TapAndHold gesture to show the context menu
// call widget->grabGesture(Qt::TapAndHoldGesture) then use the event filter
// for QLineEdit, the edit menu will be shown
// for QPlainTextEdit, the parent's context menu will be shown
// useless now...

bool GestureConverter::eventFilter(QObject *obj, QEvent *event)
{
    qDebug() << obj->objectName() << event->type();
    if(event->type() == QEvent::Gesture || event->type() == QEvent::GestureOverride)
    {
        QGestureEvent *ge = static_cast<QGestureEvent*>(event);
        qDebug() << obj->objectName() << ge->gestures();
        QGesture *ges = ge->gesture(Qt::TapAndHoldGesture);
        if(ges->state() == Qt::GestureFinished)
        {
            QContextMenuEvent newEvent(QContextMenuEvent::Mouse, ge->mapToGraphicsScene(ges->hotSpot()).toPoint());
            QApplication::sendEvent(obj, &newEvent);
        }
    }
    return false;
}
