#include "connection.h"

#include <QNetworkDatagram>

Connection::Connection(QObject *parent)
    : QObject{parent}
{
    m_pollTimer = new QTimer();
    m_serialPort = new QSerialPort();
    m_BTServer = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol);
    m_TCPSocket = new QTcpSocket();
    m_UDPSocket = new QUdpSocket();
}

bool Connection::setType(Type type)
{
    m_type = type;
    m_lastSPArgumentValid = false;
    m_lastBTArgumentValid = false;
    m_lastNetworkArgumentValid = false;
    updateSignalSlot();
    return true;
}

bool Connection::connected()
{
    return m_connected;
}

void Connection::setArgument(SerialPortArgument arg)
{
    m_currSPArgument = arg;
}

void Connection::setArgument(BTArgument arg)
{
    m_currBTArgument = arg;
}

void Connection::setArgument(NetworkArgument arg)
{
    m_currNetArgument = arg;
}

Connection::SerialPortArgument Connection::getSerialPortArgument()
{
    return m_currSPArgument;
}

Connection::BTArgument Connection::getBTArgument()
{
    return m_currBTArgument;
}

Connection::NetworkArgument Connection::getNetworkArgument()
{
    return m_currNetArgument;
}

void Connection::open()
{
    // close before open?
    if(m_type == SerialPort)
    {
        m_serialPort->setPortName(m_currSPArgument.name);
        m_serialPort->setBaudRate(m_currSPArgument.baudRate);
        m_serialPort->setDataBits(m_currSPArgument.dataBits);
        m_serialPort->setStopBits(m_currSPArgument.stopBits);
        m_serialPort->setParity(m_currSPArgument.parity);
        m_serialPort->setFlowControl(m_currSPArgument.flowControl);

        // serialport doesn't have connected() signal(open() is sync function), so call onConnected() manually
        if(m_serialPort->open(QIODevice::ReadWrite))
            onConnected();
    }
    else if(m_type == BT_Client)
    {
        m_BTSocket->connectToService(m_currBTArgument.deviceAddress, QBluetoothUuid::SerialPort);
    }
    else if(m_type == TCP_Client)
    {
        if(m_currNetArgument.useRemoteName)
            m_TCPSocket->connectToHost(m_currNetArgument.remotetName, m_currNetArgument.remotePort);
        else
            m_TCPSocket->connectToHost(m_currNetArgument.remoteAddress, m_currNetArgument.remotePort);
    }
    else if(m_type == TCP_Server)
    {
        m_TCPSocket->bind(m_currNetArgument.localAddress, m_currNetArgument.localPort);
    }
    else if(m_type == UDP)
    {
        // support only one multicast address now...
        if(m_currNetArgument.localAddress.isMulticast())
        {
            do
            {
                if(!m_UDPSocket->bind(QHostAddress::Any, m_currNetArgument.localPort, QAbstractSocket::ShareAddress))
                    break;
                // remember to leave the group in close() or disconnect()
                if(!m_UDPSocket->joinMulticastGroup(m_currNetArgument.localAddress))
                {
                    m_UDPSocket->disconnectFromHost(); // necessary? call abort()?
                    break;
                }
                onConnected(); // necessary?
            }
            while(false); // I just want to use break
        }
        else // for unicast and broadcast
            if(m_UDPSocket->bind(m_currNetArgument.localAddress, m_currNetArgument.localPort))
                onConnected(); // necessary?
    }
}

void Connection::updateSignalSlot()
{
    disconnect(m_lastReadyReadConn);
    disconnect(m_lastOnErrorConn);
    disconnect(m_lastOnConnectedConn);
    disconnect(m_lastOnDisconnectedConn);
    if(m_type == SerialPort)
    {
        m_lastReadyReadConn = connect(m_serialPort, &QIODevice::readyRead, this, &Connection::readyRead);
        m_lastOnErrorConn = connect(m_serialPort, &QSerialPort::errorOccurred, this, &Connection::onErrorOccurred);
    }
    else if(m_type == BT_Client)
    {
        m_lastReadyReadConn = connect(m_BTSocket, &QIODevice::readyRead, this, &Connection::readyRead);
        m_lastOnErrorConn = connect(m_BTSocket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error), this, &Connection::onErrorOccurred);
        m_lastOnConnectedConn = connect(m_BTSocket, &QBluetoothSocket::connected, this, &Connection::onConnected);
        m_lastOnDisconnectedConn = connect(m_BTSocket, &QBluetoothSocket::disconnected, this, &Connection::onDisconnected);
    }
    else if(m_type == TCP_Client || m_type == TCP_Server)
    {
        m_lastReadyReadConn = connect(m_TCPSocket, &QIODevice::readyRead, this, &Connection::readyRead);
        m_lastOnErrorConn = connect(m_TCPSocket, &QAbstractSocket::errorOccurred, this, &Connection::onErrorOccurred);
        m_lastOnConnectedConn = connect(m_TCPSocket, &QAbstractSocket::connected, this, &Connection::onConnected);
        m_lastOnDisconnectedConn = connect(m_TCPSocket, &QAbstractSocket::disconnected, this, &Connection::onDisconnected);
    }
    else if(m_type == UDP)
    {
        m_lastReadyReadConn = connect(m_UDPSocket, &QIODevice::readyRead, this, &Connection::readyRead);
        m_lastOnErrorConn = connect(m_UDPSocket, &QAbstractSocket::errorOccurred, this, &Connection::onErrorOccurred);
        m_lastOnConnectedConn = connect(m_UDPSocket, &QAbstractSocket::connected, this, &Connection::onConnected);
        m_lastOnDisconnectedConn = connect(m_UDPSocket, &QAbstractSocket::disconnected, this, &Connection::onDisconnected);
    }
}

void Connection::onErrorOccurred()
{
    if(m_type == SerialPort)
    {
        QSerialPort::SerialPortError error;
        error = m_serialPort->error();
        // onDisconnected(); // ?
    }
    else if(m_type == BT_Client)
    {
        QBluetoothSocket::SocketError socketError;
        socketError = m_BTSocket->error();
    }
}

QByteArray Connection::readAll()
{
    if(m_type == SerialPort)
    {
        return m_serialPort->readAll();
    }
    else if(m_type == BT_Client)
    {
        return m_BTSocket->readAll();
    }
    else if(m_type == UDP)
    {
        return m_UDPSocket->receiveDatagram().data();
    }
    return QByteArray();
}

qint64 Connection::write(const char *data, qint64 len)
{
    if(m_type == SerialPort)
    {
        return m_serialPort->write(data, len);
    }
    else if(m_type == BT_Client)
    {
        return m_BTSocket->write(data, len);
    }
    else if(m_type == UDP)
    {
        return m_UDPSocket->writeDatagram(data, len, m_currNetArgument.remoteAddress, m_currNetArgument.remotePort);
    }
    return 0;
}

qint64 Connection::write(const QByteArray &data)
{
    return write(data.constData(), data.size());
}

void Connection::onConnected()
{
    m_connected = true;
    if(m_type == SerialPort)
    {
        m_lastSPArgument = m_currSPArgument;
        m_lastSPArgumentValid = true;
    }
    else if(m_type == BT_Client)
    {
        m_lastBTArgument = m_currBTArgument;
        m_lastBTArgumentValid = true;
    }
}

void Connection::onDisconnected()
{
    m_connected = false;
}
