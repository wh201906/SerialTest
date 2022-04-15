QT       += core gui serialport printsupport
android {
    QT += bluetooth androidextras
    message(Using bluetooth on android)
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    controlitem.cpp \
    ctrltab.cpp \
    datatab.cpp \
    devicetab.cpp \
    legenditemdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    mycustomplot.cpp \
    mysettings.cpp \
    plottab.cpp \
    serialpinout.cpp \
    util.cpp

HEADERS += \
    controlitem.h \
    ctrltab.h \
    datatab.h \
    devicetab.h \
    legenditemdialog.h \
    mainwindow.h \
    mycustomplot.h \
    mysettings.h \
    plottab.h \
    serialpinout.h \
    util.h

FORMS += \
    ui/legenditemdialog.ui \
    ui/serialpinout.ui \
    ui/devicetab.ui \
    ui/datatab.ui \
    ui/ctrltab.ui \
    ui/controlitem.ui \
    ui/mainwindow.ui \
    ui/plottab.ui

TRANSLATIONS += \
    i18n/SerialTest_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Remember to change version in AndroidManifest.xml
VERSION = 0.1.4
QMAKE_TARGET_PRODUCT = "SerialTest"
QMAKE_TARGET_DESCRIPTION = "SerialTest"
QMAKE_TARGET_COMPANY = "wh201906"

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/java/priv/wh201906/serialtest/BTHelper.java \
    android/res/values/libs.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

RESOURCES += \
    i18n/language.qrc

exists(qcustomplot.cpp) {
    # For platforms which don't have qcp library, like Android.

    # Download qcustomplot source file at https://www.qcustomplot.com/index.php/download.
    # Extract the .cpp and .h file in src/ folder,
    # then build this project.
    message(Using qcustomplot sources)

    SOURCES += qcustomplot.cpp
    HEADERS += qcustomplot.h
} else {
    # For platforms which have qcp library. This will increase compile speed.

    # If qcustomplot library is not installed,
    # put the library file(*.so/*.dll) in the building folder,
    # then build this project.
    message(Using qcustomplot library)

    # Tell the qcustomplot header that it will be used as library:
    DEFINES += QCUSTOMPLOT_USE_LIBRARY

    # Link with debug version of qcustomplot if compiling in debug mode, else with release library:

    CONFIG(debug, release|debug) {
        win32:QCPLIB = qcustomplotd2
        else: QCPLIB = qcustomplotd
    } else {
        win32:QCPLIB = qcustomplot2
        else: QCPLIB = qcustomplot
    }

    LIBS += -L./ -l$$QCPLIB
}
