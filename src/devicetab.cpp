#include "devicetab.h"
#include "ui_devicetab.h"
#include "util.h"

#include <QDebug>
#include <QMessageBox>
#include <QBluetoothUuid>
#include <QBluetoothLocalDevice>
#include <QNetworkInterface>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#endif

DeviceTab::DeviceTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceTab)
{
    ui->setupUi(this);

    connect(ui->SP_portList, &QTableWidget::cellClicked, this, &DeviceTab::onTargetListCellClicked);
    connect(ui->BTClient_deviceList, &QTableWidget::cellClicked, this, &DeviceTab::onTargetListCellClicked);
    connect(ui->BLEC_deviceList, &QTableWidget::cellClicked, this, &DeviceTab::onTargetListCellClicked);
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
            ui->SP_portList->setItem(i, 4, new QTableWidgetItem(ports[i].isNull() ? "Yes" : "No"));
            ui->SP_portList->setItem(i, 5, new QTableWidgetItem(ports[i].systemLocation()));
            ui->SP_portList->setItem(i, 6, new QTableWidgetItem(QString::number(ports[i].vendorIdentifier())));
            ui->SP_portList->setItem(i, 7, new QTableWidgetItem(QString::number(ports[i].productIdentifier())));

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
        ui->BLEC_currAddrBox->clear();
        ui->refreshButton->setText(tr("Searching..."));
#ifdef Q_OS_ANDROID
        getBondedTarget(true);
#endif
        BTClient_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    }
}

#ifdef Q_OS_ANDROID
void DeviceTab::getBondedTarget(bool isBLE)
{
    QAndroidJniEnvironment env;
    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.ACCESS_FINE_LOCATION");
    if(r == QtAndroid::PermissionResult::Denied)
    {
        QtAndroid::requestPermissionsSync(QStringList() << "android.permission.ACCESS_FINE_LOCATION");
        r = QtAndroid::checkPermission("android.permission.ACCESS_FINE_LOCATION");
        if(r == QtAndroid::PermissionResult::Denied)
        {
            qDebug() << "failed to request";
        }
    }
    qDebug() << "has permission";

    QAndroidJniObject helper("priv/wh201906/serialtest/BTHelper");
    qDebug() << "test:" << helper.callObjectMethod<jstring>("TestStr").toString();
    QAndroidJniObject array = helper.callObjectMethod("getBondedDevices", "(Z)[Ljava/lang/String;", isBLE);
    int arraylen = env->GetArrayLength(array.object<jarray>());
    qDebug() << "arraylen:" << arraylen;
    QTableWidget* deviceList = isBLE ? ui->BLEC_deviceList : ui->BTClient_deviceList;
    QComboBox* deviceBox = isBLE ? ui->BLEC_currAddrBox : ui->BTClient_targetAddrBox;
    deviceList->setRowCount(arraylen);
    for(int i = 0; i < arraylen; i++)
    {
        QString info = QAndroidJniObject::fromLocalRef(env->GetObjectArrayElement(array.object<jobjectArray>(), i)).toString();
        QString address = info.left(info.indexOf(' '));
        QString name = info.right(info.length() - info.indexOf(' ') - 1);
        qDebug() << address << name;
        deviceList->setItem(i, 0, new QTableWidgetItem(name));
        deviceList->setItem(i, 1, new QTableWidgetItem(address));
        deviceList->setItem(i, 2, new QTableWidgetItem(tr("Bonded")));
        deviceBox->addItem(address);
    }
}
#endif

void DeviceTab::initUI()
{
    ui->SP_portList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->BTClient_deviceList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->BTServer_deviceList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->BLEC_deviceList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->BLEC_UUIDList->header()->setSectionResizeMode(QHeaderView::Stretch);
    ui->Net_addrPortList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->SP_flowControlBox->addItem(tr("NoFlowControl"));
    ui->SP_flowControlBox->addItem(tr("HardwareControl"));
    ui->SP_flowControlBox->addItem(tr("SoftwareControl"));
    ui->SP_flowControlBox->setItemData(0, QSerialPort::NoFlowControl);
    ui->SP_flowControlBox->setItemData(1, QSerialPort::HardwareControl);
    ui->SP_flowControlBox->setItemData(2, QSerialPort::SoftwareControl);
    ui->SP_flowControlBox->setCurrentIndex(0);
    ui->SP_parityBox->addItem(tr("NoParity"));
    ui->SP_parityBox->addItem(tr("EvenParity"));
    ui->SP_parityBox->addItem(tr("OddParity"));
    ui->SP_parityBox->addItem(tr("SpaceParity"));
    ui->SP_parityBox->addItem(tr("MarkParity"));
    ui->SP_parityBox->setItemData(0, QSerialPort::NoParity);
    ui->SP_parityBox->setItemData(1, QSerialPort::EvenParity);
    ui->SP_parityBox->setItemData(2, QSerialPort::OddParity);
    ui->SP_parityBox->setItemData(3, QSerialPort::SpaceParity);
    ui->SP_parityBox->setItemData(4, QSerialPort::MarkParity);
    ui->SP_parityBox->setCurrentIndex(0);
    ui->SP_stopBitsBox->addItem("1");
    ui->SP_stopBitsBox->addItem("1.5");
    ui->SP_stopBitsBox->addItem("2");
    ui->SP_stopBitsBox->setItemData(0, QSerialPort::OneStop);
    ui->SP_stopBitsBox->setItemData(1, QSerialPort::OneAndHalfStop);
    ui->SP_stopBitsBox->setItemData(2, QSerialPort::TwoStop);
    ui->SP_stopBitsBox->setCurrentIndex(0);
    ui->SP_dataBitsBox->addItem("5");
    ui->SP_dataBitsBox->addItem("6");
    ui->SP_dataBitsBox->addItem("7");
    ui->SP_dataBitsBox->addItem("8");
    ui->SP_dataBitsBox->setItemData(0, QSerialPort::Data5);
    ui->SP_dataBitsBox->setItemData(1, QSerialPort::Data6);
    ui->SP_dataBitsBox->setItemData(2, QSerialPort::Data7);
    ui->SP_dataBitsBox->setItemData(3, QSerialPort::Data8);
    ui->SP_dataBitsBox->setCurrentIndex(3);
    on_SP_advancedBox_clicked(false);
}

void DeviceTab::getAvailableTypes(bool useFirstValid)
{
    int firstValid = -1;
    Connection::Type currType = ui->typeBox->currentData().value<Connection::Type>();
    QHostAddress currNetLocalAddr;
    QStandardItemModel* model = static_cast<QStandardItemModel*>(ui->typeBox->model());
    // need some code to check if BLE is supported
    QVector<Connection::Type> invalid =
    {Connection::BLE_Peripheral};
    const QMap<Connection::Type, QString> typeNameMap =
    {
        {Connection::SerialPort, tr("SerialPort")},
        {Connection::BT_Client, tr("Bluetooth Client")},
        {Connection::BT_Server, tr("Bluetooth Server")},
        {Connection::BLE_Central, tr("BLE Central")},
        {Connection::BLE_Peripheral, tr("BLE Peripheral")},
        {Connection::TCP_Client, tr("TCP Client")},
        {Connection::TCP_Server, tr("TCP Server")},
        {Connection::UDP, tr("UDP")}
    };
#ifdef Q_OS_ANDROID
    invalid += Connection::SerialPort;
#endif
#ifdef Q_OS_WINDOWS
    invalid += Connection::BLE_Peripheral;
#endif
    // check Bluetooth adapters, add adapter info into adapterBox
    ui->BTClient_adapterBox->clear();
    ui->BTServer_adapterBox->clear();
    ui->BTClient_adapterBox->clear();
    int id = 0;
#ifdef Q_OS_ANDROID
    // need modify
    if(QtAndroid::checkPermission("android.permission.BLUETOOTH_CONNECT") == QtAndroid::PermissionResult::Denied)
        QtAndroid::requestPermissionsSync({"android.permission.BLUETOOTH_CONNECT"}, 10000);
#endif
    auto BTAdapterList = QBluetoothLocalDevice::allDevices();
    for(auto it = BTAdapterList.cbegin(); it != BTAdapterList.cend(); ++it)
    {
        qDebug() << "dev:" << it->name() << it->address();
        QBluetoothLocalDevice dev(it->address());
        if(dev.isValid() && dev.hostMode() != QBluetoothLocalDevice::HostPoweredOff)
        {
            QString name = QString("%1:%2").arg(id + 1).arg(it->name());
            ui->BTClient_adapterBox->addItem(name, it->address().toString());
            ui->BTServer_adapterBox->addItem(name, it->address().toString());
            ui->BTClient_adapterBox->addItem(name, it->address().toString());
            id++;
        }
    }
    if(id == 0)
    {
        invalid += Connection::BT_Client;
        invalid += Connection::BT_Server;
        invalid += Connection::BLE_Central;
        invalid += Connection::BLE_Peripheral;
    }
    else if(id == 1)
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
    if(currType == Connection::UDP)
        // for multicast address
        currNetLocalAddr = QHostAddress(ui->Net_localAddrBox->currentText());
    ui->Net_localAddrBox->clear();
    auto netInterfaceList = QNetworkInterface::allAddresses();
    for(auto it = netInterfaceList.cbegin(); it != netInterfaceList.cend(); ++it)
        ui->Net_localAddrBox->addItem((*it).toString());
    if(netInterfaceList.empty())
    {
        invalid += Connection::TCP_Client;
        invalid += Connection::TCP_Server;
        invalid += Connection::UDP;
    }
    if(currType == Connection::UDP && currNetLocalAddr.isMulticast())
        ui->Net_localAddrBox->setCurrentText(currNetLocalAddr.toString());

    ui->typeBox->blockSignals(true);
    ui->typeBox->clear();
    for(auto it = typeNameMap.cbegin(); it != typeNameMap.cend(); ++it)
    {
        if(invalid.contains(it.key()))
        {
            ui->typeBox->addItem("!" + it.value(), it.key());
            Util::disableItem(model, it.key());
        }
        else
        {
            ui->typeBox->addItem(it.value(), it.key());
            if(firstValid < 0 && useFirstValid)
            {
                ui->typeBox->setCurrentIndex(it.key());
                on_typeBox_currentIndexChanged(it.key()); // this signal is blocked now
                firstValid = it.key();
            }
        }
    }
    if(!useFirstValid)
        ui->typeBox->setCurrentIndex(currType);
    ui->typeBox->blockSignals(false);
}

void DeviceTab::onClientCountChanged()
{
    if(m_connection->type() == Connection::BT_Server)
    {
        ui->BTServer_deviceList->setRowCount(0);
        auto list = m_connection->BTServer_clientList();
        ui->BTServer_deviceList->setRowCount(list.size());
        for(int i = 0; i < list.size(); i++)
        {
            ui->BTServer_deviceList->setItem(i, 0, new QTableWidgetItem(list[i]->peerName()));
            ui->BTServer_deviceList->setItem(i, 1, new QTableWidgetItem(list[i]->peerAddress().toString()));
        }
    }
    else if(m_connection->type() == Connection::TCP_Server)
    {
        ui->Net_addrPortList->setRowCount(0);
        auto list = m_connection->TCPServer_clientList();
        ui->Net_addrPortList->setRowCount(list.size());
        for(int i = 0; i < list.size(); i++)
        {
            ui->Net_addrPortList->setItem(i, 0, new QTableWidgetItem(list[i]->peerAddress().toString()));
            ui->Net_addrPortList->setItem(i, 1, new QTableWidgetItem(list[i]->peerPort()));
            ui->Net_addrPortList->setItem(i, 1, new QTableWidgetItem(list[i]->peerName()));
        }
    }
    emit clientCountChanged();
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
            m_connection->SP_setBaudRate(baud);
            emit argumentChanged();
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
    }
    return false;
}

void DeviceTab::on_SP_advancedBox_clicked(bool checked)
{
    ui->SP_dataBitsLabel->setVisible(checked);
    ui->SP_dataBitsBox->setVisible(checked);
    ui->SP_stopBitsLabel->setVisible(checked);
    ui->SP_stopBitsBox->setVisible(checked);
    ui->SP_parityLabel->setVisible(checked);
    ui->SP_parityBox->setVisible(checked);
    ui->SP_flowControlLabel->setVisible(checked);
    ui->SP_flowControlBox->setVisible(checked);
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
        m_connection->setArgument(arg);
        m_connection->open();
    }
    else if(currType == Connection::BT_Client)
    {
        if(m_connection->isConnected())
        {
            QMessageBox::warning(this, tr("Error"), tr("The device is already connected."));
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
        m_connection->setArgument(arg);
        m_connection->open();
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
    }
    else if(currType == Connection::TCP_Client)
    {
        if(m_connection->isConnected())
        {
            QMessageBox::warning(this, tr("Error"), tr("The device is already connected."));
            return;
        }
        else if(m_connection->state() == Connection::Connecting)
        {
            // force disconnect
            m_connection->close(true);
        }
        Connection::NetworkArgument arg;
        arg.localAddress = QHostAddress(ui->Net_localAddrBox->currentText());
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
        arg.localAddress = QHostAddress(ui->Net_localAddrBox->currentText());
        arg.localPort = ui->Net_localPortEdit->text().toUInt();
        m_connection->setArgument(arg);
        m_connection->open();
    }
    else if(currType == Connection::UDP)
    {
        if(m_connection->state() != Connection::Unconnected)
        {
            QMessageBox::warning(this, tr("Error"), tr("(Something to say)"));
            return;
        }
        Connection::NetworkArgument arg;
        arg.localAddress = QHostAddress(ui->Net_localAddrBox->currentText());
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

void DeviceTab::onTargetListCellClicked(int row, int column)
{
    Q_UNUSED(column);
    if(m_connection == nullptr)
        return;
    Connection::Type currType = m_connection->type();
    if(currType == Connection::SerialPort)
    {
        ui->SP_portNameBox->setCurrentIndex(row);

        QStringList preferences = settings->childGroups();
        QStringList::iterator it;

        // search preference by <vendorID>-<productID>
        QString id = ui->SP_portList->item(row, 6)->text();  // vendor id
        id += "-";
        id += ui->SP_portList->item(row, 7)->text(); // product id
        for(it = preferences.begin(); it != preferences.end(); ++it)
        {
            if(*it == id)
            {
                loadDevicesPreference(id);
                break;
            }
        }
        if(it != preferences.end())
            return;

        // search preference by DeviceName
        id = ui->SP_portList->item(row, 0)->text();
        for(it = preferences.begin(); it != preferences.end(); ++it)
        {
            if(*it == id)
            {
                loadDevicesPreference(id);
                break;
            }
        }
    }
    else if(currType == Connection::BT_Client)
    {
        ui->BTClient_targetAddrBox->setCurrentIndex(row);
    }
    else if(currType == Connection::BLE_Central)
    {
        if(sender() == ui->BLEC_deviceList)
        {
            ui->BLEC_currAddrBox->setCurrentIndex(row);
        }
        else if(sender() == ui->BLEC_UUIDList)
        {

        }
    }
}

// platform specific
// **********************************************************************************************************************************************

void DeviceTab::saveDevicesPreference(const QString & deviceName)
{
    if(settings->group() != "")
        return;
    QSerialPortInfo info(deviceName);
    QString id;
    if(info.vendorIdentifier() != 0 && info.productIdentifier() != 0)
        id = QString::number(info.vendorIdentifier()) + "-" + QString::number(info.productIdentifier());
    else
        id = deviceName;
    settings->beginGroup(id);
    settings->setValue("BaudRate", ui->SP_baudRateBox->currentText());
    settings->setValue("DataBitsID", ui->SP_dataBitsBox->currentIndex());
    settings->setValue("StopBitsID", ui->SP_stopBitsBox->currentIndex());
    settings->setValue("ParityID", ui->SP_parityBox->currentIndex());
    settings->setValue("FlowControlID", ui->SP_flowControlBox->currentIndex());
    settings->endGroup();
}

void DeviceTab::loadDevicesPreference(const QString & id)
{
    settings->beginGroup(id);
    ui->SP_baudRateBox->setEditText(settings->value("BaudRate").toString());
    ui->SP_dataBitsBox->setCurrentIndex(settings->value("DataBitsID").toInt());
    ui->SP_stopBitsBox->setCurrentIndex(settings->value("StopBitsID").toInt());
    ui->SP_parityBox->setCurrentIndex(settings->value("ParityID").toInt());
    ui->SP_flowControlBox->setCurrentIndex(settings->value("FlowControlID").toInt());
    settings->endGroup();
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
    if(currType == Connection::BT_Client)
    {
        int i = ui->BTClient_deviceList->rowCount();
        ui->BTClient_deviceList->setRowCount(i + 1);
        ui->BTClient_deviceList->setItem(i, 0, new QTableWidgetItem(name));
        ui->BTClient_deviceList->setItem(i, 1, new QTableWidgetItem(address));
        ui->BTClient_deviceList->setItem(i, 2, new QTableWidgetItem(tr("Discovered")));
        ui->BTClient_deviceList->setItem(i, 3, new QTableWidgetItem(rssi));
        ui->BTClient_targetAddrBox->addItem(address);
        ui->BTClient_targetAddrBox->adjustSize();
    }
    else if(currType == Connection::BLE_Central)
    {
        int i = ui->BLEC_deviceList->rowCount();
        ui->BLEC_deviceList->setRowCount(i + 1);
        ui->BLEC_deviceList->setItem(i, 0, new QTableWidgetItem(name));
        ui->BLEC_deviceList->setItem(i, 1, new QTableWidgetItem(address));
        ui->BLEC_deviceList->setItem(i, 2, new QTableWidgetItem(tr("Discovered")));
        ui->BLEC_deviceList->setItem(i, 3, new QTableWidgetItem(rssi));
        ui->BLEC_currAddrBox->addItem(address);
        ui->BLEC_currAddrBox->adjustSize();
    }
    qDebug() << name
             << address
             << device.isValid()
             << device.rssi()
             << device.majorDeviceClass()
             << device.minorDeviceClass()
             << device.serviceClasses()
             << device.manufacturerData();
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
    }
    else if(newType == Connection::BT_Server)
    {
        ui->targetListStack->setCurrentWidget(ui->BTServerListPage);
        ui->argsStack->setCurrentWidget(ui->BTServerArgsPage);
    }
    else if(newType == Connection::BLE_Central)
    {
        ui->targetListStack->setCurrentWidget(ui->BLECentralListPage);
        ui->argsStack->setCurrentWidget(ui->BLECentralArgsPage);
    }
    else if(newType == Connection::TCP_Client)
    {
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
    }
    else if(newType == Connection::TCP_Server)
    {
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
    }
    else if(newType == Connection::UDP)
    {
        ui->Net_addrPortList->setRowCount(0);
        ui->Net_localAddrBox->show();
        ui->Net_localAddrBox->setEditable(true);
        ui->Net_localPortEdit->show();
        ui->Net_remoteAddrLabel->show();
        ui->Net_remoteAddrLabel->setText(tr("Remote Address:"));
        ui->Net_remotePortLabel->show();
        ui->Net_remoteAddrEdit->show();
        ui->Net_remotePortEdit->show();
        ui->Net_tipLabel->show();
        ui->targetListStack->setCurrentWidget(ui->NetListPage);
        ui->argsStack->setCurrentWidget(ui->NetArgsPage);
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
    Q_UNUSED(index)
    ui->BTClient_localAddrLabel->setText(ui->BTClient_adapterBox->currentData().toString());
    setBTClientDiscoveryAgent(QBluetoothAddress(ui->BTClient_adapterBox->currentData().toString()));
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
        delete BTClient_discoveryAgent;
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
    if(m_connection->type() == Connection::UDP)
    {
        bool convOk = false;
        quint16 port = ui->Net_remotePortEdit->text().toUInt(&convOk);
        QHostAddress addr; // test validity
        if(convOk && addr.setAddress(ui->Net_remoteAddrEdit->text()))
            m_connection->UDP_setRemote(ui->Net_remoteAddrEdit->text(), port);
    }
}

void DeviceTab::on_Net_localAddrBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(ui->Net_remoteAddrEdit->isHidden())
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
        emit argumentChanged();
}


void DeviceTab::on_SP_dataBitsBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setDataBits((QSerialPort::DataBits)ui->SP_dataBitsBox->currentData().toInt()))
        emit argumentChanged();
}


void DeviceTab::on_SP_stopBitsBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setStopBits((QSerialPort::StopBits)ui->SP_stopBitsBox->currentData().toInt()))
        emit argumentChanged();
}


void DeviceTab::on_SP_parityBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setParity((QSerialPort::Parity)ui->SP_parityBox->currentData().toInt()))
        emit argumentChanged();
}


void DeviceTab::on_SP_flowControlBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(m_connection == nullptr || m_connection->type() != Connection::SerialPort || !m_connection->isConnected())
        return;
    if(m_connection->SP_setFlowControl((QSerialPort::FlowControl)ui->SP_flowControlBox->currentData().toInt()))
        emit argumentChanged();
}


void DeviceTab::on_BLEC_connectButton_clicked()
{
    ui->BLEC_UUIDList->clear();
    QLowEnergyController *controller;
    controller = QLowEnergyController::createCentral(QBluetoothAddress(ui->BTClient_targetAddrBox->currentText()), QBluetoothAddress(ui->BTClient_adapterBox->currentData().toString()));
}

