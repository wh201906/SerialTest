#include "connection.h"

#include <QNetworkDatagram>

Connection::Connection(QObject *parent)
    : QObject{parent}
{
    // permanent
    m_pollTimer = new QTimer();
    m_serialPort = new QSerialPort();
    m_BTSocket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);
    m_BTServer = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol);
    m_TCPSocket = new QTcpSocket();
    m_TCPServer = new QTcpServer();
    m_UDPSocket = new QUdpSocket();

    BTServer_initServiceInfo();

    m_pollTimer->setInterval(100); // default interval
    connect(m_pollTimer, &QTimer::timeout, this, &Connection::onPollingTimeout);
}

Connection::Type Connection::type()
{
    return m_type;
}

bool Connection::setType(Type type)
{
    if(m_state != Unconnected)
        return false;
    m_type = type;
    m_lastSPArgumentValid = false;
    m_lastBTArgumentValid = false;
    m_lastNetArgumentValid = false;
    updateSignalSlot();
    return true;
}

bool Connection::isConnected()
{
    return m_state == Connected;
}

Connection::State Connection::state()
{
    return m_state;
}

void Connection::setPolling(bool enabled)
{
    m_pollTimerEnabled = enabled;
    if(!enabled)
        m_pollTimer->stop();
    else if(enabled && isConnected())
        m_pollTimer->start();
}

bool Connection::polling()
{
    return m_pollTimerEnabled;
}

void Connection::setPollingInterval(int msec)
{
    m_pollTimer->setInterval(msec);
}

int Connection::pollingInterval()
{
    return m_pollTimer->interval();
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
        else
            emit connectFailed();
    }
    else if(m_type == BT_Client)
    {
        changeState(Connecting);
        // TODO: add user defined UUID support
        m_BTSocket->connectToService(m_currBTArgument.deviceAddress, QBluetoothUuid::SerialPort);
    }
    else if(m_type == BT_Server)
    {
        if(!m_BTServer->listen(m_currBTArgument.localAdapterAddress))
        {
            emit connectFailed();
            return;
        }
        m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::ServiceName, m_currBTArgument.serverServiceName);
        BTServer_updateServicePort();
        if(!m_RfcommServiceInfo.registerService(m_currBTArgument.localAdapterAddress))
        {
            m_BTServer->close();
            emit connectFailed();
            return;
        }
        changeState(Bound);
        // onClientConnected() will be called when client is connected
        // onConnected() = changeState(Connected) + afterConnected()
        afterConnected();
    }
    else if(m_type == TCP_Client)
    {
        changeState(Connecting);
        // use Qt5.5 or higher to specify local address/port properly
        m_TCPSocket->bind(m_currNetArgument.localAddress, m_currNetArgument.localPort);
        // remoteName can also be a ipv4/ipv6 address
        m_TCPSocket->connectToHost(m_currNetArgument.remoteName, m_currNetArgument.remotePort);
    }
    else if(m_type == TCP_Server)
    {
        if(!m_TCPServer->listen(m_currNetArgument.localAddress, m_currNetArgument.localPort))
        {
            emit connectFailed();
            return;
        }
        changeState(Bound);
        // onClientConnected() will be called when client is connected
        // onConnected() = changeState(Connected) + afterConnected()
        afterConnected();
    }
    else if(m_type == UDP)
    {
        // support only one multicast address now...
        if(m_currNetArgument.localAddress.isMulticast())
        {
            if(!m_UDPSocket->bind(QHostAddress::Any, m_currNetArgument.localPort, QAbstractSocket::ShareAddress))
            {
                emit connectFailed();
                return;
            }
            // remember to leave the group in close() or disconnect()
            if(!m_UDPSocket->joinMulticastGroup(m_currNetArgument.localAddress))
            {
                emit connectFailed();
                m_UDPSocket->close(); // necessary? call abort()?
                return;
            }
            onConnected(); // no connection, bound = connected
        }
        else // for unicast and broadcast
            if(m_UDPSocket->bind(m_currNetArgument.localAddress, m_currNetArgument.localPort))
                onConnected(); // no connection, bound = connected
            else
                emit connectFailed();
    }
}

bool Connection::reopen()
{
    if(m_type == SerialPort)
    {
        if(!m_lastSPArgumentValid)
            return false;
        setArgument(m_lastSPArgument);
    }
    else if(m_type == BT_Client || m_type == BT_Server)
    {
        if(!m_lastBTArgumentValid)
            return false;
        setArgument(m_lastBTArgument);
    }
    else if(m_type == TCP_Client || m_type == TCP_Server || m_type == UDP)
    {
        if(!m_lastNetArgumentValid)
            return false;
        setArgument(m_lastNetArgument);
    }
    open();
    return true;
}

void Connection::close(bool forced)
{
    if(m_state == Unconnected && !forced)
        return;
    if(m_type == SerialPort)
    {
        m_serialPort->close();
    }
    else if(m_type == BT_Client)
    {
        m_BTSocket->disconnectFromService();
        m_BTSocket->close();
    }
    else if(m_type == BT_Server)
    {
        m_RfcommServiceInfo.unregisterService();
        m_BTServer->close();
        for(auto it = m_BTConnectedClients.begin(); it != m_BTConnectedClients.end(); ++it)
            (*it)->close();
        // the delete operation will be done in Server_onClientDisconnected()
    }
    else if(m_type == TCP_Client)
    {
        m_TCPSocket->close(); // will call disconnectFromHost()
    }
    else if(m_type == TCP_Server)
    {
        m_TCPServer->close();
        for(auto it = m_TCPConnectedClients.begin(); it != m_TCPConnectedClients.end(); ++it)
            (*it)->close();
        // the delete operation will be done in Server_onClientDisconnected()
    }
    else if(m_type == UDP)
    {
        m_UDPSocket->close();
    }
    onDisconnected();
}

void Connection::updateSignalSlot()
{
    disconnect(m_lastReadyReadConn);
    disconnect(m_lastOnErrorConn);
    disconnect(m_lastOnConnectedConn);
    disconnect(m_lastOnDisconnectedConn);
    if(m_type == SerialPort)
    {
        m_lastReadyReadConn = connect(m_serialPort, &QIODevice::readyRead, this, &Connection::onReadyRead);
        m_lastOnErrorConn = connect(m_serialPort, &QSerialPort::errorOccurred, this, &Connection::onErrorOccurred);
    }
    else if(m_type == BT_Client)
    {
        m_lastReadyReadConn = connect(m_BTSocket, &QIODevice::readyRead, this, &Connection::onReadyRead);
        m_lastOnErrorConn = connect(m_BTSocket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error), this, &Connection::onErrorOccurred);
        m_lastOnConnectedConn = connect(m_BTSocket, &QBluetoothSocket::connected, this, &Connection::onConnected);
        m_lastOnDisconnectedConn = connect(m_BTSocket, &QBluetoothSocket::disconnected, this, &Connection::onDisconnected);
    }
    else if(m_type == BT_Server)
    {
        // readyRead(), disconnected() is connected in onClientConnected()
        m_lastOnErrorConn = connect(m_BTServer, QOverload<QBluetoothServer::Error>::of(&QBluetoothServer::error), this, &Connection::onErrorOccurred);
        m_lastOnConnectedConn = connect(m_BTServer, &QBluetoothServer::newConnection, this, &Connection::Server_onClientConnected);
    }
    else if(m_type == TCP_Client)
    {
        m_lastReadyReadConn = connect(m_TCPSocket, &QIODevice::readyRead, this, &Connection::onReadyRead);
        m_lastOnErrorConn = connect(m_TCPSocket, &QAbstractSocket::errorOccurred, this, &Connection::onErrorOccurred);
        m_lastOnConnectedConn = connect(m_TCPSocket, &QAbstractSocket::connected, this, &Connection::onConnected);
        m_lastOnDisconnectedConn = connect(m_TCPSocket, &QAbstractSocket::disconnected, this, &Connection::onDisconnected);
    }
    else if(m_type == TCP_Server)
    {
        // readyRead(), disconnected() is connected in onClientConnected()
        m_lastOnErrorConn = connect(m_TCPServer, &QTcpServer::acceptError, this, &Connection::onErrorOccurred);
        m_lastOnConnectedConn = connect(m_TCPServer, &QTcpServer::newConnection, this, &Connection::Server_onClientConnected);
    }
    else if(m_type == UDP)
    {
        m_lastReadyReadConn = connect(m_UDPSocket, &QIODevice::readyRead, this, &Connection::onReadyRead);
        m_lastOnErrorConn = connect(m_UDPSocket, &QAbstractSocket::errorOccurred, this, &Connection::onErrorOccurred);
        m_lastOnConnectedConn = connect(m_UDPSocket, &QAbstractSocket::connected, this, &Connection::onConnected);
        m_lastOnDisconnectedConn = connect(m_UDPSocket, &QAbstractSocket::disconnected, this, &Connection::onDisconnected);
    }
}

void Connection::BTServer_initServiceInfo()
{
    // call it once
    QBluetoothServiceInfo::Sequence profileSequence;
    QBluetoothServiceInfo::Sequence classId;
    classId << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::SerialPort));
    classId << QVariant::fromValue(quint16(0x100));
    profileSequence.append(QVariant::fromValue(classId));
    m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::BluetoothProfileDescriptorList, profileSequence);

    classId.clear();
    // Add user defined UUID there
    // classId << QVariant::fromValue(QBluetoothUuid(serialServiceUuid));
    classId << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::SerialPort));

    m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::ServiceClassIds, classId);

    //! [Service name, description and provider]
    // m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::ServiceName, tr("Bt Chat Server"));
    m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::ServiceDescription, tr("Bluetooth SPP Service"));
    m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::ServiceProvider, "wh201906");
    //! [Service name, description and provider]

    //! [Service UUID set]
    // use QBluetoothUuid::SerialPort there
    // m_RfcommServiceInfo.setServiceUuid(QBluetoothUuid(serialServiceUuid));
    m_RfcommServiceInfo.setServiceUuid(QBluetoothUuid::SerialPort);
    //! [Service UUID set]

    //! [Service Discoverability]
    QBluetoothServiceInfo::Sequence publicBrowse;
    publicBrowse << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::PublicBrowseGroup));
    m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::BrowseGroupList,
                                     publicBrowse);
    //! [Service Discoverability]
}

void Connection::BTServer_updateServicePort()
{
    // call it after every m_BTServer->listen()
    //! [Protocol descriptor list]
    QBluetoothServiceInfo::Sequence protocolDescriptorList;
    QBluetoothServiceInfo::Sequence protocol;
    protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::L2cap));
    protocolDescriptorList.append(QVariant::fromValue(protocol));
    protocol.clear();
    protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::Rfcomm))
             << QVariant::fromValue(quint8(m_BTServer->serverPort()));
    protocolDescriptorList.append(QVariant::fromValue(protocol));
    m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::ProtocolDescriptorList,
                                     protocolDescriptorList);
    //! [Protocol descriptor list]
}

void Connection::changeState(State newState)
{
    State oldState = m_state;
    m_state = newState;
    if(newState != oldState)
        emit stateChanged(newState, oldState);
}

void Connection::onReadyRead()
{
    if(m_type == SerialPort)
    {
        m_buf += m_serialPort->readAll();
    }
    else if(m_type == BT_Client)
    {
        m_buf += m_BTSocket->readAll();
    }
    else if(m_type == BT_Server)
    {
        m_buf += qobject_cast<QBluetoothSocket*>(sender())->readAll();
    }
    else if(m_type == TCP_Client)
    {
        m_buf += m_TCPSocket->readAll();
    }
    else if(m_type == TCP_Server)
    {
        m_buf += qobject_cast<QTcpSocket*>(sender())->readAll();
    }
    else if(m_type == UDP)
    {
        // readyRead() will not be emitted unless all pending datagrams are handled
        // this should be handled as soon as possible
        while(m_UDPSocket->hasPendingDatagrams())
            m_buf += m_UDPSocket->receiveDatagram().data();
    }
    emit readyRead();
}

void Connection::onErrorOccurred()
{
    qDebug() << "Connection::onErrorOccurred()";
    if(m_type == SerialPort)
    {
        // connectFailed() is emitted in open()
        QSerialPort::SerialPortError error;
        error = m_serialPort->error();
        qDebug() << "SerialPort Error:" << error;

        // no error
        if(error == QSerialPort::NoError)
            ;
        // serialport still works
        else if(error == QSerialPort::FramingError || error == QSerialPort::ParityError || error == QSerialPort::BreakConditionError || error == QSerialPort::UnsupportedOperationError || error == QSerialPort::TimeoutError || error == QSerialPort::ReadError || error == QSerialPort::WriteError)
            ;
        // doesn't work, but don't close it
        else if(error == QSerialPort::NotOpenError)
            ;
        // serialport doesn't work, close it for reconnection
        else
        {
            close(true);
        }
    }
    else if(m_type == BT_Client)
    {
        QBluetoothSocket::SocketError error;
        error = m_BTSocket->error();
        qDebug() << "BT Socket Error:" << error;

        // no error
        if(error == QBluetoothSocket::NoSocketError)
            ;
        // keep the statement in Server_onClientErrorOccurred() in same
        else if(error == QBluetoothSocket::NetworkError || error == QBluetoothSocket::OperationError)
            ;
        else
        {
            if(m_state == Connecting)
                emit connectFailed();
            close(true);
        }
    }
    else if(m_type == TCP_Client)
    {
        QAbstractSocket::SocketError error;
        error = m_TCPSocket->error();
        qDebug() << "TCP Socket Error:" << error;


        // QTcpSocket::SocketTimeoutError? QTcpSocket::DatagramTooLargeError?
        // keep the statement in Server_onClientErrorOccurred() in same
        if(error == QTcpSocket::OperationError || error == QTcpSocket::TemporaryError || error == QTcpSocket::UnsupportedSocketOperationError)
            ;
        else
            close(); // this will emit disconnected()
    }
    // untested yet
    // for server, the m_state need to be changed there
    if(m_type == BT_Server)
    {
        QBluetoothServer::Error error;
        error = m_BTServer->error();
        qDebug() << "BT Server Error:" << error;
        if(!m_BTServer->isListening())
        {
            // the server is always listening when the server is running, according to my implementation
            onDisconnected();
        }
    }
    else if(m_type == TCP_Server || m_type == UDP)
    {
        QAbstractSocket::SocketError error;
        if(m_type == TCP_Server)
        {
            error = m_TCPServer->serverError();
            if(!m_TCPServer->isListening())
            {
                // the server is always listening when the server is running, according to my implementation
                onDisconnected();
            }
        }
        else // UDP
            error = m_UDPSocket->error();
        qDebug() << "Net Error:" << error;
    }
    emit errorOccurred();
}

QByteArray Connection::readAll()
{
    QByteArray result(m_buf);
    m_buf.clear();
    return result;
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
    else if(m_type == BT_Server)
    {
        quint64 maxLen = 0, currLen;
        // write to all connected clients
        for(auto it = m_BTConnectedClients.cbegin(); it != m_BTConnectedClients.cend(); ++it)
        {
            currLen = (*it)->write(data, len);
            if(maxLen > currLen)
                maxLen = currLen;
        }
        return maxLen;
    }
    else if(m_type == TCP_Client)
    {
        return m_TCPSocket->write(data, len);
    }
    else if(m_type == TCP_Server)
    {
        quint64 maxLen = 0, currLen;
        // write to all connected clients
        for(auto it = m_TCPConnectedClients.cbegin(); it != m_TCPConnectedClients.cend(); ++it)
        {
            currLen = (*it)->write(data, len);
            if(maxLen > currLen)
                maxLen = currLen;
        }
        return maxLen;
    }
    else if(m_type == UDP)
    {
        return m_UDPSocket->writeDatagram(data, len, QHostAddress(m_currNetArgument.remoteName), m_currNetArgument.remotePort);
    }
    return 0;
}

qint64 Connection::write(const QByteArray &data)
{
    return write(data.constData(), data.size());
}

void Connection::onConnected()
{
    qDebug() << "Connection::onConnected()";
    changeState(Connected);
    afterConnected();
}

void Connection::afterConnected()
{
    if(m_type == SerialPort)
    {
        m_lastSPArgument = m_currSPArgument;
        m_lastSPArgumentValid = true;
    }
    else if(m_type == BT_Client || m_type == BT_Server)
    {
        m_lastBTArgument = m_currBTArgument;
        m_lastBTArgumentValid = true;
    }
    else if(m_type == TCP_Client || m_type == TCP_Server || m_type == UDP)
    {
        m_lastNetArgument = m_currNetArgument;
        m_lastNetArgumentValid = true;
    }
    if(m_pollTimerEnabled)
        m_pollTimer->start();
    emit connected();
}

void Connection::onDisconnected()
{
    State oldState = m_state;
    qDebug() << "Connection::onDisconnected()";
    m_pollTimer->stop();
    changeState(Unconnected);
    if(oldState != Unconnected)
        emit disconnected();
}

void Connection::Server_onClientConnected()
{
    qDebug() << "Connection::Server_onClientConnected()";
    if(m_type == BT_Server)
    {
        QBluetoothSocket *socket = m_BTServer->nextPendingConnection();
        if(!socket)
            return;

        changeState(Connected);
        connect(socket, &QBluetoothSocket::readyRead, this, &Connection::onReadyRead);
        connect(socket, &QBluetoothSocket::disconnected, this, &Connection::Server_onClientDisconnected);
        connect(socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error), this, &Connection::Server_onClientErrorOccurred);
        m_BTConnectedClients.append(socket);
        emit BT_clientConnected();
    }
    else if(m_type == TCP_Server)
    {
        QTcpSocket* socket = m_TCPServer->nextPendingConnection();
        if(!socket)
            return;

        changeState(Connected);
        connect(socket, &QTcpSocket::readyRead, this, &Connection::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &Connection::Server_onClientDisconnected);
        connect(socket, &QAbstractSocket::errorOccurred, this, &Connection::onErrorOccurred);
        m_TCPConnectedClients.append(socket);
        emit TCP_clientConnected();
    }
}

// this will be called by cliendDisconnected() and clientErrorOccurred()
void Connection::Server_onClientDisconnectedHandler(QObject* clientObj)
{
    // send clientDisconnceted() only once
    bool firstCall = false;
    qDebug() << "Connection::Server_onClientDisconnected()";
    if(m_type == BT_Server)
    {
        QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(clientObj);
        if(!socket)
            return;

        firstCall = m_BTConnectedClients.removeOne(socket);
        if(m_BTConnectedClients.empty())
        {
            if(m_BTServer->isListening())
                changeState(Bound);
            else
                changeState(Unconnected);
        }
        socket->deleteLater();
        if(firstCall)
            emit BT_clientDisconnected();
    }
    else if(m_type == TCP_Server)
    {
        QTcpSocket *socket = qobject_cast<QTcpSocket *>(clientObj);
        if(!socket)
            return;

        firstCall = m_TCPConnectedClients.removeOne(socket);
        if(m_TCPConnectedClients.empty())
        {
            if(m_TCPServer->isListening())
                changeState(Bound);
            else
                changeState(Unconnected);
        }
        socket->deleteLater();
        if(firstCall)
            emit TCP_clientDisconnected();
    }
}

void Connection::Server_onClientDisconnected()
{
    Server_onClientDisconnectedHandler(sender());
}

void Connection::Server_onClientErrorOccurred()
{
    qDebug() << "Connection::Server_onClientErrorOccurred()";
    if(m_type == BT_Server)
    {
        QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(sender());
        QBluetoothSocket::SocketError socketError;
        socketError = socket->error();
        qDebug() << "BT Socket Error:" << socketError;

        // no error
        if(socketError == QBluetoothSocket::NoSocketError)
            ;
        else if(socketError == QBluetoothSocket::NetworkError || socketError == QBluetoothSocket::OperationError)
            ;
        else
        {
            socket->disconnectFromService(); // this will emit disconnected()
            socket->close();
            Server_onClientDisconnectedHandler(sender());
        }
    }
    else if(m_type == TCP_Server)
    {
        QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
        QTcpSocket::SocketError socketError;
        socketError = socket->error();
        qDebug() << "TCP Socket Error:" << socketError;

        if(socketError == QTcpSocket::OperationError || socketError == QTcpSocket::TemporaryError || socketError == QTcpSocket::UnsupportedSocketOperationError)
            ;
        else
        {
            socket->close(); // close will call disconnectFromHost()
            Server_onClientDisconnectedHandler(sender());
        }
    }
}

void Connection::onPollingTimeout()
{
    if(m_type == SerialPort)
    {
        QSerialPort::PinoutSignals newSignal;
        newSignal = SP_pinoutSignals();
        if(newSignal != m_SP_lastSignals)
            emit SP_signalsChanged(newSignal);
        m_SP_lastSignals = newSignal;
    }
}

QSerialPort::PinoutSignals Connection::SP_pinoutSignals()
{
    return m_serialPort->pinoutSignals();
}

bool Connection::SP_setDataTerminalReady(bool set)
{
    if(m_type != SerialPort)
        return false;
    return m_serialPort->setDataTerminalReady(set);
}

bool Connection::SP_isDataTerminalReady()
{
    return m_serialPort->isDataTerminalReady();
}

bool Connection::SP_setRequestToSend(bool set)
{
    if(m_type != SerialPort)
        return false;
    return m_serialPort->setRequestToSend(set);
}

bool Connection::SP_isRequestToSend()
{
    return m_serialPort->isRequestToSend();
}

bool Connection::SP_setBaudRate(qint32 baudRate)
{
    if(m_type != SerialPort)
        return false;
    if(!m_serialPort->setBaudRate(baudRate))
        return false;
    m_currSPArgument.baudRate = baudRate;
    return true;
}

qint32 Connection::SP_baudRate()
{
    return m_serialPort->baudRate();
}

bool Connection::SP_setDataBits(QSerialPort::DataBits dataBits)
{
    if(m_type != SerialPort)
        return false;
    if(!m_serialPort->setDataBits(dataBits))
        return false;
    m_currSPArgument.dataBits = dataBits;
    return true;
}

bool Connection::SP_setStopBits(QSerialPort::StopBits stopBits)
{
    if(m_type != SerialPort)
        return false;
    if(!m_serialPort->setStopBits(stopBits))
        return false;
    m_currSPArgument.stopBits = stopBits;
    return true;
}

bool Connection::SP_setParity(QSerialPort::Parity parity)
{
    if(m_type != SerialPort)
        return false;
    if(!m_serialPort->setParity(parity))
        return false;
    m_currSPArgument.parity = parity;
    return true;
}

bool Connection::SP_setFlowControl(QSerialPort::FlowControl flowControl)
{
    if(m_type != SerialPort)
        return false;
    if(!m_serialPort->setFlowControl(flowControl))
        return false;
    m_currSPArgument.flowControl = flowControl;
    return true;
}

QString Connection::BTClient_remoteName()
{
    if(m_type == BT_Client && m_BTSocket != nullptr)
        return m_BTSocket->peerName();

    return QString();
}

QBluetoothAddress Connection::BT_localAddress()
{
    if(m_type == BT_Client && m_BTSocket != nullptr)
        return m_BTSocket->localAddress();
    else if(m_type == BT_Server && m_BTServer != nullptr)
        return m_BTServer->serverAddress();
    return QBluetoothAddress();
}

QList<QBluetoothSocket *> Connection::BTServer_clientList() const
{
    return m_BTConnectedClients;
}

int Connection::BTServer_clientCount()
{
    return m_BTConnectedClients.count();
}

void Connection::UDP_setRemote(const QString& addr, quint16 port)
{
    m_currNetArgument.remoteName = addr;
    m_currNetArgument.remotePort = port;
}

QList<QTcpSocket *> Connection::TCPServer_clientList() const
{
    return m_TCPConnectedClients;
}

int Connection::TCPServer_clientCount()
{
    return m_TCPConnectedClients.count();
}
