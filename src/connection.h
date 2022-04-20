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
        // for BLE only
        QBluetoothUuid RxServiceUUID, RxCharacteristicUUID;
        QBluetoothUuid TxServiceUUID, TxCharacteristicUUID;
    };

    struct NetworkArgument
    {
        QHostAddress remoteAddress, localAddress;
        bool useRemoteName = false; // false: connect to remoteAddress rather than remoteName(for client)
        QString remotetName;
        quint16 remotePort, localPort;
    };

    explicit Connection(QObject *parent = nullptr);

    // general
    bool setType(Type type);
    bool connected();

    // connection
    void setArgument(SerialPortArgument arg);
    void setArgument(BTArgument arg);
    void setArgument(NetworkArgument arg);
    SerialPortArgument getSerialPortArgument();
    BTArgument getBTArgument();
    NetworkArgument getNetworkArgument();
    void open(); // async

    // IO
    QByteArray readAll();
    qint64 write(const char *data, qint64 len);
    qint64 write(const QByteArray &data);
private:
    Type m_type = SerialPort;
    bool m_connected = false;

    // signal/slot connections
    QMetaObject::Connection m_lastReadyReadConn;
    QMetaObject::Connection m_lastOnErrorConn;
    QMetaObject::Connection m_lastOnConnectedConn;
    QMetaObject::Connection m_lastOnDisconnectedConn;

    // establish connetion and reconnect
    bool m_lastSPArgumentValid = false, m_lastBTArgumentValid = false, m_lastNetworkArgumentValid = false;
    SerialPortArgument m_lastSPArgument, m_currSPArgument;
    BTArgument m_lastBTArgument, m_currBTArgument;
    NetworkArgument m_lastNetArgument, m_currNetArgument;

    QSerialPort* m_serialPort = nullptr;
    QBluetoothServer* m_BTServer = nullptr;
    QBluetoothSocket* m_BTSocket = nullptr;
    QTcpSocket* m_TCPSocket = nullptr;
    QUdpSocket* m_UDPSocket = nullptr;

    // for characteristics without notify property in BLE, pinout signals in serialport
    QTimer* m_pollTimer = nullptr;

    void updateSignalSlot();
signals:
    void readyRead();
private slots:
    void onErrorOccurred();
    void onConnected();
    void onDisconnected();
};

#endif // CONNECTION_H
