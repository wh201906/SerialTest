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
    void BTdeviceDiscovered(const QBluetoothDeviceInfo &device);
    void BTdiscoverFinished();
#endif

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
private slots:
    void refreshPortsInfo();
    void on_portTable_cellClicked(int row, int column);
    void on_advancedBox_clicked(bool checked);
    void on_openButton_clicked();
    void on_closeButton_clicked();
    void readData();
    void onStateButtonClicked();
    void updateRxUI();

#ifdef Q_OS_ANDROID
    void onBTConnectionChanged();
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
    QIODevice* IODevice;

    QLabel* portLabel;
    QPushButton* stateButton;
    QLabel* TxLabel;
    QLabel* RxLabel;

    QByteArray* rawReceivedData;
    QByteArray* rawSendedData;
    QByteArray* RxUIBuf;

    QTimer* updateUITimer;

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
    QBluetoothDeviceDiscoveryAgent *BTdiscoveryAgent;
    QBluetoothSocket* BTSocket;
    QString BTlastAddress;
#else
    QSerialPort* serialPort;
    QSerialPortInfo* serialPortInfo;

    QList<QDockWidget*> dockList;


    void dockInit();
    void loadPortPreference(const QString &id);
    void savePortPreference(const QString &portName);

    QLabel* baudRateLabel;
    QLabel* dataBitsLabel;
    QLabel* stopBitsLabel;
    QLabel* parityLabel;
    QCheckBox* onTopBox;

#endif
    MySettings* settings;
    PlotTab* plotTab;
    CtrlTab* ctrlTab;
    DataTab* dataTab;
    void initTabs();
};
#endif // MAINWINDOW_H
