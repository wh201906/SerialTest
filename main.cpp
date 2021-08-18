#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#ifndef Q_OS_ANDROID
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
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
