#ifndef DEVICETAB_H
#define DEVICETAB_H

#include <QWidget>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QSerialPortInfo>
#include <QSerialPort>

#include "mysettings.h"
#include "connection.h"

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
    void setConnection(Connection* conn);
public slots:
    void refreshTargetList();
    void saveDevicesPreference(const QString &deviceName);
private:
    Ui::DeviceTab *ui;

    MySettings* settings;
    Connection* m_connection = nullptr;

    QBluetoothDeviceDiscoveryAgent *BTdiscoveryAgent;

    void loadDevicesPreference(const QString &id);

    void initUI();
#ifdef Q_OS_ANDROID
    void DeviceTab::getBondedTarget();
#endif
signals:

private slots:
    void on_SP_advancedBox_clicked(bool checked);
    void on_openButton_clicked();
    void on_closeButton_clicked();
    void on_SP_portList_cellClicked(int row, int column);

    void BTdiscoverFinished();
    void BTdeviceDiscovered(const QBluetoothDeviceInfo &device);
    void on_typeBox_currentIndexChanged(int index);
};

#endif // DEVICETAB_H
