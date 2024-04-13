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

    struct SP_ID
    {
        // identify a serial port device
        quint16 m_vid = 0, m_pid = 0;
        QString m_serialNumber;

        SP_ID(quint16 vid, quint16 pid, const QString& serialNumber) :
            m_vid(vid), m_pid(pid), m_serialNumber(serialNumber) {}
        SP_ID(const QSerialPortInfo &info) :
            SP_ID(info.vendorIdentifier(), info.productIdentifier(), info.serialNumber()) {}
        SP_ID(const QString& str);
        QString toString() const;
        // (bool)(a.matches(b)) means a can use b's arguments
        // 0: unmatch 1: match 2: the same
        quint8 matches(const SP_ID& id) const;
        explicit operator bool() const; // SP_ID is invalid or not
    };
    static const QMap<QString, QString> m_historyPrefix;

    explicit DeviceTab(QWidget *parent = nullptr);
    ~DeviceTab();

    void initSettings();
    void setConnection(Connection* conn);
public slots:
    void refreshTargetList();
    void saveTCPClientPreference(const Connection::NetworkArgument &arg);
    void saveUDPPreference(const Connection::NetworkArgument &arg);
    void saveSPPreference(const Connection::SerialPortArgument& arg);
    void getAvailableTypes(bool useFirstValid = false);
    void onClientCountChanged();
    void Net_onDeleteButtonClicked();
    void syncUDPPreference();
    void syncTCPClientPreference();
    void setTouchScroll(bool enabled);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    Ui::DeviceTab *ui;

    MySettings* settings;
    Connection* m_connection = nullptr;

    QIntValidator* m_netPortValidator;

    QBluetoothDeviceDiscoveryAgent *BTClient_discoveryAgent = nullptr;
    QHash<QString, int> m_shownBTDevices;

    QLowEnergyController *m_BLEController = nullptr;
    QHash<QBluetoothUuid, QTreeWidgetItem*> m_discoveredBLEServices;

    const QString m_autoLocalAddress = tr("(Auto)");
    const QString m_anyLocalAddress = tr("(Any)");

    int m_maxHistoryNum;
    // default value is defined in initSettings() and SettingsTab::loadPreference()
    QList<Connection::SerialPortArgument> m_SPArgHistory;
    QMap<QString, int> m_SPArgHistoryIndex;
    QList<Connection::BTArgument> m_BLECArgHistory;
    QMap<QString, int> m_BLECArgHistoryIndex;
    QList<Connection::NetworkArgument> m_TCPClientHistory, m_UDPHistory;

    bool m_isBLECLoaded = false;

    void initUI();
#ifdef Q_OS_ANDROID
    bool getPermission(const QString& permission);
    void getRequiredPermission();
    void getBondedTarget(bool isBLE);
#endif
    void setBTClientDiscoveryAgent(QBluetoothAddress adapterAddress = QBluetoothAddress());
    void BLEC_addService(const QBluetoothUuid &serviceUUID, QTreeWidgetItem *parentItem = nullptr);
    void BLEC_addCharacteristic(const QLowEnergyCharacteristic& c, QTreeWidgetItem *parentItem);
    void BLEC_addDescriptor(const QLowEnergyDescriptor &descriptor, QTreeWidgetItem *parentItem);
    QString BLE_getCharacteristicPropertyString(const QLowEnergyCharacteristic &c);
    qint64 updateBTAdapterList();
    qint64 updateNetInterfaceList();
    QBluetoothUuid String2UUID(const QString &string);
    QString UUID2String(const QBluetoothUuid &UUID);
    void loadSPPreference(const Connection::SerialPortArgument &arg = Connection::SerialPortArgument(), bool loadPortName = true);
    void loadNetPreference(const Connection::NetworkArgument &arg, Connection::Type type);
    void showNetArgumentHistory(const QList<Connection::NetworkArgument> &arg, Connection::Type type);
    SP_ID SP_getPortID(int rowInList);
    bool SP_hasDuplicateID(int rowInList);
    bool SP_hasDuplicateID(const SP_ID& spid);
    int SP_getMatchedHistoryIndex(int rowInList);
signals:
    void connTypeChanged(Connection::Type type);
    void argumentChanged();
    void clientCountChanged();
private slots:
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
    void on_BTClient_serviceUUIDBox_clicked();
};

#endif // DEVICETAB_H
