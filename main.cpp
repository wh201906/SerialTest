#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    MainWindow w;
#ifdef Q_OS_ANDROID
    w.setWindowState(Qt::WindowFullScreen);
    w.showFullScreen();
#else
    w.show();
#endif
    return a.exec();
}
