#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDebug>
#include <QList>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothUuid>
#include <QBluetoothSocket>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#else
#include <QDockWidget>
#endif

#include "mysettings.h"
#include "plottab.h"
#include "ctrltab.h"
#include "datatab.h"
#include "devicetab.h"
#include "serialpinout.h"
#include "connection.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void sendData(const QByteArray &data);
    void updateStatusBar();
    void updateRxTxLen(bool updateRx = true, bool updateTx = true);
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
private slots:
    void readData();
    void onStateButtonClicked();
    void updateRxUI();

#ifndef Q_OS_ANDROID
    void onTopBoxClicked(bool checked);
#endif

    void onIODeviceConnected();
    void onIODeviceDisconnected();
    void onIODeviceConnectFailed();
private:
    Ui::MainWindow *ui;
    void initUI();

    QMenu* contextMenu;
    QAction* dockAllWindows;
    QAction* myInfo;
    QAction* currVersion;
    QAction* checkUpdate;

    Connection* IOConnection = nullptr;

    QLabel* deviceLabel;
    QPushButton* stateButton;
    QLabel* TxLabel;
    QLabel* RxLabel;

    QByteArray* rawReceivedData;
    QByteArray* rawSendedData;
    QByteArray* RxUIBuf;

    QTimer* updateUITimer;

    MySettings* settings;
    PlotTab* plotTab;
    CtrlTab* ctrlTab;
    DataTab* dataTab;
    DeviceTab* deviceTab;

    QLabel* connArgsLabel;
    SerialPinout* serialPinout;

#ifndef Q_OS_ANDROID
    QList<QDockWidget*> dockList;
    QCheckBox* onTopBox;

    void dockInit();
#endif
    void initTabs();
};
#endif // MAINWINDOW_H
