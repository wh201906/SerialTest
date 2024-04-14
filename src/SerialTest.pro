QT += core gui serialport bluetooth network printsupport
android {
    QT += androidextras
}


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32-msvc*: {
    # For Bluetooth LE in WinRT
    CONFIG += c++17
    SOURCES += winrtbluetooth.cpp
    HEADERS += winrtbluetooth.h
    LIBS += -lwindowsapp
} else {
    CONFIG += c++11
}


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
    adaptivestackedwidget.cpp \
    asynccrc.cpp \
    connection.cpp \
    controlitem.cpp \
    ctrltab.cpp \
    datatab.cpp \
    devicetab.cpp \
    filetab.cpp \
    filexceiver.cpp \
    legenditemdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    metadata.cpp \
    mycustomplot.cpp \
    mysettings.cpp \
    plottab.cpp \
    serialpinout.cpp \
    settingstab.cpp \
    util.cpp

HEADERS += \
    metadata.h \
    adaptivestackedwidget.h \
    asynccrc.h \
    connection.h \
    controlitem.h \
    ctrltab.h \
    datatab.h \
    devicetab.h \
    filetab.h \
    filexceiver.h \
    legenditemdialog.h \
    mainwindow.h \
    mycustomplot.h \
    mysettings.h \
    plottab.h \
    serialpinout.h \
    settingstab.h \
    util.h

FORMS += \
    ui/settingstab.ui \
    ui/filetab.ui \
    ui/legenditemdialog.ui \
    ui/serialpinout.ui \
    ui/devicetab.ui \
    ui/datatab.ui \
    ui/ctrltab.ui \
    ui/controlitem.ui \
    ui/mainwindow.ui \
    ui/plottab.ui

TRANSLATIONS += \
    i18n/SerialTest_zh_CN.ts \
    i18n/SerialTest_zh_TW.ts \
    i18n/SerialTest_nb_NO.ts

RC_ICONS = icon/icon.ico
ICON = icon/icon.icns

# Rules for deployment.
qnx {
    target.path = /tmp/$${TARGET}/bin
} else: unix:!android {
    # if PREFIX is specified, use PREFIX
    isEmpty(PREFIX): target.path = /opt/$${TARGET}/bin
    else: target.path = $${PREFIX}/bin
}
!isEmpty(target.path) {
    INSTALLS += target
    message(Install path: $${target.path})
}

# Remember to change version in AndroidManifest.xml
VERSION = 0.3.5
QMAKE_TARGET_PRODUCT = "SerialTest"
QMAKE_TARGET_DESCRIPTION = "SerialTest"
QMAKE_TARGET_COMPANY = "wh201906"
# Expose VERSION to the source files.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/java/priv/wh201906/serialtest/MainActivity.java \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-zh-rCN/strings.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

RESOURCES += \
    i18n/language.qrc \
    qdarkstyle/dark/darkstyle.qrc \
    qdarkstyle/light/lightstyle.qrc

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

    # /app/lib/ is for Flatpak
    # no need to worry if /app/lib/ exists or not
    LIBS += -L/app/lib/ -L./ -l$$QCPLIB
}
