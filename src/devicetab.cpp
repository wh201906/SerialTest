#include "devicetab.h"
#include "ui_devicetab.h"
#include "util.h"

#include <QDebug>
#include <QMessageBox>
#include <QBluetoothUuid>
#include <QBluetoothLocalDevice>
#include <QNetworkInterface>
#include <QTreeWidgetItem>
#include <QScroller>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#endif
#ifdef _MSC_VER
#include <winrtbluetooth.h>
#endif

const QMap<QString, QString> DeviceTab::m_historyPrefix =
{
    {QLatin1String("SP"), QLatin1String("SerialTest_History_SerialPort")},
    {QLatin1String("BTClient"), QLatin1String("SerialTest_History_BT_Client")},
    {QLatin1String("BTServer"), QLatin1String("SerialTest_History_BT_Server")},
    {QLatin1String("BLEC"), QLatin1String("SerialTest_History_BLE_Central")},
    {QLatin1String("TCPServer"), QLatin1String("SerialTest_History_TCP_Server")},
    {QLatin1String("TCPClient"), QLatin1String("SerialTest_History_TCP_Client")},
    {QLatin1String("UDP"), QLatin1String("SerialTest_History_UDP")},
};

DeviceTab::DeviceTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceTab)
{
    ui->setupUi(this);

    m_netPortValidator = new QIntValidator(this);
    m_netPortValidator->setRange(0, 65535);

    connect(ui->SP_portList, &QTableWidget::cellClicked, this, &DeviceTab::onTargetListCellClicked);
    connect(ui->BTClient_deviceList, &QTableWidget::cellClicked, this, &DeviceTab::onTargetListCellClicked);
    connect(ui->BLEC_deviceList, &QTableWidget::cellClicked, this, &DeviceTab::onTargetListCellClicked);
    connect(ui->BLEC_RxServiceUUIDBox, &QComboBox::currentTextChanged, this, &DeviceTab::on_BLEC_ServiceUUIDBox_currentTextChanged);
    connect(ui->BLEC_TxServiceUUIDBox, &QComboBox::currentTextChanged, this, &DeviceTab::on_BLEC_ServiceUUIDBox_currentTextChanged);
    connect(ui->Net_addrPortList, &QTableWidget::cellClicked, this, &DeviceTab::onTargetListCellClicked);
    connect(ui->Net_remoteAddrEdit, &QLineEdit::editingFinished, this, &DeviceTab::Net_onRemoteChanged);
    connect(ui->Net_remotePortEdit, &QLineEdit::editingFinished, this, &DeviceTab::Net_onRemoteChanged);
    ui->SP_baudRateBox->installEventFilter(this);
    ui->BLECentralListSplitter->handle(1)->installEventFilter(this);

    initUI();
    refreshTargetList();
}

DeviceTab::~DeviceTab()
{
    delete ui;
}

void DeviceTab::initSettings()
{
    settings = MySettings::defaultSettings();

    // config and history are loaded in this function

    settings->beginGroup("SerialTest");
    m_maxHistoryNum = settings->value("History_MaxCount", 200).toInt();
    settings->endGroup();

    // arrays
    QStringList groups = settings->childGroups();
    int size;
    size = 0;
    if(!groups.contains(m_historyPrefix["SP"]))
        settings->beginWriteArray(m_historyPrefix["SP"], 0);
    else
        size = settings->beginReadArray(m_historyPrefix["SP"]);
    for(int i = 0; i < size; i++)
    {
        settings->setArrayIndex(i);

        QStringList argList = settings->value("Arg").toStringList();
        Connection::SerialPortArgument arg = Connection::stringList2SPArg(argList);
        if(!arg.name.isEmpty())
        {
            m_SPArgHistory.append(arg);
            // don't use i as index there
            // empty arguments are skipped
            m_SPArgHistoryIndex[arg.id] = m_SPArgHistory.size() - 1;
        }
    }
    settings->endArray();
//    if(!groups.contains(m_historyPrefix["BLEC"]))
//        settings->beginWriteArray(m_historyPrefix["BLEC"], 0);
    size = 0;
    if(!groups.contains(m_historyPrefix["TCPClient"]))
        settings->beginWriteArray(m_historyPrefix["TCPClient"], 0);
    else
        size = settings->beginReadArray(m_historyPrefix["TCPClient"]);
    for(int i = 0; i < size; i++)
    {
        settings->setArrayIndex(i);

        QStringList argList = settings->value("Arg").toStringList();
        Connection::NetworkArgument arg = Connection::stringList2NetArg(argList);
        if(!arg.localAddress.isNull())
        {
            m_TCPClientHistory.append(arg);
        }
    }
    settings->endArray();

    size = 0;
    if(!groups.contains(m_historyPrefix["UDP"]))
        settings->beginWriteArray(m_historyPrefix["UDP"], 0);
    else
        size = settings->beginReadArray(m_historyPrefix["UDP"]);
    for(int i = 0; i < size; i++)
    {
        settings->setArrayIndex(i);

        QStringList argList = settings->value("Arg").toStringList();
        Connection::NetworkArgument arg = Connection::stringList2NetArg(argList);
        if(!arg.localAddress.isNull())
        {
            m_UDPHistory.append(arg);
        }
    }
    settings->endArray();

    if(m_SPArgHistory.isEmpty())
        loadSPPreference();
    else
        loadSPPreference(m_SPArgHistory.last());

    // TCP client preference(last connected) is loaded in on_typeBox_currentIndexChanged()
    // because TCP client/TCP server/UDP share the same widgets

    // non-arrays
    settings->beginGroup(m_historyPrefix["BTClient"]);
    ui->BTClient_serviceUUIDBox->setChecked(settings->value("UserSpecifiedServiceUUID", false).toBool());
    ui->BTClient_serviceUUIDEdit->setText(settings->value("ServiceUUID").toString());
    settings->endGroup();
    on_BTClient_serviceUUIDBox_clicked();
    settings->beginGroup(m_historyPrefix["BTServer"]);
    ui->BTServer_serviceNameEdit->setText(settings->value("LastServiceName", "SerialTest_BT").toString());
    settings->endGroup();

    // TCP server preference(last connected) is loaded in on_typeBox_currentIndexChanged()
}

void DeviceTab::setConnection(Connection *conn)
{
    m_connection = conn;
}

void DeviceTab::refreshTargetList()
{
    if(m_connection == nullptr)
        return;
    Connection::Type currType = m_connection->type();
    if(currType == Connection::SerialPort)
    {
        ui->SP_portList->setRowCount(0);
        ui->SP_portNameBox->clear();
        QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
        ui->SP_portList->setRowCount(ports.size());
        for(int i = 0; i < ports.size(); i++)
        {
            ui->SP_portList->setItem(i, 0, new QTableWidgetItem(ports[i].portName()));
            ui->SP_portNameBox->addItem(ports[i].portName());
            ui->SP_portList->setItem(i, 1, new QTableWidgetItem(ports[i].description()));
            ui->SP_portList->setItem(i, 2, new QTableWidgetItem(ports[i].manufacturer()));
            ui->SP_portList->setItem(i, 3, new QTableWidgetItem(ports[i].serialNumber()));
            ui->SP_portList->setItem(i, 4, new QTableWidgetItem(ports[i].isNull() ? tr("Yes") : tr("No")));
            ui->SP_portList->setItem(i, 5, new QTableWidgetItem(ports[i].systemLocation()));
            quint16 xid;
            QString xidString;
            QTableWidgetItem* idItem;
            xid = ports[i].vendorIdentifier();
            xidString = (xid == 0) ? "0" : QString("%1(%2)").arg(xid).arg(xid, 4, 16, QLatin1Char('0'));
            idItem = new QTableWidgetItem(xidString);
            idItem->setData(Qt::UserRole, xid);
            ui->SP_portList->setItem(i, 6, idItem);
            xid = ports[i].productIdentifier();
            xidString = (xid == 0) ? "0" : QString("%1(%2)").arg(xid).arg(xid, 4, 16, QLatin1Char('0'));
            idItem = new QTableWidgetItem(xidString);
            idItem->setData(Qt::UserRole, xid);
            ui->SP_portList->setItem(i, 7, idItem);
            QList<qint32> baudRateList = ports[i].standardBaudRates();
            QString baudRates = "";
            for(int j = 0; j < baudRates.size(); j++)
            {
                baudRates += QString::number(baudRateList[j]) + ", ";
            }
            ui->SP_portList->setItem(i, 8, new QTableWidgetItem(baudRates));
        }
    }
    else if(currType == Connection::BT_Client)
    {
        ui->BTClient_deviceList->setRowCount(0);
        m_shownBTDevices.clear();
        ui->BTClient_targetAddrBox->clear();
        ui->refreshButton->setText(tr("Searching..."));
#ifdef Q_OS_ANDROID
        getBondedTarget(false);
#endif
        BTClient_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
    }
    else if(currType == Connection::BLE_Central)
    {
        ui->BLEC_deviceList->setRowCount(0);
        m_shownBTDevices.clear();
        ui->BLEC_targetAddrBox->clear();
        ui->BLEC_currAddrBox->clear();
        ui->refreshButton->setText(tr("Searching..."));
#ifdef Q_OS_ANDROID
        getBondedTarget(true);
#endif
        BTClient_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    }
}

#ifdef Q_OS_ANDROID
bool DeviceTab::getPermission(const QString& permission)
{
    QtAndroid::PermissionResult result = QtAndroid::checkPermission(permission);
    if(result == QtAndroid::PermissionResult::Denied)
    {
        QtAndroid::requestPermissionsSync(QStringList() << permission);
        result = QtAndroid::checkPermission(permission);
        if(result == QtAndroid::PermissionResult::Denied)
            return false;
    }
    return true;
}

void DeviceTab::getRequiredPermission()
{
    QStringList permissionList =
    {
        "android.permission.ACCESS_FINE_LOCATION",
        "android.permission.BLUETOOTH_ADMIN"
    };
    if(QtAndroid::androidSdkVersion() >= 31)
    {
        permissionList += "android.permission.BLUETOOTH_SCAN";
        permissionList += "android.permission.BLUETOOTH_CONNECT";
    }
    else
    {
        permissionList += "android.permission.BLUETOOTH";
    }
    for(const QString& permission : permissionList)
    {
        if(!getPermission(permission))
            qDebug() << "Failed to request permission" << permission;
    }
}

void DeviceTab::getBondedTarget(bool isBLE)
{
    QAndroidJniEnvironment androidEnv;
    getRequiredPermission();
    QAndroidJniObject array = QtAndroid::androidActivity().callObjectMethod("getBondedDevices", "(Z)[Ljava/lang/String;", isBLE);
    int arrayLen = androidEnv->GetArrayLength(array.object<jarray>());
    qDebug() << "arrayLen:" << arrayLen;
    QTableWidget* deviceList = isBLE ? ui->BLEC_deviceList : ui->BTClient_deviceList;
    deviceList->setRowCount(arrayLen);
    for(int i = 0; i < arrayLen; i++)
    {
        QString info = QAndroidJniObject::fromLocalRef(androidEnv->GetObjectArrayElement(array.object<jobjectArray>(), i)).toString();
        QString address = info.left(info.indexOf(' '));
        QString name = info.right(info.length() - info.indexOf(' ') - 1);
        qDebug() << address << name;
        deviceList->setItem(i, 0, new QTableWidgetItem(name));
        deviceList->setItem(i, 1, new QTableWidgetItem(address));
        deviceList->setItem(i, 2, new QTableWidgetItem(tr("Bonded")));
        deviceList->setItem(i, 3, new QTableWidgetItem());
        m_shownBTDevices[address] = i;
        if(isBLE)
        {
            ui->BLEC_currAddrBox->addItem(address);
            ui->BLEC_targetAddrBox->addItem(address);
        }
        else
        {
            ui->BTClient_targetAddrBox->addItem(address);
        }
    }
}
#endif

void DeviceTab::initUI()
{
    ui->SP_portList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->BTClient_deviceList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->BTServer_deviceList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->BLEC_deviceList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->BLEC_UUIDList->header()->setStretchLastSection(false); // when stretchLastSection is true, sectionResizeMode will be ignored
    ui->BLEC_UUIDList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->Net_addrPortList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->SP_flowControlBox->addItem(tr("NoFlowControl"), QSerialPort::NoFlowControl);
    ui->SP_flowControlBox->addItem(tr("HardwareControl"), QSerialPort::HardwareControl);
    ui->SP_flowControlBox->addItem(tr("SoftwareControl"), QSerialPort::SoftwareControl);

    ui->SP_parityBox->addItem(tr("NoParity"), QSerialPort::NoParity);
    ui->SP_parityBox->addItem(tr("EvenParity"), QSerialPort::EvenParity);
    ui->SP_parityBox->addItem(tr("OddParity"), QSerialPort::OddParity);
    ui->SP_parityBox->addItem(tr("SpaceParity"), QSerialPort::SpaceParity);
    ui->SP_parityBox->addItem(tr("MarkParity"), QSerialPort::MarkParity);

    ui->SP_stopBitsBox->addItem("1", QSerialPort::OneStop);
    ui->SP_stopBitsBox->addItem("1.5", QSerialPort::OneAndHalfStop);
    ui->SP_stopBitsBox->addItem("2", QSerialPort::TwoStop);

    ui->SP_dataBitsBox->addItem("5", QSerialPort::Data5);
    ui->SP_dataBitsBox->addItem("6", QSerialPort::Data6);
    ui->SP_dataBitsBox->addItem("7", QSerialPort::Data7);
    ui->SP_dataBitsBox->addItem("8", QSerialPort::Data8);

    ui->Net_localPortEdit->setValidator(m_netPortValidator);
    ui->Net_remotePortEdit->setValidator(m_netPortValidator);
}

void DeviceTab::getAvailableTypes(bool useFirstValid)
{
    int firstValid = -1;
    int num = 0;
    Connection::Type currType = ui->typeBox->currentData().value<Connection::Type>();
    QStandardItemModel* model = static_cast<QStandardItemModel*>(ui->typeBox->model());
    // need some code to check if BLE is supported
    QVector<Connection::Type> invalid =
    {Connection::BLE_Peripheral};
#ifdef Q_OS_ANDROID
    settings->beginGroup("SerialTest_Connect");
    bool AndroidHWSerialEnabled = settings->value("Android_HWSerial", false).toBool();
    settings->endGroup();
    if(!AndroidHWSerialEnabled)
        invalid += Connection::SerialPort;
#endif
#ifdef Q_OS_WINDOWS
    // Qt5 and Qt6 doesn't support BLE Peripheral on Windows
    // https://doc.qt.io/qt-5/qtbluetooth-index.html
    // https://doc.qt.io/qt-6/qtbluetooth-index.html
    invalid += Connection::BLE_Peripheral;
#endif
    // check Bluetooth adapters, add adapter info into adapterBox
    num = updateBTAdapterList();
    if(num == 0)
    {
        invalid += Connection::BT_Client;
        invalid += Connection::BT_Server;
        invalid += Connection::BLE_Central;
        invalid += Connection::BLE_Peripheral;
    }
    else if(num == 1)
    {
        ui->BTClient_localAdapterWidget->hide();
        ui->BLEC_localAdapterWidget->hide();
    }
    else // more than one adapter
    {
        ui->BTClient_localAdapterWidget->show();
        ui->BLEC_localAdapterWidget->show();
    }
    on_BTClient_adapterBox_activated(0); // index is unused there
    on_BTServer_adapterBox_activated(0); // index is unused there

    // check network interfaces
    num = updateNetInterfaceList();
    if(num == 0)
    {
        invalid += Connection::TCP_Client;
        invalid += Connection::TCP_Server;
        invalid += Connection::UDP;
    }

    ui->typeBox->blockSignals(true);
    ui->typeBox->clear();
    for(auto it = Connection::getTypeNameMap().cbegin(); it != Connection::getTypeNameMap().cend(); ++it)
    {
        const auto& type = it.key();
        // Connection::getTypeName() will return the translated QString

        if(invalid.contains(type))
        {
            ui->typeBox->addItem("!" + Connection::getTypeName(type), type);
            Util::disableItem(model, type);
        }
        else
        {
            ui->typeBox->addItem(Connection::getTypeName(type), type);
            if(firstValid < 0 && useFirstValid)
            {
                ui->typeBox->setCurrentIndex(type);
                on_typeBox_currentIndexChanged(type); // this signal is blocked now
                firstValid = type;
            }
        }
    }
    if(!useFirstValid)
        ui->typeBox->setCurrentIndex(currType);
    ui->typeBox->blockSignals(false);
}

qint64 DeviceTab::updateBTAdapterList()
{
    // Apps can only get a dummy MAC address 02:00:00:00:00:00 since Android 6.0
    // https://developer.android.com/about/versions/marshmallow/android-6.0-changes#behavior-hardware-id
    qint64 adapterID = 0;

    ui->BTClient_adapterBox->clear();
    ui->BTServer_adapterBox->clear();
    ui->BLEC_adapterBox->clear();
#ifdef Q_OS_ANDROID
    // need modify
    getRequiredPermission();
#endif
#ifdef _MSC_VER
    // Only powered on adapters
    auto BTAdapterList = WinRTBluetooth::allLocalDevices(true);
#else
    // All adapters
    auto BTAdapterList = QBluetoothLocalDevice::allDevices();
#endif
    for(auto it = BTAdapterList.cbegin(); it != BTAdapterList.cend(); ++it)
    {
        qDebug() << "dev:" << it->name() << it->address();
#ifndef _MSC_VER
        QBluetoothLocalDevice dev(it->address());
        if(!dev.isValid() || dev.hostMode() == QBluetoothLocalDevice::HostPoweredOff)
        {
            continue;
        }
#endif
        QString name = QString("%1:%2").arg(adapterID + 1).arg(it->name());
        ui->BTClient_adapterBox->addItem(name, it->address().toString());
        ui->BTServer_adapterBox->addItem(name, it->address().toString());
        ui->BLEC_adapterBox->addItem(name, it->address().toString());
        adapterID++;
    }
    return adapterID; // adapter count
}

qint64 DeviceTab::updateNetInterfaceList()
{
    Connection::Type currType = ui->typeBox->currentData().value<Connection::Type>();
    QHostAddress currNetLocalAddr;
    QList<QHostAddress> shownAddresses;

    if(currType == Connection::UDP)
    {
        // for multicast address
        if(ui->Net_localAddrBox->currentText() != m_autoLocalAddress && ui->Net_localAddrBox->currentText() != m_anyLocalAddress)
            currNetLocalAddr = QHostAddress(ui->Net_localAddrBox->currentText());
    }
    ui->Net_localAddrBox->clear();
    if(currType == Connection::TCP_Client)
        ui->Net_localAddrBox->addItem(m_autoLocalAddress);
    else if(currType == Connection::TCP_Server || currType == Connection::UDP)
        ui->Net_localAddrBox->addItem(m_anyLocalAddress);
    auto netInterfaceList = QNetworkInterface::allAddresses();
    for(auto it = netInterfaceList.cbegin(); it != netInterfaceList.cend(); ++it)
    {
        shownAddresses.append(*it);
        ui->Net_localAddrBox->addItem(it->toString());
    }
#ifdef Q_OS_ANDROID
    QAndroidJniEnvironment androidEnv;
    QAndroidJniObject array = QtAndroid::androidActivity().callObjectMethod("getIPv6Addresses", "()[Ljava/lang/String;");
    int arrayLen = androidEnv->GetArrayLength(array.object<jarray>());
    qDebug() << "arraylen:" << arrayLen;
    for(int i = 0; i < arrayLen; i++)
    {
        QString addressStr = QAndroidJniObject::fromLocalRef(androidEnv->GetObjectArrayElement(array.object<jobjectArray>(), i)).toString();
        QHostAddress address(addressStr);
        // QHostAddress has operator==(), so QList::contains() works
        if(!shownAddresses.contains(address))
        {
            shownAddresses.append(address);
            ui->Net_localAddrBox->addItem(addressStr);
        }
    }
#endif

    if(currType == Connection::UDP && currNetLocalAddr.isMulticast())
        ui->Net_localAddrBox->setCurrentText(currNetLocalAddr.toString());
    return netInterfaceList.size();
}

void DeviceTab::onClientCountChanged()
{
    if(m_connection->type() == Connection::BT_Server)
    {
        ui->BTServer_deviceList->setRowCount(0);
        auto list = m_connection->BTServer_clientList();
        ui->BTServer_deviceList->setRowCount(list.size());
        ui->BTServer_deviceList->blockSignals(true); // avoid emitting cellChanged()
        for(int i = 0; i < list.size(); i++)
        {
            QTableWidgetItem* tmpItem;
            tmpItem = new QTableWidgetItem(list[i]->peerName());
            tmpItem->setData(Qt::UserRole, QVariant((quintptr)list[i]));
            ui->BTServer_deviceList->setItem(i, 0, tmpItem);
            ui->BTServer_deviceList->setItem(i, 1, new QTableWidgetItem(list[i]->peerAddress().toString()));

            QPushButton* disconnectButton = new QPushButton;
            disconnectButton->setText(tr("Disconnect"));
            connect(disconnectButton, &QPushButton::clicked, list[i], &QBluetoothSocket::disconnectFromService);
            ui->BTServer_deviceList->setIndexWidget(ui->BTServer_deviceList->model()->index(i, 2), disconnectButton);
            tmpItem = new QTableWidgetItem();
            tmpItem->setFlags(tmpItem->flags() | Qt::ItemIsUserCheckable);
            tmpItem->setCheckState(Qt::Checked);
            ui->BTServer_deviceList->setItem(i, 3, tmpItem);
            tmpItem = new QTableWidgetItem();
            tmpItem->setFlags(tmpItem->flags() | Qt::ItemIsUserCheckable);
            tmpItem->setCheckState(Qt::Checked);
            ui->BTServer_deviceList->setItem(i, 4, tmpItem);
        }
        ui->BTServer_deviceList->blockSignals(false);
    }
    else if(m_connection->type() == Connection::TCP_Server)
    {
        ui->Net_addrPortList->setRowCount(0);
        auto list = m_connection->TCPServer_clientList();
        ui->Net_addrPortList->setRowCount(list.size());
        ui->Net_addrPortList->blockSignals(true); // avoid emitting cellChanged()
        for(int i = 0; i < list.size(); i++)
        {
            ui->Net_addrPortList->setItem(i, 0, new QTableWidgetItem(list[i]->peerName()));
            ui->Net_addrPortList->setItem(i, 1, new QTableWidgetItem(list[i]->localAddress().toString()));
            ui->Net_addrPortList->setItem(i, 2, new QTableWidgetItem(QString::number(list[i]->localPort())));
            QTableWidgetItem* tmpItem;
            tmpItem = new QTableWidgetItem(list[i]->peerAddress().toString());
            tmpItem->setData(Qt::UserRole, QVariant((quintptr)list[i]));
            ui->Net_addrPortList->setItem(i, 3, tmpItem);
            ui->Net_addrPortList->setItem(i, 4, new QTableWidgetItem(QString::number(list[i]->peerPort())));

            QPushButton* disconnectButton = new QPushButton;
            disconnectButton->setText(tr("Disconnect"));
            connect(disconnectButton, &QPushButton::clicked, list[i], &QTcpSocket::disconnectFromHost);
            ui->Net_addrPortList->setIndexWidget(ui->Net_addrPortList->model()->index(i, 5), disconnectButton);
            tmpItem = new QTableWidgetItem();
            tmpItem->setFlags(tmpItem->flags() | Qt::ItemIsUserCheckable);
            tmpItem->setCheckState(Qt::Checked);
            ui->Net_addrPortList->setItem(i, 6, tmpItem);
            tmpItem = new QTableWidgetItem();
            tmpItem->setFlags(tmpItem->flags() | Qt::ItemIsUserCheckable);
            tmpItem->setCheckState(Qt::Checked);
            ui->Net_addrPortList->setItem(i, 7, tmpItem);
        }
        ui->Net_addrPortList->blockSignals(false);
    }
    emit clientCountChanged();
}

void DeviceTab::Net_onDeleteButtonClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    Connection::Type currType = m_connection->type();
    QVariant var = btn->property("ItemId");
    if(!var.isValid())
        return;
    int itemID = var.toInt();
    if(currType == Connection::TCP_Client)
    {
        m_TCPClientHistory.removeAt(itemID);
        syncTCPClientPreference();
        showNetArgumentHistory(m_TCPClientHistory, currType);
    }
    else if(currType == Connection::UDP)
    {
        m_UDPHistory.removeAt(itemID);
        syncUDPPreference();
        showNetArgumentHistory(m_UDPHistory, currType);
    }

}

bool DeviceTab::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->SP_baudRateBox && event->type() == QEvent::FocusOut)
    {
        // like editFinished()
        QComboBox* box = qobject_cast<QComboBox*>(watched);
        qint32 baud = box->currentText().toInt();
        if(baud != 0 && baud != m_connection->SP_baudRate() && m_connection->type() == Connection::SerialPort && m_connection->isConnected())
        {
            if(m_connection->SP_setBaudRate(baud))
            {
                saveSPPreference(m_connection->getSerialPortArgument());
                emit argumentChanged();
            }
        }
    }
    else if(watched == ui->BLECentralListSplitter->handle(1))
    {
        // double click the handle to reset the size
        if(event->type() == QEvent::MouseButtonDblClick)
        {
            QList<int> newSizes = ui->BLECentralListSplitter->sizes(); // 2 elements
            newSizes[1] += newSizes[0];
            newSizes[0] = newSizes[1] * 0.5;
            newSizes[1] -= newSizes[0];
            ui->BLECentralListSplitter->setSizes(newSizes);
        }
        // save layout when mouse button is released
        else if(event->type() == QEvent::MouseButtonRelease)
        {
            QList<int> sizes = ui->BLECentralListSplitter->sizes(); // 2 elements
            double ratio = (double)sizes[0] / (sizes[0] + sizes[1]);
            settings->beginGroup("SerialTest_Connect");
            settings->setValue("BLEC_SplitRatio", ratio);
            settings->endGroup();
        }
        else if(event->type() == QEvent::Show && !m_isBLECLoaded)
        {
            // ui->BLECentralListSplitter->sizes() will return all 0 if the widgets are invisible
            // the widgets are visible after this event happens
            settings->beginGroup("SerialTest_Connect");
            QList<int> newSizes = ui->BLECentralListSplitter->sizes();
            double ratio = settings->value("BLEC_SplitRatio", 0.5).toDouble();
            settings->endGroup();

            newSizes[1] += newSizes[0];
            newSizes[0] = newSizes[1] * ratio;
            newSizes[1] -= newSizes[0];

            ui->BLECentralListSplitter->setSizes(newSizes);
            m_isBLECLoaded = true;
        }
    }
    return false;
}

void DeviceTab::on_openButton_clicked()
{
    Connection::Type currType = m_connection->type();
    if(currType == Connection::SerialPort)
    {
        if(m_connection->isConnected())
        {
            QMessageBox::warning(this, tr("Error"), tr("The port has been opened."));
            return;
        }
        Connection::SerialPortArgument arg;
        arg.name = ui->SP_portNameBox->currentText();
        arg.baudRate = ui->SP_baudRateBox->currentText().toInt();
        arg.dataBits = (QSerialPort::DataBits)ui->SP_dataBitsBox->currentData().toInt();
        arg.stopBits = (QSerialPort::StopBits)ui->SP_stopBitsBox->currentData().toInt();
        arg.parity = (QSerialPort::Parity)ui->SP_parityBox->currentData().toInt();
        arg.flowControl = (QSerialPort::FlowControl)ui->SP_flowControlBox->currentData().toInt();
        const SP_ID spid = SP_ID(QSerialPortInfo(arg.name));
        if(spid && !SP_hasDuplicateID(spid))
            arg.id = spid.toString();
        else
            arg.id = arg.name;
        m_connection->setArgument(arg);
        m_connection->open();
    }
    else if(currType == Connection::BT_Client)
    {
        if(m_connection->isConnected())
        {
            QMessageBox::warning(this, tr("Error"), tr("The device has already connected."));
            return;
        }
        else if(m_connection->state() == Connection::Connecting)
        {
            // force disconnect
            m_connection->close(true);
        }
        Connection::BTArgument arg;
        arg.localAdapterAddress = QBluetoothAddress(ui->BTClient_adapterBox->currentData().toString());
        arg.deviceAddress = QBluetoothAddress(ui->BTClient_targetAddrBox->currentText());
        if(ui->BTClient_serviceUUIDBox->isChecked() && !ui->BTClient_serviceUUIDEdit->text().isEmpty())
            arg.RxServiceUUID = String2UUID(ui->BTClient_serviceUUIDEdit->text());
        else
            arg.RxServiceUUID = QBluetoothUuid::SerialPort;
        m_connection->setArgument(arg);
        m_connection->open();

        settings->beginGroup(m_historyPrefix["BTClient"]);
        if(arg.RxServiceUUID == QBluetoothUuid::SerialPort)
        {
            settings->setValue("UserSpecifiedServiceUUID", false);
        }
        else
        {
            // Update "ServiceUUID" only if the service UUID is not SerialPort
            settings->setValue("UserSpecifiedServiceUUID", ui->BTClient_serviceUUIDBox->isChecked());
            settings->setValue("ServiceUUID", arg.RxServiceUUID);
        }
        settings->endGroup();
    }
    else if(currType == Connection::BT_Server)
    {
        if(m_connection->state() != Connection::Unconnected)
        {
            QMessageBox::warning(this, tr("Error"), tr("The server is already running."));
            return;
        }
        Connection::BTArgument arg;
        arg.localAdapterAddress = QBluetoothAddress(ui->BTServer_adapterBox->currentData().toString());
        arg.serverServiceName = ui->BTServer_serviceNameEdit->text();
        m_connection->setArgument(arg);
        m_connection->open();

        settings->beginGroup(m_historyPrefix["BTServer"]);
        settings->setValue("LastServiceName", arg.serverServiceName);
        settings->endGroup();
    }
    else if(currType == Connection::BLE_Central)
    {
        if(m_connection->isConnected())
        {
            QMessageBox::warning(this, tr("Error"), tr("The device has already connected."));
            return;
        }
        else if(m_connection->state() == Connection::Connecting)
        {
            m_connection->close(true);
        }
        if(m_BLEController != nullptr)
            m_BLEController->disconnectFromDevice();
        Connection::BTArgument arg;
        arg.localAdapterAddress = QBluetoothAddress(ui->BLEC_adapterBox->currentData().toString());
        arg.deviceAddress = QBluetoothAddress(ui->BLEC_targetAddrBox->currentText());
        arg.RxServiceUUID = String2UUID(ui->BLEC_RxServiceUUIDBox->currentText());
        arg.RxCharacteristicUUID = String2UUID(ui->BLEC_RxCharacteristicUUIDBox->currentText());
        arg.TxServiceUUID = String2UUID(ui->BLEC_TxServiceUUIDBox->currentText());
        arg.TxCharacteristicUUID = String2UUID(ui->BLEC_TxCharacteristicUUIDBox->currentText());
        m_connection->setArgument(arg);
        m_connection->open();
    }
    else if(currType == Connection::TCP_Client)
    {
        if(m_connection->isConnected())
        {
            QMessageBox::warning(this, tr("Error"), tr("The client has already connected to the server."));
            return;
        }
        else if(m_connection->state() == Connection::Connecting)
        {
            // force disconnect
            m_connection->close(true);
        }
        Connection::NetworkArgument arg;
        if(ui->Net_localAddrBox->currentText() != m_autoLocalAddress)
            arg.localAddress = QHostAddress(ui->Net_localAddrBox->currentText());
        else
            arg.localAddress = QHostAddress::Any;
        arg.localPort = ui->Net_localPortEdit->text().toUInt();
        arg.remoteName = ui->Net_remoteAddrEdit->text();
        arg.remotePort = ui->Net_remotePortEdit->text().toUInt();
        m_connection->setArgument(arg);
        m_connection->open();
    }
    else if(currType == Connection::TCP_Server)
    {
        if(m_connection->state() != Connection::Unconnected)
        {
            QMessageBox::warning(this, tr("Error"), tr("The server is already running."));
            return;
        }
        Connection::NetworkArgument arg;
        if(ui->Net_localAddrBox->currentText() != m_anyLocalAddress)
            arg.localAddress = QHostAddress(ui->Net_localAddrBox->currentText());
        else
            arg.localAddress = QHostAddress::Any;
        arg.localPort = ui->Net_localPortEdit->text().toUInt();
        m_connection->setArgument(arg);
        m_connection->open();

        settings->beginGroup(m_historyPrefix["TCPServer"]);
        settings->setValue("LastPort", arg.localPort);
        settings->endGroup();
    }
    else if(currType == Connection::UDP)
    {
        if(m_connection->state() != Connection::Unconnected)
        {
            QMessageBox::warning(this, tr("Error"), tr("The socket has already bound to a port."));
            return;
        }
        Connection::NetworkArgument arg;
        if(ui->Net_localAddrBox->currentText() != m_anyLocalAddress)
            arg.localAddress = QHostAddress(ui->Net_localAddrBox->currentText());
        else
            arg.localAddress = QHostAddress::Any;
        arg.localPort = ui->Net_localPortEdit->text().toUInt();
        arg.remoteName = ui->Net_remoteAddrEdit->text();
        arg.remotePort = ui->Net_remotePortEdit->text().toUInt();
        m_connection->setArgument(arg);
        m_connection->open();
    }
}

void DeviceTab::on_closeButton_clicked()
{
    m_connection->close();
}

DeviceTab::SP_ID DeviceTab::SP_getPortID(int rowInList)
{
    quint16 vid = ui->SP_portList->item(rowInList, 6)->data(Qt::UserRole).toUInt();
    quint16 pid = ui->SP_portList->item(rowInList, 7)->data(Qt::UserRole).toUInt();
    return SP_ID(vid, pid, ui->SP_portList->item(rowInList, 3)->text());
}

bool DeviceTab::SP_hasDuplicateID(int rowInList)
{
    return SP_hasDuplicateID(SP_getPortID(rowInList));
}

bool DeviceTab::SP_hasDuplicateID(const SP_ID& spid)
{
    int counter = 0;
    if(!spid) // don't handle invalid SP_ID
        return false;
    for(int i = 0; i < ui->SP_portList->rowCount(); i++)
    {
        if(spid.matches(SP_getPortID(i)) == 2) // exactly the same one
        {
            counter++;
            if(counter >= 2)
                return true;
        }
    }
    return false;
}

int DeviceTab::SP_getMatchedHistoryIndex(int rowInList)
{
    int matchedIndex = -1;
    const QString targetPortName = ui->SP_portList->item(rowInList, 0)->text();
    const SP_ID targetSPID = SP_getPortID(rowInList);
    const QString targetSPIDString = targetSPID.toString();

    if(m_SPArgHistoryIndex.contains(targetSPIDString) && !SP_hasDuplicateID(rowInList))
        matchedIndex = m_SPArgHistoryIndex[targetSPIDString];
    else if(m_SPArgHistoryIndex.contains(targetPortName))
        matchedIndex = m_SPArgHistoryIndex[targetPortName];
    else
    {
        // reversed order, because the last one in the list is also the latest one.
        // for every history item, if its id partially matches the targetSPID, or its name equals to targetPortName, then it is used.
        for(int i = m_SPArgHistory.size() - 1; i >= 0; i--)
        {
            if(targetSPID && targetSPID.matches(m_SPArgHistory[i].id)) // don't handle invalid SP_ID
            {
                matchedIndex = i;
                break;
            }
            if(targetPortName == m_SPArgHistory[i].name)
            {
                matchedIndex = i;
                break;
            }
        }
    }
    return matchedIndex;
}

void DeviceTab::onTargetListCellClicked(int row, int column)
{
    Q_UNUSED(column)
    if(m_connection == nullptr)
        return;
    Connection::Type currType = m_connection->type();
    if(currType == Connection::SerialPort)
    {
        // for default config
        ui->SP_portNameBox->setCurrentIndex(row);

        int historyIndex = SP_getMatchedHistoryIndex(row);

        // don't override portName
        if(historyIndex != -1)
            loadSPPreference(m_SPArgHistory[historyIndex], false);
    }
    else if(currType == Connection::BT_Client)
    {
        ui->BTClient_targetAddrBox->setCurrentIndex(row);
    }
    else if(currType == Connection::BLE_Central)
    {
        if(sender() == ui->BLEC_deviceList)
        {
            ui->BLEC_targetAddrBox->setCurrentIndex(row);
            ui->BLEC_currAddrBox->setCurrentIndex(row);
        }
    }
    else if(currType == Connection::TCP_Client || currType == Connection::UDP)
    {
        ui->Net_localAddrBox->setCurrentText(ui->Net_addrPortList->item(row, 1)->text());
        ui->Net_localPortEdit->setText(ui->Net_addrPortList->item(row, 2)->text());
        ui->Net_remoteAddrEdit->setText(ui->Net_addrPortList->item(row, 3)->text());
        ui->Net_remotePortEdit->setText(ui->Net_addrPortList->item(row, 4)->text());
    }
}

// platform specific
// **********************************************************************************************************************************************

void DeviceTab::saveTCPClientPreference(const Connection::NetworkArgument& arg)
{
    int id;
    Connection::NetworkArgument newArg;
    id = m_TCPClientHistory.indexOf(arg);
    if(id != -1)
        newArg = m_TCPClientHistory.takeAt(id);
    else
        newArg = arg;
    m_TCPClientHistory.append(newArg);

    syncTCPClientPreference();
    showNetArgumentHistory(m_TCPClientHistory, Connection::TCP_Client);
}

void DeviceTab::syncTCPClientPreference()
{
    int num;
    num = (m_TCPClientHistory.length() > m_maxHistoryNum) ? (m_TCPClientHistory.length() - m_maxHistoryNum) : 0;
    for(int i = 0; i < num; i++)
        m_TCPClientHistory.removeFirst();

    settings->beginWriteArray(m_historyPrefix["TCPClient"], m_TCPClientHistory.length());
    for(int i = 0; i < m_TCPClientHistory.length(); i++)
    {
        settings->setArrayIndex(i);
        settings->setValue("Arg", Connection::arg2StringList(m_TCPClientHistory[i]));
    }
    settings->endArray();
}

void DeviceTab::saveUDPPreference(const Connection::NetworkArgument& arg)
{
    int id;
    Connection::NetworkArgument newArg;
    id = m_UDPHistory.indexOf(arg);
    if(id != -1)
        newArg = m_UDPHistory.takeAt(id);
    else
        newArg = arg;
    m_UDPHistory.append(newArg);

    syncUDPPreference();
    showNetArgumentHistory(m_UDPHistory, Connection::UDP);
}

void DeviceTab::syncUDPPreference()
{
    int num;
    num = (m_UDPHistory.length() > m_maxHistoryNum) ? (m_UDPHistory.length() - m_maxHistoryNum) : 0;
    for(int i = 0; i < num; i++)
        m_UDPHistory.removeFirst();

    settings->beginWriteArray(m_historyPrefix["UDP"], m_UDPHistory.length());
    for(int i = 0; i < m_UDPHistory.length(); i++)
    {
        settings->setArrayIndex(i);
        settings->setValue("Arg", Connection::arg2StringList(m_UDPHistory[i]));
    }
    settings->endArray();
}

void DeviceTab::saveSPPreference(const Connection::SerialPortArgument& arg)
{
    int removeNum = 0;

    // remove existing one
    // arg.id can be portName or <VID>-<PID>[-<serialNumber>]
    //
    // TODO:
    // restore the mapping from arg.id to arg
    // (the mapping is changed in onTargetListCellClicked())
    // (IDK if this TODO is still necessary)
    if(m_SPArgHistoryIndex.contains(arg.id))
    {
        int removedId;
        m_SPArgHistory.removeAt(m_SPArgHistoryIndex[arg.id]);
        removedId = m_SPArgHistoryIndex.take(arg.id);
        // update index
        for(auto it = m_SPArgHistoryIndex.begin(); it != m_SPArgHistoryIndex.end(); ++it)
        {
            if(it.value() > removedId)
                it.value()--;
        }
    }

    // add arg as latest
    m_SPArgHistory.append(arg);
    m_SPArgHistoryIndex[arg.id] = m_SPArgHistory.length() - 1;

    // remove oldest to fit the size limit
    removeNum = (m_SPArgHistory.length() > m_maxHistoryNum) ? (m_SPArgHistory.length() - m_maxHistoryNum) : 0;
    if(removeNum)
    {
        for(int i = 0; i < removeNum; i++)
            m_SPArgHistory.removeFirst();
        // Just update the index rather than rebuild it
        for(auto it = m_SPArgHistoryIndex.begin(); it != m_SPArgHistoryIndex.end();)
        {
            if(it.value() < removeNum)
                it = m_SPArgHistoryIndex.erase(it);
            else
            {
                it.value() -= removeNum;
                ++it;
            }
        }
    }

    // save
    settings->beginWriteArray(m_historyPrefix["SP"], m_SPArgHistory.length());
    for(int i = 0; i < m_SPArgHistory.length(); i++)
    {
        settings->setArrayIndex(i);
        settings->setValue("Arg", Connection::arg2StringList(m_SPArgHistory[i]));
    }
    settings->endArray();
}

void DeviceTab::loadSPPreference(const Connection::SerialPortArgument& arg, bool loadPortName)
{
    // on_SP_XXBox_currentIndexChanged() is used to update serial port argument on the fly
    // prevent triggering them when setting arguments programmatically
    ui->SP_baudRateBox->blockSignals(true);
    ui->SP_dataBitsBox->blockSignals(true);
    ui->SP_stopBitsBox->blockSignals(true);
    ui->SP_parityBox->blockSignals(true);
    ui->SP_flowControlBox->blockSignals(true);

    if(loadPortName)
        ui->SP_portNameBox->setCurrentText(arg.name);
    ui->SP_baudRateBox->setEditText(QString::number(arg.baudRate));
    ui->SP_dataBitsBox->setCurrentIndex(ui->SP_dataBitsBox->findData(arg.dataBits));
    ui->SP_stopBitsBox->setCurrentIndex(ui->SP_stopBitsBox->findData(arg.stopBits));
    ui->SP_parityBox->setCurrentIndex(ui->SP_parityBox->findData(arg.parity));
    ui->SP_flowControlBox->setCurrentIndex(ui->SP_flowControlBox->findData(arg.flowControl));

    ui->SP_baudRateBox->blockSignals(false);
    ui->SP_dataBitsBox->blockSignals(false);
    ui->SP_stopBitsBox->blockSignals(false);
    ui->SP_parityBox->blockSignals(false);
    ui->SP_flowControlBox->blockSignals(false);
}

void DeviceTab::loadNetPreference(const Connection::NetworkArgument& arg, Connection::Type type)
{
    // block currentIndexChanged()
    ui->Net_localAddrBox->blockSignals(true);
    const QString& anyAddr = (type == Connection::TCP_Client) ? m_autoLocalAddress : m_anyLocalAddress;
    ui->Net_localAddrBox->setCurrentText(arg.localAddress == QHostAddress::Any ? anyAddr : arg.localAddress.toString());
    ui->Net_localPortEdit->setText(QString::number(arg.localPort));
    ui->Net_remoteAddrEdit->setText(arg.remoteName);
    ui->Net_remotePortEdit->setText(QString::number(arg.remotePort));
    ui->Net_localAddrBox->blockSignals(false);
}

void DeviceTab::showNetArgumentHistory(const QList<Connection::NetworkArgument> &argList, Connection::Type type)
{
    ui->Net_addrPortList->setRowCount(0);
    ui->Net_addrPortList->setRowCount(argList.size());
    ui->Net_addrPortList->blockSignals(true); // avoid emitting cellChanged()
    const QString& anyAddr = (type == Connection::TCP_Client) ? m_autoLocalAddress : m_anyLocalAddress;
    int size = argList.size();
    for(int i = 0; i < size; i++)
    {
        // reversed order
        QTableWidgetItem* tmpItem;
        tmpItem = new QTableWidgetItem(argList[i].alias);
        tmpItem->setFlags(tmpItem->flags() | Qt::ItemIsEditable);
        ui->Net_addrPortList->setItem(size - i - 1, 0, tmpItem);
        tmpItem = new QTableWidgetItem((argList[i].localAddress == QHostAddress::Any) ? anyAddr : argList[i].localAddress.toString());
        tmpItem->setFlags(tmpItem->flags() & ~Qt::ItemIsEditable);
        ui->Net_addrPortList->setItem(size - i - 1, 1, tmpItem);
        tmpItem = new QTableWidgetItem(QString::number(argList[i].localPort));
        tmpItem->setFlags(tmpItem->flags() & ~Qt::ItemIsEditable);
        ui->Net_addrPortList->setItem(size - i - 1, 2, tmpItem);
        tmpItem = new QTableWidgetItem(argList[i].remoteName);
        tmpItem->setFlags(tmpItem->flags() & ~Qt::ItemIsEditable);
        ui->Net_addrPortList->setItem(size - i - 1, 3, tmpItem);
        tmpItem = new QTableWidgetItem(QString::number(argList[i].remotePort));
        tmpItem->setFlags(tmpItem->flags() & ~Qt::ItemIsEditable);
        ui->Net_addrPortList->setItem(size - i - 1, 4, tmpItem);
        QPushButton* deleteButton = new QPushButton;
        deleteButton->setText(tr("Delete"));
        deleteButton->setProperty("ItemId", i);
        connect(deleteButton, &QPushButton::clicked, this, &DeviceTab::Net_onDeleteButtonClicked);
        ui->Net_addrPortList->setIndexWidget(ui->Net_addrPortList->model()->index(size - i - 1, 5), deleteButton);
    }
    ui->Net_addrPortList->blockSignals(false);
}

void DeviceTab::BTdiscoverFinished()
{
    ui->refreshButton->setText(tr("Refresh"));
}

void DeviceTab::BTdeviceDiscovered(const QBluetoothDeviceInfo& device)
{
    QString address = device.address().toString();
    QString name = device.name();
    QString rssi = QString::number(device.rssi());
    Connection::Type currType = m_connection->type();
    QTableWidget* deviceList = (currType == Connection::BT_Client) ? ui->BTClient_deviceList : ui->BLEC_deviceList;
    int i;
    if(m_shownBTDevices.contains(address))
    {
        // just update rssi
        i = m_shownBTDevices[address];
        deviceList->setItem(i, 3, new QTableWidgetItem(rssi));
    }
    else
    {
        i = deviceList->rowCount();
        deviceList->setRowCount(i + 1);
        deviceList->setItem(i, 0, new QTableWidgetItem(name));
        deviceList->setItem(i, 1, new QTableWidgetItem(address));
        deviceList->setItem(i, 2, new QTableWidgetItem(tr("Discovered")));
        deviceList->setItem(i, 3, new QTableWidgetItem(rssi));
        if(currType == Connection::BT_Client)
        {
            ui->BTClient_targetAddrBox->addItem(address);
        }
        else
        {
            ui->BLEC_currAddrBox->addItem(address);
            ui->BLEC_targetAddrBox->addItem(address);
        }
        m_shownBTDevices[address] = i;
    }
    qDebug() << name
             << address
             << device.isValid()
             << device.rssi()
             << device.majorDeviceClass()
             << device.minorDeviceClass()
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
             << device.serviceClasses();
#else
             << device.serviceClasses()
             << device.manufacturerData();
#endif
}


void DeviceTab::on_typeBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    Connection::Type newType;
    bool result;
    newType = ui->typeBox->currentData().value<Connection::Type>();
    if(m_connection == nullptr)
        return;
    result = m_connection->setType(newType);
    if(!result)
    {
        if(m_connection->state() != Connection::Unconnected)
            QMessageBox::warning(this, tr("Error"), tr("Please close the current connection first."));
        else
            QMessageBox::warning(this, tr("Error"), tr("Unsupported interface."));
        ui->typeBox->blockSignals(true);
        ui->typeBox->setCurrentIndex(m_connection->type());
        ui->typeBox->blockSignals(false);
        return;
    }
    // stop searching
    BTClient_discoveryAgent->stop();
    BTdiscoverFinished();
    if(newType == Connection::SerialPort)
    {
        ui->targetListStack->setCurrentWidget(ui->SPListPage);
        ui->argsStack->setCurrentWidget(ui->SPArgsPage);
    }
    else if(newType == Connection::BT_Client)
    {
        ui->targetListStack->setCurrentWidget(ui->BTClientListPage);
        ui->argsStack->setCurrentWidget(ui->BTClientArgsPage);
        updateBTAdapterList();
    }
    else if(newType == Connection::BT_Server)
    {
        ui->targetListStack->setCurrentWidget(ui->BTServerListPage);
        ui->argsStack->setCurrentWidget(ui->BTServerArgsPage);
        updateBTAdapterList();
    }
    else if(newType == Connection::BLE_Central)
    {
        ui->targetListStack->setCurrentWidget(ui->BLECentralListPage);
        ui->argsStack->setCurrentWidget(ui->BLECentralArgsPage);
        updateBTAdapterList();
    }
    else if(newType == Connection::TCP_Client)
    {
        ui->Net_addrPortList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
        ui->Net_addrPortList->setColumnCount(6);
        ui->Net_addrPortList->setRowCount(0);
        ui->Net_localAddrBox->show();
        ui->Net_localAddrBox->setEditable(false);
        ui->Net_localPortEdit->show();
        ui->Net_remoteAddrLabel->show();
        ui->Net_remoteAddrLabel->setText(tr("Remote Address/Name:"));
        ui->Net_remotePortLabel->show();
        ui->Net_remoteAddrEdit->show();
        ui->Net_remotePortEdit->show();
        ui->Net_tipLabel->hide();
        ui->targetListStack->setCurrentWidget(ui->NetListPage);
        ui->argsStack->setCurrentWidget(ui->NetArgsPage);

        updateNetInterfaceList();
        if(!m_TCPClientHistory.isEmpty())
        {
            loadNetPreference(m_TCPClientHistory.last(), newType);
            showNetArgumentHistory(m_TCPClientHistory, newType);
        }
    }
    else if(newType == Connection::TCP_Server)
    {
        ui->Net_addrPortList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->Net_addrPortList->setColumnCount(8);
        ui->Net_addrPortList->setHorizontalHeaderItem(6, new QTableWidgetItem(tr("Receive")));
        ui->Net_addrPortList->setHorizontalHeaderItem(7, new QTableWidgetItem(tr("Send")));
        ui->Net_addrPortList->setRowCount(0);
        ui->Net_localAddrBox->show();
        ui->Net_localAddrBox->setEditable(false);
        ui->Net_localPortEdit->show();
        ui->Net_remoteAddrLabel->hide();
        ui->Net_remotePortLabel->hide();
        ui->Net_remoteAddrEdit->hide();
        ui->Net_remotePortEdit->hide();
        ui->Net_tipLabel->hide();
        ui->targetListStack->setCurrentWidget(ui->NetListPage);
        ui->argsStack->setCurrentWidget(ui->NetArgsPage);

        updateNetInterfaceList();
        qint16 TCPServerPort = 0;
        settings->beginGroup(m_historyPrefix["TCPServer"]);
        TCPServerPort = settings->value("LastPort", 0).toUInt();
        settings->endGroup();
        ui->Net_localPortEdit->setText(QString::number(TCPServerPort));
    }
    else if(newType == Connection::UDP)
    {
        ui->Net_addrPortList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
        ui->Net_addrPortList->setColumnCount(6);
        ui->Net_addrPortList->setRowCount(0);
        ui->Net_localAddrBox->show();
        ui->Net_localAddrBox->setEditable(true); // support multicast address and broadcast address
        ui->Net_localPortEdit->show();
        ui->Net_remoteAddrLabel->show();
        ui->Net_remoteAddrLabel->setText(tr("Remote Address:"));
        ui->Net_remotePortLabel->show();
        ui->Net_remoteAddrEdit->show();
        ui->Net_remotePortEdit->show();
        ui->Net_tipLabel->show();
        ui->targetListStack->setCurrentWidget(ui->NetListPage);
        ui->argsStack->setCurrentWidget(ui->NetArgsPage);
        updateNetInterfaceList();
        if(!m_UDPHistory.isEmpty())
        {
            loadNetPreference(m_UDPHistory.last(), newType);
            showNetArgumentHistory(m_UDPHistory, newType);
        }
    }
    emit connTypeChanged(newType);
    refreshTargetList();
}

void DeviceTab::on_refreshButton_clicked()
{
    getAvailableTypes();
    if(ui->refreshButton->text() == tr("Refresh"))
        refreshTargetList();
    else
    {
        BTClient_discoveryAgent->stop();
        BTdiscoverFinished();
    }
}

void DeviceTab::on_BTClient_adapterBox_activated(int index)
{
    // This function is actually useless.
    // According to the source code of Qt Connectivity (v5.9.0, v5.15.2 and v6.7.0),
    // the constructor of QBluetoothDeviceDiscoveryAgent only checks if the adapter address exists
    // in QBluetoothLocalDevice::allDevices(). It ignores null address and won't specify which adapter to be used.
    Q_UNUSED(index)
    ui->BTClient_localAddrLabel->setText(ui->BTClient_adapterBox->currentData().toString());
    setBTClientDiscoveryAgent(QBluetoothAddress());
#ifdef Q_OS_ANDROID
    // invalid MAC address, ignore
    ui->BTClient_localAddrLabel->setHidden(ui->BTClient_localAddrLabel->text() == "02:00:00:00:00:00");
#endif
}

void DeviceTab::setBTClientDiscoveryAgent(QBluetoothAddress adapterAddress)
{
    if(BTClient_discoveryAgent != nullptr)
    {
        disconnect(BTClient_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceTab::BTdeviceDiscovered);
        disconnect(BTClient_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &DeviceTab::BTdiscoverFinished);
        disconnect(BTClient_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &DeviceTab::BTdiscoverFinished);
        BTClient_discoveryAgent->deleteLater();
    }
    BTClient_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(adapterAddress, this);
    connect(BTClient_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceTab::BTdeviceDiscovered);
    connect(BTClient_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &DeviceTab::BTdiscoverFinished);
    connect(BTClient_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &DeviceTab::BTdiscoverFinished);

}

void DeviceTab::on_BTServer_serviceNameEdit_editingFinished()
{
    if(ui->BTServer_serviceNameEdit->text().isEmpty())
        ui->BTServer_serviceNameEdit->setText(tr("SerialTest_BT"));
}


void DeviceTab::on_BTServer_adapterBox_activated(int index)
{
    Q_UNUSED(index)
    ui->BTServer_localAddrLabel->setText(ui->BTServer_adapterBox->currentData().toString());
#ifdef Q_OS_ANDROID
    // invalid MAC address, ignore
    ui->BTServer_localAddrLabel->setHidden(ui->BTServer_localAddrLabel->text() == "02:00:00:00:00:00");
#endif
}


void DeviceTab::Net_onRemoteChanged()
{
    if(m_connection->type() == Connection::UDP && m_connection->isConnected())
    {
        bool convOk = false;
        quint16 port = ui->Net_remotePortEdit->text().toUInt(&convOk);
        QHostAddress addr; // test validity
        if(convOk && addr.setAddress(ui->Net_remoteAddrEdit->text()))
        {
            m_connection->UDP_setRemote(ui->Net_remoteAddrEdit->text(), port);
            // get raw user input
            saveUDPPreference(m_connection->getNetworkArgument(false, false));
            emit argumentChanged();
        }
    }
}

void DeviceTab::on_Net_localAddrBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(ui->Net_remoteAddrEdit->isHidden() || ui->Net_localAddrBox->currentText() == m_autoLocalAddress || ui->Net_localAddrBox->currentText() == m_anyLocalAddress)
        return;
    // fill the remoteAddrEdit if user doesn't specify it
    if(ui->Net_remoteAddrEdit->text().isEmpty() || ui->Net_localAddrBox->findText(ui->Net_remoteAddrEdit->text(), Qt::MatchExactly) != -1) // case insensitive
        ui->Net_remoteAddrEdit->setText(ui->Net_localAddrBox->currentText());
}


void DeviceTab::on_SP_baudRateBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setBaudRate(ui->SP_baudRateBox->currentText().toInt()))
    {
        saveSPPreference(m_connection->getSerialPortArgument());
        emit argumentChanged();
    }
}


void DeviceTab::on_SP_dataBitsBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setDataBits((QSerialPort::DataBits)ui->SP_dataBitsBox->currentData().toInt()))
    {
        saveSPPreference(m_connection->getSerialPortArgument());
        emit argumentChanged();
    }
}


void DeviceTab::on_SP_stopBitsBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setStopBits((QSerialPort::StopBits)ui->SP_stopBitsBox->currentData().toInt()))
    {
        saveSPPreference(m_connection->getSerialPortArgument());
        emit argumentChanged();
    }
}


void DeviceTab::on_SP_parityBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setParity((QSerialPort::Parity)ui->SP_parityBox->currentData().toInt()))
    {
        saveSPPreference(m_connection->getSerialPortArgument());
        emit argumentChanged();
    }
}


void DeviceTab::on_SP_flowControlBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setFlowControl((QSerialPort::FlowControl)ui->SP_flowControlBox->currentData().toInt()))
    {
        saveSPPreference(m_connection->getSerialPortArgument());
        emit argumentChanged();
    }
}


void DeviceTab::on_BLEC_connectButton_clicked()
{
    if(m_BLEController != nullptr)
    {
        m_BLEController->disconnectFromDevice();
        m_BLEController->deleteLater();
        m_BLEController = nullptr;
    }
    if(ui->BLEC_connectButton->text() == tr("Connect"))
    {
        // stage 1: connect to device
        ui->BLEC_UUIDList->clear();
        ui->BLEC_connectButton->setText(tr("Disconnect"));
        ui->BLEC_currAddrLabel->setText(ui->BLEC_currAddrBox->currentText());
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        m_BLEController = new QLowEnergyController(QBluetoothAddress(ui->BLEC_currAddrBox->currentText()), QBluetoothAddress(ui->BLEC_adapterBox->currentData().toString()));
#else
        m_BLEController = QLowEnergyController::createCentral(QBluetoothAddress(ui->BLEC_currAddrBox->currentText()), QBluetoothAddress(ui->BLEC_adapterBox->currentData().toString()));
#endif
        connect(m_BLEController, &QLowEnergyController::connected, m_BLEController, &QLowEnergyController::discoverServices);
        connect(m_BLEController, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), [ = ](QLowEnergyController::Error newError)
        {
            qDebug() << newError;
        });

        connect(m_BLEController, &QLowEnergyController::serviceDiscovered, this, &DeviceTab::BLEC_onRootServiceDiscovered);

        // stash user input
        auto lastServiceList = m_discoveredBLEServices.keys();
        QBluetoothUuid lastService;
        lastService = String2UUID(ui->BLEC_RxServiceUUIDBox->currentText());
        ui->BLEC_RxServiceUUIDBox->clear();
        if(!lastService.isNull() && !lastServiceList.contains(lastService))
            ui->BLEC_RxServiceUUIDBox->addItem(UUID2String(lastService));
        lastService = String2UUID(ui->BLEC_TxServiceUUIDBox->currentText());
        ui->BLEC_TxServiceUUIDBox->clear();
        if(!lastService.isNull() && !lastServiceList.contains(lastService))
            ui->BLEC_TxServiceUUIDBox->addItem(UUID2String(lastService));

        m_discoveredBLEServices.clear();
        m_BLEController->connectToDevice();
    }
    else
    {
        ui->BLEC_connectButton->setText(tr("Connect"));
    }

}

void DeviceTab::BLEC_onRootServiceDiscovered(const QBluetoothUuid& newService)
{
    BLEC_addService(newService);
}

void DeviceTab::BLEC_addService(const QBluetoothUuid& serviceUUID, QTreeWidgetItem* parentItem)
{
    QTreeWidgetItem* item = new QTreeWidgetItem;
    QString UUIDString = UUID2String(serviceUUID);
    auto service = m_BLEController->createServiceObject(serviceUUID);
    item->setText(0, UUIDString);
    item->setText(1, tr("Service"));
    item->setText(3, service->serviceName());
    if(parentItem == nullptr)
        ui->BLEC_UUIDList->addTopLevelItem(item);
    else
        parentItem->addChild(item);

    m_discoveredBLEServices[serviceUUID] = item;
    connect(service, &QLowEnergyService::stateChanged, this, &DeviceTab::BLEC_onServiceDetailDiscovered);
    service->discoverDetails();

    if(ui->BLEC_RxServiceUUIDBox->findText(UUIDString, Qt::MatchExactly) == -1)
        ui->BLEC_RxServiceUUIDBox->addItem(UUIDString);
    if(ui->BLEC_TxServiceUUIDBox->findText(UUIDString, Qt::MatchExactly) == -1)
        ui->BLEC_TxServiceUUIDBox->addItem(UUIDString);
}

void DeviceTab::BLEC_addCharacteristic(const QLowEnergyCharacteristic& c, QTreeWidgetItem* parentItem)
{
    QTreeWidgetItem* item = new QTreeWidgetItem;
    QString UUIDString = UUID2String(c.uuid());
    item->setText(0, UUIDString);
    item->setText(1, tr("Characteristic"));
    item->setText(2, BLE_getCharacteristicPropertyString(c));
    item->setText(3, c.name());
    parentItem->addChild(item);

    if(ui->BLEC_RxServiceUUIDBox->currentText() == parentItem->text(0))
        ui->BLEC_RxCharacteristicUUIDBox->addItem(UUIDString);
    if(ui->BLEC_TxServiceUUIDBox->currentText() == parentItem->text(0))
        ui->BLEC_TxCharacteristicUUIDBox->addItem(UUIDString);

    const QList<QLowEnergyDescriptor> descriptors = c.descriptors();
    for(auto it = descriptors.cbegin(); it != descriptors.cend(); ++it)
        BLEC_addDescriptor(*it, item);
}

void DeviceTab::BLEC_addDescriptor(const QLowEnergyDescriptor& descriptor, QTreeWidgetItem* parentItem)
{
    QTreeWidgetItem* item = new QTreeWidgetItem;
    item->setText(0, UUID2String(descriptor.uuid()));
    item->setText(1, tr("Descriptor"));
    item->setText(3, descriptor.name());
    parentItem->addChild(item);
}

void DeviceTab::BLEC_onServiceDetailDiscovered(QLowEnergyService::ServiceState newState)
{
    auto service = qobject_cast<QLowEnergyService*>(sender());
    QTreeWidgetItem* parentItem = m_discoveredBLEServices[service->serviceUuid()];
    if(newState == QLowEnergyService::InvalidService)
        service->deleteLater();
    else if(newState == QLowEnergyService::ServiceDiscovered)
    {
        // add included services
        const QList<QBluetoothUuid> includedServices = service->includedServices();
        for(auto it = includedServices.cbegin(); it != includedServices.cend(); ++it)
        {
            // prevent circular dependency
            if(m_discoveredBLEServices.contains(*it))
                continue;
            BLEC_addService(*it, parentItem);
        }
        // add characteristics
        const QList<QLowEnergyCharacteristic> chars = service->characteristics();
        for(auto it = chars.cbegin(); it != chars.cend(); ++it)
            BLEC_addCharacteristic(*it, parentItem);

        service->deleteLater();
    }
}

QString DeviceTab::BLE_getCharacteristicPropertyString(const QLowEnergyCharacteristic& c)
{
    QString result;
    auto properties = c.properties();
    const QMap<QLowEnergyCharacteristic::PropertyType, QString> map =
    {
        {QLowEnergyCharacteristic::Broadcasting, tr("Broadcast")},
        {QLowEnergyCharacteristic::Read, tr("Read")},
        {QLowEnergyCharacteristic::WriteNoResponse, tr("WriteNoResponse")},
        {QLowEnergyCharacteristic::Write, tr("Write")},
        {QLowEnergyCharacteristic::Notify, tr("Notify")},
        {QLowEnergyCharacteristic::Indicate, tr("Indicate")},
        {QLowEnergyCharacteristic::WriteSigned, tr("WriteSigned")},
        {QLowEnergyCharacteristic::ExtendedProperty, tr("ExtendedProperty")}
    };
    for(auto it = map.cbegin(); it != map.cend(); ++it)
    {
        if(properties.testFlag(it.key()))
            result += ", " + it.value();
    }
    if(!result.isEmpty())
        result.remove(0, 2);
    return result;
}

void DeviceTab::on_BTServer_deviceList_cellChanged(int row, int column)
{
    // 3:Rx 4:Tx
    QTableWidget* widget = ui->BTServer_deviceList;
    if(column != 3 && column != 4)
        return;

    QBluetoothSocket* socket = (QBluetoothSocket*)widget->item(row, 0)->data(Qt::UserRole).value<quintptr>();
    m_connection->BTServer_setClientMode(socket, widget->item(row, 3)->checkState() == Qt::Checked, widget->item(row, 4)->checkState() == Qt::Checked);
}

void DeviceTab::on_Net_addrPortList_cellChanged(int row, int column)
{

    Connection::Type type = m_connection->type();
    if(type == Connection::TCP_Server && (column == 6 || column == 7))
    {
        // set client Rx/Tx enabled
        // 6:Rx 7:Tx
        QTableWidget* widget = ui->Net_addrPortList;
        QTcpSocket* socket = (QTcpSocket*)widget->item(row, 3)->data(Qt::UserRole).value<quintptr>();
        m_connection->TCPServer_setClientMode(socket, widget->item(row, 6)->checkState() == Qt::Checked, widget->item(row, 7)->checkState() == Qt::Checked);
    }
    else if(type == Connection::TCP_Client && column == 0)
    {
        // update alias
        // 0:alias
        QString newAlias = ui->Net_addrPortList->item(row, 0)->text();
        m_TCPClientHistory[m_TCPClientHistory.size() - 1 - row].alias = newAlias;
        syncTCPClientPreference();
    }
    else if(type == Connection::UDP && column == 0)
    {
        // update alias
        // 0:alias
        QString newAlias = ui->Net_addrPortList->item(row, 0)->text();
        m_UDPHistory[m_UDPHistory.size() - 1 - row].alias = newAlias;
        syncUDPPreference();
    }
}

void DeviceTab::on_BLEC_ServiceUUIDBox_currentTextChanged(const QString &arg1)
{
    QComboBox* serviceBox = qobject_cast<QComboBox*>(sender());
    QComboBox* characteristicBox = (serviceBox == ui->BLEC_RxServiceUUIDBox) ? ui->BLEC_RxCharacteristicUUIDBox : ui->BLEC_TxCharacteristicUUIDBox;
    QBluetoothUuid currServiceUUID = String2UUID(arg1);
    QString stashedUUIDString;
    if(currServiceUUID.isNull())
        return;
    // don't use arg1 there, useUUID2String(currServiceUUID)
    auto itemList = ui->BLEC_UUIDList->findItems(UUID2String(currServiceUUID), Qt::MatchExactly);
    if(itemList.isEmpty())
        return;

    // stash user input
    QBluetoothUuid currCharacteristicUUID = String2UUID(characteristicBox->currentText());
    if(!currCharacteristicUUID.isNull() && characteristicBox->findText(UUID2String(currCharacteristicUUID), Qt::MatchExactly) == -1)
    {
        stashedUUIDString = UUID2String(currCharacteristicUUID);
    }
    characteristicBox->clear();

    auto serviceItem = itemList[0];
    for(int i = 0; i < serviceItem->childCount(); i++)
    {
        QString UUIDString = serviceItem->child(i)->text(0);
        characteristicBox->addItem(UUIDString);
    }
    if(!stashedUUIDString.isEmpty())
        characteristicBox->setCurrentText(stashedUUIDString);
}

QBluetoothUuid DeviceTab::String2UUID(const QString& string)
{
    bool ok;
    quint32 val = string.toUInt(&ok, 16);
    if(ok)
        return QBluetoothUuid(val);
    QByteArray data = QByteArray::fromHex(string.toLatin1());
    if(data.length() == 2 || data.length() == 4)
        val = data.toHex().toUInt(&ok, 16);
    if(ok)
        return QBluetoothUuid(val);
    return QBluetoothUuid(string);
}

QString DeviceTab::UUID2String(const QBluetoothUuid& UUID)
{
    bool ok;
    QString result = QString("%1").arg(UUID.toUInt32(&ok), UUID.minimumSize() * 2, 16, QLatin1Char('0'));
    if(ok)
        return result;
    else
        return UUID.toString();
}

DeviceTab::SP_ID::SP_ID(const QString &str)
{
    bool isOk = false;
    m_vid = str.section('-', 0, 0).toUInt(&isOk);
    if(!isOk)
        return;
    m_pid = str.section('-', 1, 1).toUInt(&isOk);
    if(!isOk)
    {
        m_vid = 0; // invalid
        return;
    }
    m_serialNumber = str.section('-', 2);
}

QString DeviceTab::SP_ID::toString() const
{
    // if hasXXIdentifier==false, xxID==0
    // even if hasVendorIdentifier==true && VID==0, the VID is invalid in real world
    if(!*this)
        return QString();
    QString id = QString::number(m_vid) + "-" + QString::number(m_pid);
    if(!m_serialNumber.isEmpty())
        id += "-" + m_serialNumber;
    return id;
}

quint8 DeviceTab::SP_ID::matches(const SP_ID &id) const
{
    // (bool)(a.matches(b)) means a can use b's arguments
    // 0: unmatch 1: match 2: the same
    if(this->m_vid == id.m_vid && this->m_pid == id.m_pid)
        return (this->m_serialNumber == id.m_serialNumber) ? 2 : 1;
    else
        return 0;
}

DeviceTab::SP_ID::operator bool() const
{
    return (m_vid != 0 || m_pid != 0 || !m_serialNumber.isEmpty());
}

void DeviceTab::on_BTClient_serviceUUIDBox_clicked()
{
    bool userSpecifiedUUID = ui->BTClient_serviceUUIDBox->isChecked();
    ui->BTClient_serviceUUIDEdit->setVisible(userSpecifiedUUID);
    ui->BTClient_tipLabel->setVisible(!userSpecifiedUUID);
}

void DeviceTab::setTouchScroll(bool enabled)
{
    if(enabled)
    {

        QScroller::grabGesture(ui->BLEC_argsScrollArea);
        QScroller::grabGesture(ui->Net_argsScrollArea);
        QScroller::grabGesture(ui->SP_portList);
        QScroller::grabGesture(ui->BTServer_deviceList);
        QScroller::grabGesture(ui->BTClient_deviceList);
        QScroller::grabGesture(ui->BLEC_deviceList);
        QScroller::grabGesture(ui->BLEC_UUIDList);
        QScroller::grabGesture(ui->Net_addrPortList);
    }
    else
    {

        QScroller::ungrabGesture(ui->BLEC_argsScrollArea);
        QScroller::ungrabGesture(ui->Net_argsScrollArea);
        QScroller::ungrabGesture(ui->SP_portList);
        QScroller::ungrabGesture(ui->BTServer_deviceList);
        QScroller::ungrabGesture(ui->BTClient_deviceList);
        QScroller::ungrabGesture(ui->BLEC_deviceList);
        QScroller::ungrabGesture(ui->BLEC_UUIDList);
        QScroller::ungrabGesture(ui->Net_addrPortList);

    }
}
