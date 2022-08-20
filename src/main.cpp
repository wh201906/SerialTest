#include "mainwindow.h"
#include "mysettings.h"

#include <QApplication>
#include <QDir>
#include <QTranslator>
#include <QFileInfo>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
#ifndef Q_OS_ANDROID
    // A trick to handle non-ascii path
    // The application cannot find the plugins when the path contains non ascii characters.
    // However, the plugins will be load after creating MainWindow(or QApplication?).
    // QDir will handle the path correctly.
    QDir* pluginDir = new QDir;
    if(pluginDir->cd("plugins")) // has plugins folder
    {
        qputenv("QT_PLUGIN_PATH", pluginDir->absolutePath().toLocal8Bit());
    }
    delete pluginDir;
#endif

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

#ifdef Q_OS_ANDROID
    // on Android, use default.
    MySettings::init(QSettings::NativeFormat);
#else
    // on PC, store preferences in files for portable use
    // Firstly, find it in current working directory
    QString configPath = "preference.ini";
    if(!QFileInfo::exists(configPath))
    {
        // Then, find it in AppConfigLocation
        configPath = QStandardPaths::locate(QStandardPaths::AppConfigLocation, "preference.ini");
        if(configPath.isEmpty())
        {
            // If no config file is found, create one in current working directory
            configPath = "preference.ini";
        }
    }
    MySettings::init(QSettings::IniFormat, configPath);
#endif

    // set language by config file
    QTranslator translator;
    MySettings* m_settings = MySettings::defaultSettings();
    m_settings->beginGroup("SerialTest");
    QString language = m_settings->value("Lang_Name").toString();
    QString languageFile = m_settings->value("Lang_Path").toString();
    m_settings->endGroup();
    m_settings = nullptr;

    bool languageSet = false;
    if(language == "(sys)")
        languageSet = false;
    if(language == "zh_CN")
        languageSet = translator.load(":/i18n/SerialTest_zh_CN.qm");
    else if(language == "en")
        languageSet = true;
    else if(language == "(ext)")
        languageSet = translator.load(languageFile);

    if(!languageSet)
    {
        // set language by system locale
        QLocale locale = QLocale::system();
        if(locale.language() == QLocale::Chinese || locale.country() == QLocale::China)
            languageSet = translator.load(":/i18n/SerialTest_zh_CN.qm");
    }
    if(languageSet)
        a.installTranslator(&translator);

    MainWindow w;
    w.show();
    return a.exec();
}
