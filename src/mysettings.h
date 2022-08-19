#ifndef MYSETTINGS_H
#define MYSETTINGS_H

#include <QSettings>

// the QSettings has its singleton mode(setPath/setOrganizationName then QSettings()).
// However, the setPath() with INI format will store my config file to a platform specified path.
// I can't change the path, so I create a new settings with singleton.

class MySettings
{
public:
    static bool init(QSettings::Format format, const QString &path = QString());
    static MySettings *defaultSettings();

    bool setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void beginGroup(const QString &prefix);
    void endGroup();
    QString group() const;
    QStringList childGroups() const;
    void clear();
    QString fileName() const;
    void sync();
    QStringList allKeys() const;
    int beginReadArray(const QString &prefix);
    void beginWriteArray(const QString &prefix, int size = -1);
    void endArray();
    void setArrayIndex(int i);
    void remove(const QString &key);
private:
    MySettings() {}
    MySettings(const MySettings&) {}
    MySettings& operator=(const MySettings&)
    {
        return *this;
    };
    ~MySettings();

    static MySettings* m_inst;
    static QSettings * m_settings;
};

#endif // MYSETTINGS_H
