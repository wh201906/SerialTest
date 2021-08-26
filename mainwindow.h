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
#include <QSettings>
#include <QDockWidget>
#endif

#include "qcustomplot.h"

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
#ifdef Q_OS_ANDROID
    void BTdeviceDiscovered(const QBluetoothDeviceInfo &device);
    void BTdiscoverFinished();
#endif

private slots:
    void refreshPortsInfo();

    void on_portTable_cellDoubleClicked(int row, int column);

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

    void on_data_repeatBox_clicked(bool checked);

    void on_receivedCopyButton_clicked();

    void on_sendedCopyButton_clicked();

    void on_receivedExportButton_clicked();

    void on_sendedExportButton_clicked();

    void onStateButtonClicked();

    void on_plot_dataNumBox_valueChanged(int arg1);

    void on_plot_clearButton_clicked();

    void on_plot_legendCheckBox_stateChanged(int arg1);

    void on_plot_advancedBox_stateChanged(int arg1);

    void onQCPLegendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item);

    void onQCPMouseMoved(QMouseEvent *event);

    void on_plot_tracerCheckBox_stateChanged(int arg1);

    void on_plot_fitXButton_clicked();

    void on_plot_fitYButton_clicked();

    void onQCPSelectionChanged();

    void on_plot_frameSpTypeBox_currentIndexChanged(int index);

    void on_plot_dataSpTypeBox_currentIndexChanged(int index);

    void updateRxUI();

    void on_receivedUpdateButton_clicked();

    void onXAxisChangedByUser(const QCPRange &newRange);

    void on_plot_XTypeBox_currentIndexChanged(int index);

    void onQCPAxisDoubleClick(QCPAxis *axis);


#ifdef Q_OS_ANDROID
    void onBTConnectionChanged();
#else
    void onTopBoxClicked(bool checked);

    void onSerialErrorOccurred(QSerialPort::SerialPortError error);
#endif

    void onIODeviceConnected();

    void onIODeviceDisconnected();

    void on_ctrl_addCMDButton_clicked();

    void on_ctrl_addSliderButton_clicked();

    void on_ctrl_addCheckBoxButton_clicked();

    void on_ctrl_addSpinBoxButton_clicked();

    void onCtrlItemDestroyed();
    void sendData(QByteArray &data);
    void on_ctrl_clearButton_clicked();

    void on_data_suffixTypeBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    void initUI();
    void stateUpdate();
    bool IODeviceState;

    QIODevice* IODevice;

    QScrollBar* RxSlider;
    int currRxSliderPos = 0;
    int userRequiredRxSliderPos = 0;

    QLabel* portLabel;
    QPushButton* stateButton;
    QLabel* TxLabel;
    QLabel* RxLabel;

    QByteArray* rawReceivedData;
    QByteArray* rawSendedData;
    QByteArray* RxUIBuf;
    QString* plotBuf;
    quint64 plotCounter;
    QCPItemTracer* plotTracer;
    QCPItemText* plotText;
    int plotSelectedId = 0;
    QString plotSelectedName;
    QString plotFrameSeparator;
    QString plotDataSeparator;
    double plotXAxisWidth;
    QSharedPointer<QCPAxisTickerTime> plotTimeTicker = QSharedPointer<QCPAxisTickerTime>(new QCPAxisTickerTime);
    QSharedPointer<QCPAxisTicker> plotDefaultTicker;
    QTime plotTime;
    int hexCounter = 0;

    int ctrlItemCount = 0;

    QTimer* repeatTimer;
    QTimer* updateUITimer;

    bool isReceivedDataHex = false;
    bool isSendedDataHex = false;
    void appendReceivedData(QByteArray &data);
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
    QSettings* settings;

    void dockInit();
    void loadPreference(const QString &id);
    void savePreference(const QString &portName);

    QLabel* baudRateLabel;
    QLabel* dataBitsLabel;
    QLabel* stopBitsLabel;
    QLabel* parityLabel;
    QCheckBox* onTopBox;

#endif

    void updateTracer(double x);
};
#endif // MAINWINDOW_H
