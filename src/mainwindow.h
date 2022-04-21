#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDebug>
#include <QList>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothUuid>
#include <QBluetoothSocket>
#else
#include <QSerialPort>
#include <QSerialPortInfo>
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
#ifdef Q_OS_ANDROID
    void openDevice(const QString &name);
#else
    void openDevice(const QString &name, const qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits, QSerialPort::Parity parity, QSerialPort::FlowControl flowControl);
#endif
    void closeDevice();
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
private slots:
    void readData();
    void onStateButtonClicked();
    void updateRxUI();

#ifdef Q_OS_ANDROID

#else
    void onTopBoxClicked(bool checked);
    void onSerialErrorOccurred(QSerialPort::SerialPortError error);
#endif

    void onIODeviceConnected();
    void onIODeviceDisconnected();
private:
    Ui::MainWindow *ui;
    void initUI();
    void stateUpdate();

    QMenu* contextMenu;
    QAction* dockAllWindows;
    QAction* myInfo;
    QAction* currVersion;
    QAction* checkUpdate;

    bool IODeviceState;
    Connection* IOConnection;

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

    enum tableHeader
    {
        HPortName = 0,
        HDescription,
        HManufacturer,
        HSerialNumber,
        HIsNull,
        HSystemLocation,
        HVendorID,
        HProductID,
        HBaudRates
    };

#ifdef Q_OS_ANDROID

#else
    QList<QDockWidget*> dockList;
    QLabel* baudRateLabel;
    QLabel* dataBitsLabel;
    QLabel* stopBitsLabel;
    QLabel* parityLabel;
    SerialPinout* serialPinout;
    QCheckBox* onTopBox;

    void dockInit();
#endif
    void initTabs();
};
#endif // MAINWINDOW_H
