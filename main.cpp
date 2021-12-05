#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
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
