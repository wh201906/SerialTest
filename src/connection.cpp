#include "connection.h"

#include <QNetworkDatagram>
#include <QMetaEnum>

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

QString Connection::getTypeName(Type type)
{
    return tr(m_typeNameMap[type].data());
}

const QMap<Connection::Type, QLatin1String>& Connection::getTypeNameMap()
{
    return m_typeNameMap;
}

QStringList Connection::getErrorStringList() const
{
    return m_errorStringList;
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

Connection::NetworkArgument Connection::getNetworkArgument(bool fillLocalAddress, bool fillLocalPort)
{
    // the NetworkArgument passed to Connection might have auto-filled arguments
    // (localAddress == Any or localPort == 0)
    // After connected, the actural argument can be fetched

    if(!fillLocalAddress && !fillLocalPort)
        return m_currNetArgument;
    Connection::NetworkArgument arg = m_currNetArgument;

    if(fillLocalAddress && arg.localAddress == QHostAddress::Any) // local address is not specified
    {
        if(m_type == TCP_Client)
            arg.localAddress = m_TCPSocket->localAddress();
        else if(m_type == TCP_Server)
            arg.localAddress = m_TCPServer->serverAddress();
        else if(m_type == UDP)
            arg.localAddress = m_UDPSocket->localAddress();
    }
    if(fillLocalPort && arg.localPort == 0) // a random port is used
    {
        if(m_type == TCP_Client)
            arg.localPort = m_TCPSocket->localPort();
        else if(m_type == TCP_Server)
            arg.localPort = m_TCPServer->serverPort();
        else if(m_type == UDP)
            arg.localPort = m_UDPSocket->localPort();
    }
    return arg;
}

// Connection::SerialPortArgument Connection::stringList2SPArg(const QStringList& list)
QStringList Connection::arg2StringList(const SerialPortArgument& arg)
{
    QStringList argList
    {
        arg.name,
        QString::number(arg.baudRate),
        QString::number(arg.dataBits),
        QString::number(arg.stopBits),
        QString::number(arg.parity),
        QString::number(arg.flowControl),
    };
    if(arg.name != arg.id)
        argList += arg.id;
    return argList;
}

QStringList Connection::arg2StringList(const NetworkArgument &arg)
{
    QStringList argList
    {
        (arg.localAddress == QHostAddress::Any) ? "(Any)" : arg.localAddress.toString(),
        QString::number(arg.localPort),
        arg.remoteName,
        QString::number(arg.remotePort),
    };
    if(!arg.alias.isEmpty())
        argList += arg.alias;
    return argList;
}

// QStringList Connection::arg2StringList(const SerialPortArgument& arg)
Connection::SerialPortArgument Connection::stringList2SPArg(const QStringList& list)
{
    Connection::SerialPortArgument arg;
    if(!list.isEmpty())
    {
        int tmp;
        bool ok;
        arg.name = list[0];
        if(list.size() < 7)
            arg.id = arg.name;
        switch(list.size())
        {
        case 7:
            arg.id = list[6];
        case 6:
            if((tmp = list[5].toInt(&ok)) || ok)
                arg.flowControl = (QSerialPort::FlowControl)tmp;
        case 5:
            if((tmp = list[4].toInt(&ok)) || ok)
                arg.parity = (QSerialPort::Parity)tmp;
        case 4:
            if((tmp = list[3].toInt(&ok)) || ok)
                arg.stopBits = (QSerialPort::StopBits)tmp;
        case 3:
            if((tmp = list[2].toInt(&ok)) || ok)
                arg.dataBits = (QSerialPort::DataBits)tmp;
        case 2:
            if((tmp = list[1].toInt(&ok)) || ok)
                arg.baudRate = tmp;
        }
    }
    return arg;
}

Connection::NetworkArgument Connection::stringList2NetArg(const QStringList &list)
{
    Connection::NetworkArgument arg;
    if(list.size() >= 4)
    {
        QString name;
        name = list[0];
        if(name == "(Any)")
            arg.localAddress = QHostAddress::Any;
        else
            arg.localAddress = QHostAddress(name);
        arg.localPort = list[1].toUShort();
        arg.remoteName = list[2];
        arg.remotePort = list[3].toUShort();
        if(list.size() >= 5)
            arg.alias = list[4];
    }
    return arg;
}

void Connection::open()
{
    setCollectingErrorStringList(true);
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
            emit connectFailed(getErrorStringList());
    }
    else if(m_type == BT_Client)
    {
        changeState(Connecting);
        m_BTSocket->connectToService(m_currBTArgument.deviceAddress, m_currBTArgument.RxServiceUUID);
    }
    else if(m_type == BT_Server)
    {
        if(!m_BTServer->listen(m_currBTArgument.localAdapterAddress))
        {
            emit connectFailed(tr("Failed to listen on adapter") + "\n" + m_currBTArgument.localAdapterAddress.toString());
            return;
        }
        m_RfcommServiceInfo.setAttribute(QBluetoothServiceInfo::ServiceName, m_currBTArgument.serverServiceName);
        BTServer_updateServicePort();
        if(!m_RfcommServiceInfo.registerService(m_currBTArgument.localAdapterAddress))
        {
            m_BTServer->close();
            emit connectFailed(tr("Failed to register service on adapter") + "\n" + m_currBTArgument.localAdapterAddress.toString());
            return;
        }
        changeState(Bound);
        // onClientConnected() will be called when client is connected
        // onConnected() = changeState(Connected) + afterConnected()
        afterConnected();
    }
    else if(m_type == BLE_Central)
    {
        changeState(Connecting);
        if(m_currBTArgument.RxServiceUUID != m_currBTArgument.TxServiceUUID)
            m_BLERxTxMode = BLE_2S2C;
        else
            m_BLERxTxMode = m_currBTArgument.RxCharacteristicUUID == m_currBTArgument.TxCharacteristicUUID ? BLE_1S1C : BLE_1S2C;
        if(m_BLEController != nullptr)
            m_BLEController->deleteLater();
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        m_BLEController = new QLowEnergyController(m_currBTArgument.deviceAddress, m_currBTArgument.localAdapterAddress);
#else
        m_BLEController = QLowEnergyController::createCentral(m_currBTArgument.deviceAddress, m_currBTArgument.localAdapterAddress);
#endif
        connect(m_BLEController, &QLowEnergyController::connected, m_BLEController, &QLowEnergyController::discoverServices);
        connect(m_BLEController, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &Connection::onErrorOccurred);
        connect(m_BLEController, &QLowEnergyController::serviceDiscovered, this, &Connection::BLEC_onServiceDiscovered);
        m_BLEDiscoveredServices.clear();
        m_BLEController->connectToDevice();
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
            emit connectFailed(tr("Failed to listen to ") + "\n"
                               + QString("(%1, %2)")
                               .arg(m_currNetArgument.localAddress.toString())
                               .arg(m_currNetArgument.localPort));
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
                emit connectFailed(tr("(Multicast)Failed to listen to port ") + QString::number(m_currNetArgument.localPort));
                return;
            }
            // remember to leave the group in close() or disconnect()
            if(!m_UDPSocket->joinMulticastGroup(m_currNetArgument.localAddress))
            {
                emit connectFailed(tr("(Multicast)Failed to join ") + "\n" + m_currNetArgument.localAddress.toString());
                m_UDPSocket->close(); // necessary? call abort()?
                return;
            }
            onConnected(); // no connection, bound = connected
        }
        else // for unicast and broadcast
            if(m_UDPSocket->bind(m_currNetArgument.localAddress, m_currNetArgument.localPort))
                onConnected(); // no connection, bound = connected
            else
                emit connectFailed(QString("(%1, %2)")
                                   .arg(m_currNetArgument.localAddress.toString())
                                   .arg(m_currNetArgument.localPort));
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
    else if(m_type == BT_Client || m_type == BT_Server || m_type == BLE_Central)
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
        m_BTTxClients.clear();
        for(auto it = m_BTConnectedClients.begin(); it != m_BTConnectedClients.end(); ++it)
            (*it)->close();
        // the delete operation will be done in Server_onClientDisconnected()
    }
    else if(m_type == BLE_Central)
    {
        m_BLERxCharacteristicValid = false;
        m_BLETxCharacteristicValid = false;
        if(m_BLERxTxService != nullptr)
        {
            QLowEnergyDescriptor desc = m_BLERxTxService->characteristic(m_currBTArgument.RxCharacteristicUUID).descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
            m_BLERxTxService->writeDescriptor(desc, QByteArray::fromHex("0000"));
            m_BLERxTxService->deleteLater();
            m_BLERxTxService = nullptr;
        }
        if(m_BLETxService != nullptr)
        {
            m_BLETxService->deleteLater();
            m_BLETxService = nullptr;
        }
        if(m_BLEController != nullptr)
        {
            m_BLEController->disconnectFromDevice();
            m_BLEController->deleteLater();
            m_BLEController = nullptr;
        }
    }
    else if(m_type == TCP_Client)
    {
        m_TCPSocket->close(); // will call disconnectFromHost()
        // for some unknown reason, the QTCPSocket might keep the error state for a while
        // use a new socket for fast reconnect
        m_TCPSocket->deleteLater();
        m_TCPSocket = new QTcpSocket();
        updateSignalSlot();
    }
    else if(m_type == TCP_Server)
    {
        m_TCPServer->close();
        m_TCPTxClients.clear();
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
    else if(m_type == BLE_Central)
    {
        // The BLE related object is not persistent, so the related signals/slots are not handled there.
        // readyRead(), readAll() -> BLEC_onDataArrived()
        // error() -> onErrorOccurred() (signals are connected when the object is created)
        // connected() -> onConnected() (called in BLEC_onServiceDetailDiscovered())
        // disconnected() -> onErrorOccurred() (no disconnected() signal)
    }
    else if(m_type == TCP_Client)
    {
        m_lastReadyReadConn = connect(m_TCPSocket, &QIODevice::readyRead, this, &Connection::onReadyRead);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        m_lastOnErrorConn = connect(m_TCPSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Connection::onErrorOccurred);
#else
        m_lastOnErrorConn = connect(m_TCPSocket, &QAbstractSocket::errorOccurred, this, &Connection::onErrorOccurred);
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        m_lastOnErrorConn = connect(m_UDPSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Connection::onErrorOccurred);
#else
        m_lastOnErrorConn = connect(m_UDPSocket, &QAbstractSocket::errorOccurred, this, &Connection::onErrorOccurred);
#endif
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
        if(m_isCollectingErrorString && error != QSerialPort::NoError)
            m_errorStringList += m_serialPort->errorString();
        qDebug() << "SerialPort Error:" << error << m_serialPort->errorString();

        if(m_SP_ignoredErrorList.contains(error))
        {
            qDebug() << error << "ignored";
        }
        // no error
        else if(error == QSerialPort::NoError)
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
        if(m_isCollectingErrorString && error != QBluetoothSocket::NoSocketError)
            m_errorStringList += m_BTSocket->errorString();
        qDebug() << "BT Socket Error:" << error << m_BTSocket->errorString();
        qDebug() << "State:" << m_BTSocket->state();

        // no error
        if(error == QBluetoothSocket::NoSocketError)
            ;
        // keep the statement in Server_onClientErrorOccurred() in same
        else if(error == QBluetoothSocket::NetworkError || error == QBluetoothSocket::OperationError)
            ;
        else
        {
            if(m_state == Connecting)
                emit connectFailed(getErrorStringList());
            close(true);
        }
    }
    else if(m_type == BLE_Central)
    {
        if(sender() == m_BLEController)
        {
            QLowEnergyController::Error error;
            error = m_BLEController->error();
            if(m_isCollectingErrorString && error != QLowEnergyController::NoError)
                m_errorStringList += m_BLEController->errorString();
            qDebug() << "BLE Central Controller Error:" << error << m_BLEController->errorString();
            qDebug() << "State:" << m_BLEController->state();

            if(error == QLowEnergyController::NoError)
                ;
            else
            {
                if(m_state == Connecting)
                {
                    QStringList infoList = getErrorStringList();
                    infoList.prepend(tr("Controller Error: "));
                    emit connectFailed(infoList);
                }
                close(true);
            }
        }
        else if(sender() == m_BLERxTxService || sender() == m_BLETxService)
        {
            QLowEnergyService* service = qobject_cast<QLowEnergyService*>(sender());
            QLowEnergyService::ServiceError error;
            error = service->error();
            // service->errorString() doesn't exist
            qDebug() << "BLE Central Service Error:" << error;
            qDebug() << "State:" << service->state();

            if(error == QLowEnergyService::NoError)
                ;
            else if(error == QLowEnergyService::CharacteristicReadError || error == QLowEnergyService::CharacteristicWriteError || error == QLowEnergyService::DescriptorReadError || error == QLowEnergyService::DescriptorWriteError)
                ;
            else
            {
                if(m_state == Connecting)
                    emit connectFailed(tr("Service Error: ")
                                       + QString::fromUtf8(QMetaEnum::fromType<QLowEnergyService::ServiceError>().valueToKey(error)));
                close(true);
            }
        }
    }
    else if(m_type == TCP_Client)
    {
        QAbstractSocket::SocketError error;
        error = m_TCPSocket->error();
        if(m_isCollectingErrorString)
            m_errorStringList += m_TCPSocket->errorString();
        qDebug() << "TCP Socket Error:" << error << m_TCPSocket->errorString();
        qDebug() << "State:" << m_TCPSocket->state();

        // QTcpSocket::SocketTimeoutError? QTcpSocket::DatagramTooLargeError?
        // keep the statement in Server_onClientErrorOccurred() in same
        if(error == QTcpSocket::OperationError || error == QTcpSocket::TemporaryError || error == QTcpSocket::UnsupportedSocketOperationError)
            ;
        else
        {
            if(m_state == Connecting)
                emit connectFailed(getErrorStringList());
            close(true); // this will emit disconnected()
        }
    }
    // untested yet
    // for server, the m_state need to be changed there
    if(m_type == BT_Server)
    {
        QBluetoothServer::Error error;
        error = m_BTServer->error();
        // m_BTServer->errorString() doesn't exist
        qDebug() << "BT Server Error:" << error;

        if(!m_BTServer->isListening())
        {
            // the server is always listening when the server is running, according to my implementation
            onDisconnected();
        }
    }
    else if(m_type == TCP_Server)
    {
        QAbstractSocket::SocketError error;
        error = m_TCPServer->serverError();
        if(m_isCollectingErrorString)
            m_errorStringList += m_TCPServer->errorString();
        qDebug() << "TCP Server Error:" << error <<  m_TCPServer->errorString();
        if(!m_TCPServer->isListening())
        {
            // the server is always listening when the server is running, according to my implementation
            onDisconnected();
        }
    }
    else if(m_type == UDP)
    {
        QAbstractSocket::SocketError error;
        error = m_UDPSocket->error();
        if(m_isCollectingErrorString)
            m_errorStringList += m_UDPSocket->errorString();
        qDebug() << "UDP Error:" << error << m_UDPSocket->errorString();
        qDebug() << "UDP State:" << m_UDPSocket->state();

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
        qint64 maxLen = 0, currLen;
        // write to all connected clients
        for(auto it = m_BTTxClients.cbegin(); it != m_BTTxClients.cend(); ++it)
        {
            currLen = (*it)->write(data, len);
            if(currLen > maxLen)
                maxLen = currLen;
        }
        return maxLen;
    }
    else if(m_type == BLE_Central)
    {
        if(m_BLERxTxMode == BLE_2S2C)
        {
            m_BLETxService->writeCharacteristic(m_BLETxCharacteristic, QByteArray::fromRawData(data, len), m_BLETxWriteMode);
        }
        else
        {
            m_BLERxTxService->writeCharacteristic(m_BLETxCharacteristic, QByteArray::fromRawData(data, len), m_BLETxWriteMode);
        }
        return len; // no feedback
    }
    else if(m_type == TCP_Client)
    {
        return m_TCPSocket->write(data, len);
    }
    else if(m_type == TCP_Server)
    {
        qint64 maxLen = 0, currLen;
        // write to all connected clients
        for(auto it = m_TCPTxClients.cbegin(); it != m_TCPTxClients.cend(); ++it)
        {
            currLen = (*it)->write(data, len);
            if(currLen > maxLen)
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

qint64 Connection::write(const QByteArray & data)
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
    else if(m_type == BT_Client || m_type == BT_Server || m_type == BLE_Central)
    {
        m_lastBTArgument = m_currBTArgument;
        m_lastBTArgumentValid = true;
    }
    else if(m_type == TCP_Client)
    {
        m_lastNetArgument = m_currNetArgument;
        m_lastNetArgumentValid = true;
    }
    else if(m_type == TCP_Server)
    {
        m_lastNetArgument = m_currNetArgument;
        m_lastNetArgumentValid = true;
    }
    else if(m_type == UDP)
    {
        m_lastNetArgument = m_currNetArgument;
        m_lastNetArgumentValid = true;
    }
    if(m_pollTimerEnabled)
        m_pollTimer->start();
    setCollectingErrorStringList(false);
    emit connected();
}

void Connection::onDisconnected()
{
    State oldState = m_state;
    qDebug() << "Connection::onDisconnected()";
    m_pollTimer->stop();
    changeState(Unconnected);
    setCollectingErrorStringList(false);
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
        m_BTTxClients.append(socket);
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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Connection::onErrorOccurred);
#else
        connect(socket, &QAbstractSocket::errorOccurred, this, &Connection::onErrorOccurred);
#endif
        m_TCPConnectedClients.append(socket);
        m_TCPTxClients.append(socket);
        emit TCP_clientConnected();
    }
}

// this will be called by cliendDisconnected() and clientErrorOccurred()
void Connection::Server_onClientDisconnectedHandler(QObject * clientObj)
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
        m_BTTxClients.removeOne(socket);
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
        m_TCPTxClients.removeOne(socket);
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
        if(m_isCollectingErrorString && socketError != QBluetoothSocket::NoSocketError)
            m_errorStringList += socket->errorString();
        qDebug() << "BT Socket Error:" << socketError << socket->errorString();
        qDebug() << "State:" << socket->state();

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
        if(m_isCollectingErrorString)
            m_errorStringList += socket->errorString();
        qDebug() << "TCP Socket Error:" << socketError << socket->errorString();
        qDebug() << "State:" << socket->state();

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
    if(isConnected() && m_lastSPArgumentValid)
        m_lastSPArgument.baudRate = baudRate;
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
    if(isConnected() && m_lastSPArgumentValid)
        m_lastSPArgument.dataBits = dataBits;
    return true;
}

bool Connection::SP_setStopBits(QSerialPort::StopBits stopBits)
{
    if(m_type != SerialPort)
        return false;
    if(!m_serialPort->setStopBits(stopBits))
        return false;
    m_currSPArgument.stopBits = stopBits;
    if(isConnected() && m_lastSPArgumentValid)
        m_lastSPArgument.stopBits = stopBits;
    return true;
}

bool Connection::SP_setParity(QSerialPort::Parity parity)
{
    if(m_type != SerialPort)
        return false;
    if(!m_serialPort->setParity(parity))
        return false;
    m_currSPArgument.parity = parity;
    if(isConnected() && m_lastSPArgumentValid)
        m_lastSPArgument.parity = parity;
    return true;
}

bool Connection::SP_setFlowControl(QSerialPort::FlowControl flowControl)
{
    if(m_type != SerialPort)
        return false;
    if(!m_serialPort->setFlowControl(flowControl))
        return false;
    m_currSPArgument.flowControl = flowControl;
    if(isConnected() && m_lastSPArgumentValid)
        m_lastSPArgument.flowControl = flowControl;
    return true;
}

void Connection::SP_setIgnoredErrorList(const QList<QSerialPort::SerialPortError> &errorList)
{
    m_SP_ignoredErrorList = errorList;
}

QList<QSerialPort::SerialPortError> Connection::SP_getIgnoredErrorList()
{
    return m_SP_ignoredErrorList;
}

QString Connection::BT_remoteName()
{
    if(m_type == BT_Client && m_BTSocket != nullptr)
        return m_BTSocket->peerName();
    else if(m_type == BLE_Central && m_BLEController != nullptr)
        return m_BLEController->remoteName();

    return QString();
}

QBluetoothAddress Connection::BT_localAddress()
{
    if(m_type == BT_Client && m_BTSocket != nullptr)
        return m_BTSocket->localAddress();
    else if(m_type == BT_Server && m_BTServer != nullptr)
        return m_BTServer->serverAddress();
    else if(m_type == BLE_Central && m_BLEController != nullptr)
        return m_BLEController->localAddress();
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

bool Connection::BTServer_setClientMode(QBluetoothSocket * clientSocket, bool RxEnabled, bool TxEnabled)
{
    if(!m_BTConnectedClients.contains(clientSocket))
        return false;
    if(RxEnabled)
    {
        disconnect(clientSocket, &QBluetoothSocket::readyRead, this, &Connection::blackhole);
        connect(clientSocket, &QBluetoothSocket::readyRead, this, &Connection::onReadyRead);
    }
    else
    {
        disconnect(clientSocket, &QBluetoothSocket::readyRead, this, &Connection::onReadyRead);
        connect(clientSocket, &QBluetoothSocket::readyRead, this, &Connection::blackhole);
    }

    if(TxEnabled && !m_BTTxClients.contains(clientSocket))
        m_BTTxClients.append(clientSocket);
    else if(!TxEnabled)
        m_BTTxClients.removeOne(clientSocket);

    return true;
}

void Connection::UDP_setRemote(const QString & addr, quint16 port)
{
    if(m_type != UDP)
        return;
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

bool Connection::TCPServer_setClientMode(QTcpSocket * clientSocket, bool RxEnabled, bool TxEnabled)
{
    if(!m_TCPConnectedClients.contains(clientSocket))
        return false;
    if(RxEnabled)
    {
        disconnect(clientSocket, &QTcpSocket::readyRead, this, &Connection::blackhole);
        connect(clientSocket, &QTcpSocket::readyRead, this, &Connection::onReadyRead);
    }
    else
    {
        disconnect(clientSocket, &QTcpSocket::readyRead, this, &Connection::onReadyRead);
        connect(clientSocket, &QTcpSocket::readyRead, this, &Connection::blackhole);
    }

    if(TxEnabled && !m_TCPTxClients.contains(clientSocket))
        m_TCPTxClients.append(clientSocket);
    else if(!TxEnabled)
        m_TCPTxClients.removeOne(clientSocket);

    return true;
}

void Connection::blackhole()
{
    // discard received data
    // not efficient, but works
    qobject_cast<QIODevice*>(sender())->readAll();
}

void Connection::BLEC_onServiceDiscovered(const QBluetoothUuid & serviceUUID)
{
    m_BLEDiscoveredServices.append(serviceUUID);
    if(m_BLERxTxService != nullptr && (m_BLETxService != nullptr || m_currBTArgument.RxServiceUUID == m_currBTArgument.TxServiceUUID))
        return;
    auto service = m_BLEController->createServiceObject(serviceUUID);
    if(m_BLERxTxService == nullptr && m_currBTArgument.RxServiceUUID == serviceUUID)
        m_BLERxTxService = service;
    if(m_BLETxService == nullptr && m_currBTArgument.RxServiceUUID != m_currBTArgument.TxServiceUUID && m_currBTArgument.TxServiceUUID == serviceUUID)
        m_BLETxService = service;
    // for characteristics and included services
    connect(service, &QLowEnergyService::stateChanged, this, &Connection::BLEC_onServiceDetailDiscovered);
    service->discoverDetails();
}

void Connection::BLEC_onServiceDetailDiscovered(QLowEnergyService::ServiceState newState)
{
    bool deleteService = true;
    auto service = qobject_cast<QLowEnergyService*>(sender());
    if(newState == QLowEnergyService::InvalidService)
        deleteService = true;
    else if(newState == QLowEnergyService::ServiceDiscovered)
    {
        deleteService = true;
        // add included services
        const QList<QBluetoothUuid> includedServices = service->includedServices();
        for(auto it = includedServices.cbegin(); it != includedServices.cend(); ++it)
        {
            // prevent circular dependency
            if(m_BLEDiscoveredServices.contains(*it))
                continue;
            BLEC_onServiceDiscovered(*it);
        }

        const QList<QLowEnergyCharacteristic> chars = service->characteristics();
        // delete unused service
        if(service != m_BLERxTxService && service != m_BLETxService)
        {
            deleteService = true;
        }
        else if(m_BLERxTxMode == BLE_1S1C && service == m_BLERxTxService)
        {
            for(auto it = chars.cbegin(); it != chars.cend(); ++it)
            {
                if(!m_BLERxCharacteristicValid && it->uuid() == m_currBTArgument.RxCharacteristicUUID && it->properties().testFlag(QLowEnergyCharacteristic::Notify))
                {
                    const bool hasWriteProperty = it->properties().testFlag(QLowEnergyCharacteristic::Write);
                    const bool hasWriteNoResponseProperty = it->properties().testFlag(QLowEnergyCharacteristic::WriteNoResponse);
                    if(hasWriteProperty || hasWriteNoResponseProperty)
                    {
                        // Use WRITE if both properties are supported
                        m_BLETxWriteMode = hasWriteProperty ? QLowEnergyService::WriteWithResponse : QLowEnergyService::WriteWithoutResponse;
                        m_BLERxCharacteristicValid = true;
                        m_BLETxCharacteristicValid = true;
                        deleteService = false;
                    }
                }
            }
        }
        else if(m_BLERxTxMode == BLE_1S2C && service == m_BLERxTxService)
        {
            for(auto it = chars.cbegin(); it != chars.cend(); ++it)
            {
                if(!m_BLERxCharacteristicValid && it->uuid() == m_currBTArgument.RxCharacteristicUUID && it->properties().testFlag(QLowEnergyCharacteristic::Notify))
                {
                    m_BLERxCharacteristicValid = true;
                    deleteService = false;
                }
                if(!m_BLETxCharacteristicValid && it->uuid() == m_currBTArgument.TxCharacteristicUUID)
                {
                    const bool hasWriteProperty = it->properties().testFlag(QLowEnergyCharacteristic::Write);
                    const bool hasWriteNoResponseProperty = it->properties().testFlag(QLowEnergyCharacteristic::WriteNoResponse);
                    if(hasWriteProperty || hasWriteNoResponseProperty)
                    {
                        // Use WRITE if both properties are supported
                        m_BLETxWriteMode = hasWriteProperty ? QLowEnergyService::WriteWithResponse : QLowEnergyService::WriteWithoutResponse;
                        m_BLETxCharacteristicValid = true;
                        deleteService = false;
                    }
                }
            }
        }
        else if(m_BLERxTxMode == BLE_2S2C && service == m_BLERxTxService)
        {
            for(auto it = chars.cbegin(); it != chars.cend(); ++it)
            {
                if(!m_BLERxCharacteristicValid && it->uuid() == m_currBTArgument.RxCharacteristicUUID && it->properties().testFlag(QLowEnergyCharacteristic::Notify))
                {
                    m_BLERxCharacteristicValid = true;
                    deleteService = false;
                }
            }
        }
        else if(service == m_BLETxService && m_BLERxTxMode == BLE_2S2C)
        {
            for(auto it = chars.cbegin(); it != chars.cend(); ++it)
            {
                if(!m_BLETxCharacteristicValid && it->uuid() == m_currBTArgument.TxCharacteristicUUID)
                {
                    const bool hasWriteProperty = it->properties().testFlag(QLowEnergyCharacteristic::Write);
                    const bool hasWriteNoResponseProperty = it->properties().testFlag(QLowEnergyCharacteristic::WriteNoResponse);
                    if(hasWriteProperty || hasWriteNoResponseProperty)
                    {
                        // Use WRITE if both properties are supported
                        m_BLETxWriteMode = hasWriteProperty ? QLowEnergyService::WriteWithResponse : QLowEnergyService::WriteWithoutResponse;
                        m_BLETxCharacteristicValid = true;
                        deleteService = false;
                    }
                }
            }
        }


        if(!deleteService)
        {
            if(m_BLERxCharacteristicValid && m_BLETxCharacteristicValid)
            {
                // Rx
                connect(m_BLERxTxService, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &Connection::onErrorOccurred);
                connect(m_BLERxTxService, &QLowEnergyService::characteristicChanged, this, &Connection::BLEC_onDataArrived);
                connect(m_BLERxTxService, &QLowEnergyService::characteristicRead, this, &Connection::BLEC_onDataArrived);
                QLowEnergyDescriptor desc = m_BLERxTxService->characteristic(m_currBTArgument.RxCharacteristicUUID).descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                m_BLERxTxService->writeDescriptor(desc, QByteArray::fromHex("0100"));
                // Tx
                if(m_BLERxTxMode == BLE_2S2C)
                {
                    connect(m_BLETxService, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error), this, &Connection::onErrorOccurred);
                    m_BLETxCharacteristic = m_BLETxService->characteristic(m_currBTArgument.TxCharacteristicUUID);
                }
                else
                    m_BLETxCharacteristic = m_BLERxTxService->characteristic(m_currBTArgument.TxCharacteristicUUID);
                onConnected();
            }
        }
        else
        {
            m_BLEDiscoveredServices.removeOne(service->serviceUuid());
            if(service == m_BLERxTxService) // characteristic not found
            {
                m_BLERxTxService = nullptr;
            }
            else if(service == m_BLETxService) // characteristic not found
            {
                m_BLETxService = nullptr;
            }
            service->deleteLater();
            // all root services and included services are handled
            if(m_BLEDiscoveredServices.isEmpty() && m_BLEController->state() == QLowEnergyController::DiscoveredState)
            {
                qDebug() << "Cannot find Tx/Rx Characteristic matching the requirement.";
                close();
            }
        }
    }
}

void Connection::BLEC_onDataArrived(const QLowEnergyCharacteristic & characteristic, const QByteArray & newValue)
{
    Q_UNUSED(characteristic)
    m_buf += newValue;
    emit readyRead();
}

void Connection::setCollectingErrorStringList(bool state)
{
    m_isCollectingErrorString = state;
    if(state)
        m_errorStringList.clear();
}

const QMap<Connection::Type, QLatin1String> Connection::m_typeNameMap
{
    {Connection::SerialPort, QLatin1String(QT_TR_NOOP("SerialPort"))},
    {Connection::BT_Client, QLatin1String(QT_TR_NOOP("Bluetooth Client"))},
    {Connection::BT_Server, QLatin1String(QT_TR_NOOP("Bluetooth Server"))},
    {Connection::BLE_Central, QLatin1String(QT_TR_NOOP("BLE Central"))},
    {Connection::BLE_Peripheral, QLatin1String(QT_TR_NOOP("BLE Peripheral"))},
    {Connection::TCP_Client, QLatin1String(QT_TR_NOOP("TCP Client"))},
    {Connection::TCP_Server, QLatin1String(QT_TR_NOOP("TCP Server"))},
    {Connection::UDP, QLatin1String(QT_TR_NOOP("UDP"))}
};

bool Connection::NetworkArgument::operator==(const NetworkArgument &other) const
{
    // alias doesn't matter
    return localAddress == other.localAddress
           && localPort == other.localPort
           && remoteName == other.remoteName
           && remotePort == other.remotePort;
}

QDebug operator<<(QDebug dbg, const Connection::SerialPortArgument& arg)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "("
                  << arg.name << ","
                  << arg.baudRate << ",";
    if(arg.dataBits > 0 && arg.parity == QSerialPort::NoParity && arg.stopBits == QSerialPort::OneStop)
        dbg.nospace() << (int)arg.dataBits << "N1,";
    else
    {
        dbg.nospace()
                << arg.dataBits << ","
                << arg.stopBits << ","
                << arg.parity << ",";
    }
    dbg.nospace()
            << arg.flowControl << ","
            << arg.id << ")";
    return dbg;
}

QDebug operator<<(QDebug dbg, const Connection::BTArgument& arg)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "("
                  << arg.deviceAddress << ","
                  << arg.serverServiceName << ","
                  << arg.localAdapterAddress << ","
                  << arg.RxServiceUUID << ","
                  << arg.RxCharacteristicUUID << ","
                  << arg.TxServiceUUID << ","
                  << arg.TxCharacteristicUUID << ")";
    return dbg;
}

QDebug operator<<(QDebug dbg, const Connection::NetworkArgument& arg)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "("
                  << arg.localAddress  << ","
                  << arg.remoteName << ","
                  << arg.remotePort  << ","
                  << arg.localPort  << ","
                  << arg.alias << ")";
    return dbg;
}
