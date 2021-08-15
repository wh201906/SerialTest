#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QList>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QTimer>
#include <QScrollBar>
#include <QDockWidget>
#include <QSettings>
#include <QClipboard>

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
private slots:
    void refreshPortsInfo();
    void on_portTable_cellDoubleClicked(int row, int column);

    void on_advancedBox_clicked(bool checked);

    void on_openButton_clicked();

    void on_closeButton_clicked();

    void readData();
    void on_sendButton_clicked();


    void onErrorOccurred(QSerialPort::SerialPortError error);
    void on_sendedHexBox_stateChanged(int arg1);

    void on_receivedHexBox_stateChanged(int arg1);

    void on_receivedClearButton_clicked();

    void on_sendedClearButton_clicked();

    void on_suffixCharEdit_textChanged(const QString &arg1);

    void on_suffixByteEdit_textChanged(const QString &arg1);

    void on_sendEdit_textChanged(const QString &arg1);

    void on_repeatBox_stateChanged(int arg1);

    void on_receivedCopyButton_clicked();

    void on_sendedCopyButton_clicked();

    void on_receivedExportButton_clicked();

    void on_sendedExportButton_clicked();

    void onTopBoxClicked(bool checked);

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
private:
    Ui::MainWindow *ui;
    QSerialPort* port;
    QSerialPortInfo* info;
    void initUI();
    void stateUpdate();
    bool portState;

    QScrollBar* RxSlider;
    int currRxSliderPos = 0;
    int userRequiredRxSliderPos = 0;

    QLabel* portLabel;
    QPushButton* stateButton;
    QLabel* baudRateLabel;
    QLabel* dataBitsLabel;
    QLabel* stopBitsLabel;
    QLabel* parityLabel;
    QLabel* TxLabel;
    QLabel* RxLabel;
    QCheckBox* onTopBox;

    QByteArray* rawReceivedData;
    QByteArray* rawSendedData;
    QByteArray* RxUIBuf;
    QString* plotBuf;
    quint64 plotCounter;
    QCPItemTracer* plotTracer;
    int plotSelectedId = 0;
    QString plotSelectedName;
    QString plotFrameSeparator;
    QString plotDataSeparator;
    double plotXAxisWidth;
    QSharedPointer<QCPAxisTickerTime> plotTimeTicker = QSharedPointer<QCPAxisTickerTime>(new QCPAxisTickerTime);
    QSharedPointer<QCPAxisTicker> plotDefaultTicker;
    QTime plotTime;
    int hexCounter = 0;

    QTimer* repeatTimer;
    QTimer* updateUITimer;

    QList<QDockWidget*> dockList;
    QSettings* settings;

    bool isReceivedDataHex = false;
    bool isSendedDataHex = false;
    void dockInit();
    void loadPreference(const QString &id);
    void savePreference(const QString &portName);
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
};
#endif // MAINWINDOW_H
