#ifndef DEVICETAB_H
#define DEVICETAB_H

#include <QWidget>
#ifdef Q_OS_ANDROID
#include <QBluetoothDeviceDiscoveryAgent>
#else
#include <QSerialPortInfo>
#include <QSerialPort>
#endif

#include "mysettings.h"

namespace Ui
{
class DeviceTab;
}

class DeviceTab : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceTab(QWidget *parent = nullptr);
    ~DeviceTab();

    void initSettings();
public slots:
    void refreshDevicesInfo();
#ifndef Q_OS_ANDROID
    void saveDevicesPreference(const QString &deviceName);
#endif
private:
    Ui::DeviceTab *ui;

    enum tableHeader
    {
        HDeviceName = 0,
        HDescription,
        HManufacturer,
        HSerialNumber,
        HIsNull,
        HSystemLocation,
        HVendorID,
        HProductID,
        HBaudRates
    };

    MySettings* settings;

#ifdef Q_OS_ANDROID
    QBluetoothDeviceDiscoveryAgent *BTdiscoveryAgent;
#endif

#ifndef Q_OS_ANDROID
    void loadDevicesPreference(const QString &id);
#endif

    void initUI();
signals:
    void closeDevice();
#ifdef Q_OS_ANDROID
    void openDevice(const QString &name);
#else
    void openDevice(const QString &name, const qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits, QSerialPort::Parity parity, QSerialPort::FlowControl flowControl);
#endif
private slots:
    void on_advancedBox_clicked(bool checked);
    void on_openButton_clicked();
    void on_closeButton_clicked();
    void on_deviceTable_cellClicked(int row, int column);

#ifdef Q_OS_ANDROID
    void BTdiscoverFinished();
    void BTdeviceDiscovered(const QBluetoothDeviceInfo &device);
#endif
};

#endif // DEVICETAB_H
