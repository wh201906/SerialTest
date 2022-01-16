#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QTranslator>

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
    QLocale loc = QLocale::system();
    QTranslator trans;
    if(loc.language() == QLocale::Chinese)
    {
        trans.load(":/lang/SerialTest_zh_CN.qm");
        a.installTranslator(&trans);
    }
    MainWindow w;
#ifdef Q_OS_ANDROID
    w.setWindowState(Qt::WindowFullScreen);
    w.showFullScreen();
#else
    w.show();
#endif
    return a.exec();
}
