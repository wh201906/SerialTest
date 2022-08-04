#ifndef DEVICETAB_H
#define DEVICETAB_H

#include <QWidget>
#include <QTreeWidget>
#include <QBluetoothHostInfo>
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
    void getAvailableTypes(bool useFirstValid = false);
    void onClientCountChanged();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    Ui::DeviceTab *ui;

    MySettings* settings;
    Connection* m_connection = nullptr;

    QBluetoothDeviceDiscoveryAgent *BTClient_discoveryAgent = nullptr;
    QHash<QString, int> m_shownBTDevices;

    QLowEnergyController *m_BLEController = nullptr;
    QHash<QBluetoothUuid, QTreeWidgetItem*> m_discoveredBLEServices;

    const QString m_autoLocalAddress = tr("(Auto)");
    const QString m_anyLocalAddress = tr("(Any)");

    void loadDevicesPreference(const QString &id);

    void initUI();
#ifdef Q_OS_ANDROID
    void getBondedTarget(bool isBLE);
#endif
    void setBTClientDiscoveryAgent(QBluetoothAddress adapterAddress = QBluetoothAddress());
    void BLEC_addService(const QBluetoothUuid &serviceUUID, QTreeWidgetItem *parentItem = nullptr);
    void BLEC_addCharacteristic(const QLowEnergyCharacteristic& c, QTreeWidgetItem *parentItem);
    void BLEC_addDescriptor(const QLowEnergyDescriptor &descriptor, QTreeWidgetItem *parentItem);
    QString BLE_getCharacteristicPropertyString(const QLowEnergyCharacteristic &c);
    qsizetype updateBTAdapterList();
    qsizetype updateNetInterfaceList();
signals:
    void connTypeChanged(Connection::Type type);
    void argumentChanged();
    void clientCountChanged();
private slots:
    void on_SP_advancedBox_clicked(bool checked);
    void on_openButton_clicked();
    void on_closeButton_clicked();
    void onTargetListCellClicked(int row, int column);

    void BTdiscoverFinished();
    void BTdeviceDiscovered(const QBluetoothDeviceInfo &device);
    void on_typeBox_currentIndexChanged(int index);
    void on_refreshButton_clicked();
    void on_BTClient_adapterBox_activated(int index);
    void on_BTServer_serviceNameEdit_editingFinished();
    void on_BTServer_adapterBox_activated(int index);
    void Net_onRemoteChanged();
    void on_Net_localAddrBox_currentIndexChanged(int index);
    void on_SP_baudRateBox_currentIndexChanged(int index);
    void on_SP_dataBitsBox_currentIndexChanged(int index);
    void on_SP_stopBitsBox_currentIndexChanged(int index);
    void on_SP_parityBox_currentIndexChanged(int index);
    void on_SP_flowControlBox_currentIndexChanged(int index);
    void on_BLEC_connectButton_clicked();
    void BLEC_onRootServiceDiscovered(const QBluetoothUuid &newService);
    void BLEC_onServiceDetailDiscovered(QLowEnergyService::ServiceState newState);
    void on_BTServer_deviceList_cellChanged(int row, int column);
    void on_Net_addrPortList_cellChanged(int row, int column);
    void on_BLEC_ServiceUUIDBox_currentTextChanged(const QString &arg1);
};

#endif // DEVICETAB_H
