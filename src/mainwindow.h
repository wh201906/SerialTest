#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDebug>
#include <QList>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QTimer>
#include <QScrollBar>
#include <QClipboard>

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
    void onRxSliderValueChanged(int value);
    void onRxSliderMoved(int value);
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

    void on_sendButton_clicked();

    void on_sendedHexBox_stateChanged(int arg1);

    void on_receivedHexBox_stateChanged(int arg1);

    void on_receivedClearButton_clicked();

    void on_sendedClearButton_clicked();

    void on_sendEdit_textChanged(const QString &arg1);

    void on_data_repeatCheckBox_stateChanged(int arg1);

    void on_receivedCopyButton_clicked();

    void on_sendedCopyButton_clicked();

    void on_receivedExportButton_clicked();

    void on_sendedExportButton_clicked();

    void onStateButtonClicked();

    void updateRxUI();

    void on_receivedUpdateButton_clicked();




#ifdef Q_OS_ANDROID
    void onBTConnectionChanged();
#else
    void onTopBoxClicked(bool checked);

    void onSerialErrorOccurred(QSerialPort::SerialPortError error);

    void on_data_flowDTRBox_clicked(bool checked);

    void on_data_flowRTSBox_clicked(bool checked);
#endif

    void onIODeviceConnected();

    void onIODeviceDisconnected();

    void on_data_suffixTypeBox_currentIndexChanged(int index);

    void on_data_encodingSetButton_clicked();

    void saveDataPreference();
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

    QScrollBar* RxSlider;
    int currRxSliderPos = 0;
    int userRequiredRxSliderPos = 0;

    QLabel* portLabel;
    QPushButton* stateButton;
    QLabel* TxLabel;
    QLabel* RxLabel;

    int dataEncodingId = 0;
    QTextCodec* dataCodec = nullptr; // for Tx and generating Rx decoder
    QTextDecoder* RxDecoder = nullptr; // for Rx UI, a multi-byte character might be split.

    QByteArray* rawReceivedData;
    QByteArray* rawSendedData;
    char lastReceivedByte = '\0';
    QByteArray* RxUIBuf;

    int hexCounter = 0;

    QTimer* repeatTimer;
    QTimer* updateUITimer;

    bool isReceivedDataHex = false;
    bool isSendedDataHex = false;
    void appendReceivedData(const QByteArray &data);
    void syncReceivedEditWithData();
    void syncSendedEditWithData();

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
    void setupPlot();

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
    void loadPreference();
};
#endif // MAINWINDOW_H
