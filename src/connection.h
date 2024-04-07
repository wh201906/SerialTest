#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QTimer>
#include <QSerialPort>
#include <QBluetoothSocket>
#include <QBluetoothServer>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUdpSocket>
#include <QDataStream>
#include <QDebug>

class Connection : public QObject
{
    Q_OBJECT
public:
    enum Type
    {
        SerialPort = 0,
        BT_Client,
        BT_Server,
        BLE_Central,
        BLE_Peripheral,
        TCP_Client,
        TCP_Server,
        UDP
    };
    Q_ENUM(Type)

    enum State
    {
        Unconnected = 0,
        Connecting,
        Connected,
        Bound,
    };

    struct SerialPortArgument
    {
        QString name;
        qint32 baudRate = 9600;
        QSerialPort::DataBits dataBits = QSerialPort::Data8;
        QSerialPort::StopBits stopBits = QSerialPort::OneStop;
        QSerialPort::Parity parity = QSerialPort::NoParity;
        QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;
        QString id; // <name> or <VID>-<PID>[-<serialNumber>]
    };

    struct BTArgument
    {
        // for Bluetooth classic and BLE
        QBluetoothAddress deviceAddress;
        // for Bluetooth classic server
        QString serverServiceName;
        QBluetoothAddress localAdapterAddress;
        // for BLE(RxServiceUUID) and Bluetooth classic(serviceUUID)
        QBluetoothUuid RxServiceUUID;
        // for BLE only
        QBluetoothUuid RxCharacteristicUUID;
        QBluetoothUuid TxServiceUUID, TxCharacteristicUUID;
    };

    struct NetworkArgument
    {
        QHostAddress localAddress = QHostAddress::Null;
        // connectToHost() supports hostName and hostAddress, writeDatagram() only supports hostAddress
        // use QString to store all hostName/hostAddress
        QString remoteName;
        quint16 remotePort = 37280, localPort = 0;
        QString alias; // useless for connection
        bool operator==(const NetworkArgument& other) const;
    };

    explicit Connection(QObject *parent = nullptr);

    // general
    Type type();
    bool setType(Type type);
    bool isConnected();
    State state();
    bool polling();
    void setPollingInterval(int msec);
    int pollingInterval();
    static QString getTypeName(Type type);
    static const QMap<Connection::Type, QLatin1String>& getTypeNameMap();
    QStringList getErrorStringList() const;

    // connection
    SerialPortArgument getSerialPortArgument();
    BTArgument getBTArgument();
    NetworkArgument getNetworkArgument(bool fillLocalAddress = true, bool fillLocalPort = true);
    static QStringList arg2StringList(const SerialPortArgument& arg);
    static QStringList arg2StringList(const BTArgument& arg);
    static QStringList arg2StringList(const NetworkArgument& arg);
    static SerialPortArgument stringList2SPArg(const QStringList& list);
    static NetworkArgument stringList2NetArg(const QStringList& list);


    // IO
    QByteArray readAll();
    qint64 write(const char *data, qint64 len);
    qint64 write(const QByteArray &data);

    // SerialPort
    QSerialPort::PinoutSignals SP_pinoutSignals();
    bool SP_setDataTerminalReady(bool set);
    bool SP_isDataTerminalReady();
    bool SP_setRequestToSend(bool set);
    bool SP_isRequestToSend();
    bool SP_setBaudRate(qint32 baudRate);
    qint32 SP_baudRate();
    bool SP_setDataBits(QSerialPort::DataBits dataBits);
    bool SP_setStopBits(QSerialPort::StopBits stopBits);
    bool SP_setParity(QSerialPort::Parity parity);
    bool SP_setFlowControl(QSerialPort::FlowControl flowControl);
    void SP_setIgnoredErrorList(const QList<QSerialPort::SerialPortError>& errorList);
    QList<QSerialPort::SerialPortError> SP_getIgnoredErrorList();

    // Bluetooth
    QString BT_remoteName();
    QBluetoothAddress BT_localAddress();
    QList<QBluetoothSocket*> BTServer_clientList() const;
    int BTServer_clientCount();
    bool BTServer_setClientMode(QBluetoothSocket* clientSocket, bool RxEnabled = true, bool TxEnabled = true);

    // Network
    void UDP_setRemote(const QString& addr, quint16 port);
    QList<QTcpSocket*> TCPServer_clientList() const;
    int TCPServer_clientCount();
    bool TCPServer_setClientMode(QTcpSocket* clientSocket, bool RxEnabled = true, bool TxEnabled = true);
public slots:
    // general
    void setPolling(bool enabled);

    // connection
    void setArgument(Connection::SerialPortArgument arg);
    void setArgument(Connection::BTArgument arg);
    void setArgument(Connection::NetworkArgument arg);
    void open(); // async
    bool reopen(); // async, return false if no argument is stored in the previous connection
    void close(bool forced = false); // async

    // BLE
    void BLEC_onDataArrived(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
private:
    enum BLE_RxTxMode
    {
        BLE_1S1C = 0, // Tx and Rx use the same service and characteristic
        BLE_1S2C, // Tx and Rx use the same service but different characteristics
        BLE_2S2C, // Tx and Rx use different services
    };
    Type m_type = SerialPort;
    State m_state = Unconnected;

    // signal/slot connections
    QMetaObject::Connection m_lastReadyReadConn;
    QMetaObject::Connection m_lastOnErrorConn;
    QMetaObject::Connection m_lastOnConnectedConn;
    QMetaObject::Connection m_lastOnDisconnectedConn;

    // establish connetion and reconnect
    bool m_lastSPArgumentValid = false, m_lastBTArgumentValid = false, m_lastNetArgumentValid = false;
    SerialPortArgument m_lastSPArgument, m_currSPArgument;
    BTArgument m_lastBTArgument, m_currBTArgument;
    NetworkArgument m_lastNetArgument, m_currNetArgument;

    QSerialPort* m_serialPort = nullptr;
    QBluetoothServer* m_BTServer = nullptr;
    QBluetoothSocket* m_BTSocket = nullptr;
    QLowEnergyController* m_BLEController = nullptr;
    QLowEnergyService* m_BLERxTxService = nullptr; // Rx of all mode, Tx of BLE_1S1C and BLE 1S2C mode
    QLowEnergyService* m_BLETxService = nullptr; // Tx of BLE_2S2C mode
    bool m_BLERxCharacteristicValid = false;
    bool m_BLETxCharacteristicValid = false;
    QLowEnergyCharacteristic m_BLETxCharacteristic;
    QLowEnergyService::WriteMode m_BLETxWriteMode;
    QTcpServer* m_TCPServer = nullptr;
    QTcpSocket* m_TCPSocket = nullptr;
    QUdpSocket* m_UDPSocket = nullptr;

    QList<QBluetoothSocket*> m_BTConnectedClients;
    QList<QBluetoothSocket*> m_BTTxClients;
    QList<QBluetoothUuid> m_BLEDiscoveredServices;
    QList<QTcpSocket*> m_TCPConnectedClients;
    QList<QTcpSocket*> m_TCPTxClients;
    QBluetoothServiceInfo m_RfcommServiceInfo;
    BLE_RxTxMode m_BLERxTxMode;


    // for characteristics without notify property in BLE, pinout signals in serialport
    QTimer* m_pollTimer = nullptr;
    bool m_pollTimerEnabled = false;

    //
    QSerialPort::PinoutSignals m_SP_lastSignals;
    QList<QSerialPort::SerialPortError> m_SP_ignoredErrorList;

    QByteArray m_buf;

    bool m_isCollectingErrorString = false;
    QStringList m_errorStringList;
    void setCollectingErrorStringList(bool state);

    static const QMap<Connection::Type, QLatin1String> m_typeNameMap;

    void updateSignalSlot();
    void BTServer_initServiceInfo();
    void BTServer_updateServicePort();
    void changeState(State newState);
    void Server_onClientDisconnectedHandler(QObject *clientObj);
    void afterConnected();
signals:
    void readyRead();
    void connected();
    void disconnected();
    void connectFailed(const QString& info);
    void connectFailed(const QStringList& infoList);
    void errorOccurred();
    // the slot can accept newState only
    void stateChanged(Connection::State newState, Connection::State oldState);
    void SP_signalsChanged(QSerialPort::PinoutSignals signal);
    // for BT_Server
    void BT_clientConnected();
    void BT_clientDisconnected();
    // for TCP_Server
    void TCP_clientConnected();
    void TCP_clientDisconnected();
private slots:
    void onReadyRead();
    void onErrorOccurred();
    void onConnected();
    // onDisconnected() might be called more than once
    void onDisconnected();
    void Server_onClientConnected();
    // Server_onClientDisconnected() might be called more than once
    void Server_onClientDisconnected();
    void Server_onClientErrorOccurred();
    void onPollingTimeout();
    void blackhole();
    // BLE
    void BLEC_onServiceDiscovered(const QBluetoothUuid& serviceUUID);
    void BLEC_onServiceDetailDiscovered(QLowEnergyService::ServiceState newState);

};

Q_DECLARE_METATYPE(Connection::SerialPortArgument)
QDebug operator<<(QDebug dbg, const Connection::SerialPortArgument& arg);
QDebug operator<<(QDebug dbg, const Connection::BTArgument& arg);
QDebug operator<<(QDebug dbg, const Connection::NetworkArgument& arg);

#endif // CONNECTION_H
