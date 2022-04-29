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
        quint32 baudRate;
        QSerialPort::DataBits dataBits;
        QSerialPort::StopBits stopBits;
        QSerialPort::Parity parity;
        QSerialPort::FlowControl flowControl;
    };

    struct BTArgument
    {
        // for Bluetooth classic and BLE
        QBluetoothAddress deviceAddress;
        // for Bluetooth classic server
        QString serverServiceName;
        QBluetoothAddress localAdapterAddress;
        // for BLE only
        QBluetoothUuid RxServiceUUID, RxCharacteristicUUID;
        QBluetoothUuid TxServiceUUID, TxCharacteristicUUID;
    };

    struct NetworkArgument
    {
        QHostAddress localAddress;
        // connectToHost() supports hostName and hostAddress, writeDatagram() only supports hostAddress
        // use QString to store all hostName/hostAddress
        QString remoteName;
        quint16 remotePort, localPort;
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

    // connection
    SerialPortArgument getSerialPortArgument();
    BTArgument getBTArgument();
    NetworkArgument getNetworkArgument();


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

    // Bluetooth
    QString BTClient_remoteName();
    QBluetoothAddress BT_localAddress();

    void UDP_setRemote(const QString& addr, quint16 port);
public slots:
    // general
    void setPolling(bool enabled);

    // connection
    void setArgument(SerialPortArgument arg);
    void setArgument(BTArgument arg);
    void setArgument(NetworkArgument arg);
    void open(); // async
    bool reopen(); // async, return false if no argument is stored in the previous connection
    void close(bool forced = false); // async
private:
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
    QTcpServer* m_TCPServer = nullptr;
    QTcpSocket* m_TCPSocket = nullptr;
    QUdpSocket* m_UDPSocket = nullptr;

    QList<QBluetoothSocket*> m_BTConnectedClients;
    QList<QTcpSocket*> m_TCPConnectedClients;
    QBluetoothServiceInfo m_RfcommServiceInfo;


    // for characteristics without notify property in BLE, pinout signals in serialport
    QTimer* m_pollTimer = nullptr;
    bool m_pollTimerEnabled = false;

    //
    QSerialPort::PinoutSignals m_SP_lastSignals;

    QByteArray m_buf;

    void updateSignalSlot();
    void BTServer_initServiceInfo();
    void BTServer_updateServicePort();
    void changeState(State newState);
    void Server_onClientDisconnectedHandler(QObject *clientObj);
signals:
    void readyRead();
    void connected();
    void disconnected();
    void connectFailed();
    void errorOccurred();
    // the slot can accept newState only
    void stateChanged(State newState, State oldState);
    void SP_signalsChanged(QSerialPort::PinoutSignals signal);
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

};

#endif // CONNECTION_H
