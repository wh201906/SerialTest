#include "mysettings.h"

MySettings *MySettings::m_inst = nullptr;
QSettings *MySettings::m_settings = nullptr;

bool MySettings::init(QSettings::Format format, const QString &path)
{
    if(format == QSettings::IniFormat)
    {
        m_settings = new QSettings(path, format);
        m_settings->setIniCodec("UTF-8");
    }
    else
        m_settings = new QSettings("wh201906", "SerialTest");
    if(m_settings->status() != QSettings::NoError)
    {
        delete m_settings;
        m_settings = nullptr;
        return false;
    }
    else
    {
        m_inst = new MySettings;
        return true;
    }
}

MySettings::~MySettings()
{
    if(m_settings)
    {
        delete m_settings;
        m_settings = nullptr;
    }
    if(m_inst)
        m_inst = nullptr;
}

MySettings *MySettings::defaultSettings()
{
    return m_inst;
}

bool MySettings::setValue(const QString &key, const QVariant &value)
{
    if(m_settings == nullptr)
        return false;
    m_settings->setValue(key, value);
    return true;
}

QVariant MySettings::value(const QString &key, const QVariant &defaultValue) const
{
    if(m_settings == nullptr)
        return QVariant();
    return m_settings->value(key, defaultValue);
}

void MySettings::beginGroup(const QString &prefix)
{
    if(m_settings == nullptr)
        return;
    m_settings->beginGroup(prefix);
}

void MySettings::endGroup()
{
    if(m_settings == nullptr)
        return;
    m_settings->endGroup();
}

QString MySettings::group() const
{
    if(m_settings == nullptr)
        return QString();
    return m_settings->group();
}

QStringList MySettings::childGroups() const
{
    if(m_settings == nullptr)
        return QStringList();
    return m_settings->childGroups();
}

void MySettings::clear()
{
    if(m_settings == nullptr)
        return;
    m_settings->clear();
}

QString MySettings::fileName() const
{
    if(m_settings == nullptr)
        return QString();
    return m_settings->fileName();
}

void MySettings::sync()
{
    if(m_settings == nullptr)
        return;
    m_settings->sync();
}

QStringList MySettings::allKeys() const
{
    if(m_settings == nullptr)
        return QStringList();
    return m_settings->allKeys();
}

int MySettings::beginReadArray(const QString &prefix)
{
    if(m_settings == nullptr)
        return 0;
    return m_settings->beginReadArray(prefix);
}

void MySettings::beginWriteArray(const QString &prefix, int size)
{
    if(m_settings == nullptr)
        return;
    m_settings->beginWriteArray(prefix, size);
}

void MySettings::endArray()
{
    if(m_settings == nullptr)
        return;
    m_settings->endArray();
}

void MySettings::setArrayIndex(int i)
{
    if(m_settings == nullptr)
        return;
    m_settings->setArrayIndex(i);
}

void MySettings::remove(const QString &key)
{
    if(m_settings == nullptr)
        return;
    m_settings->remove(key);
}
