#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QTextCodec>
#include <QStandardItemModel>

class Util
{
public:
    Util();
    static QByteArray unescape(const QString& text, QTextCodec* codec);
    static void disableItem(QStandardItemModel* model, int id, bool enabled = false);
#ifdef Q_OS_ANDROID
    static void showToast(const QString &message, bool isLong = false);
#endif
    static QString getValidLocalFilename(const QList<QUrl>& urlList);
private:
    static const char unescapeTable[];
    static int unescapeHelper(QStringRef text, int &result, int baseBits);
};

class GestureConverter : public QObject
{
    Q_OBJECT

protected:
    bool eventFilter(QObject* obj, QEvent *event) override;
};

#endif // UTIL_H
