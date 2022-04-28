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
    connect(ui->Net_remoteAddrEdit, &QLineEdit::editingFinished, this, &DeviceTab::Net_onRemoteChanged);
    connect(ui->Net_remotePortEdit, &QLineEdit::editingFinished, this, &DeviceTab::Net_onRemoteChanged);

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
    ui->BTClient_deviceList->setRowCount(arraylen);
    for(int i = 0; i < arraylen; i++)
    {
        QString info = QAndroidJniObject::fromLocalRef(env->GetObjectArrayElement(array.object<jobjectArray>(), i)).toString();
        QString address = info.left(info.indexOf(' '));
        QString name = info.right(info.length() - info.indexOf(' ') - 1);
        qDebug() << address << name;
        ui->BTClient_deviceList->setItem(i, 0, new QTableWidgetItem(name));
        ui->BTClient_deviceList->setItem(i, 1, new QTableWidgetItem(address));
        ui->BTClient_deviceList->setItem(i, 2, new QTableWidgetItem(tr("Bonded")));
        ui->BTClient_targetAddrBox->addItem(address);
    }
}
#endif

void DeviceTab::initUI()
{
    ui->SP_portList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->BTClient_deviceList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

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
    QVector<Connection::Type> invalid =
    {Connection::BLE_Central, Connection::BLE_Peripheral, Connection::TCP_Client, Connection::TCP_Server};
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
            ui->BTClient_adapterBox->addItem(QString("%1:%2").arg(id + 1).arg(it->name()), it->address().toString());
            ui->BTServer_adapterBox->addItem(QString("%1:%2").arg(id + 1).arg(it->name()), it->address().toString());
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
    }
    else // more than one adapter
    {
        ui->BTClient_localAdapterWidget->show();
    }
    on_BTClient_adapterBox_activated(0); // index is unused there
    on_BTServer_adapterBox_activated(0); // index is unused there

    // check network interfaces
    if(currType == Connection::UDP)
        // for multicast address
        currNetLocalAddr = QHostAddress(ui->Net_localAddrBox->currentText());
    ui->Net_localAddrBox->clear();
    id = 0;
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
        arg.baudRate = ui->SP_baudRateBox->currentText().toUInt();
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
        // show "..." in statusBar
        emit updateStatusBar();
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
        // show "..." in statusBar
        emit updateStatusBar();
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
        arg.remoteAddress = QHostAddress(ui->Net_remoteAddrEdit->text());
        arg.remotePort = ui->Net_remotePortEdit->text().toUInt();
        m_connection->setArgument(arg);
        m_connection->open();
        emit updateStatusBar();
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
    if(m_connection->type() == Connection::SerialPort)
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
    else if(m_connection->type() == Connection::BT_Client)
    {
        ui->BTClient_targetAddrBox->setCurrentIndex(row);
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

void DeviceTab::BTdeviceDiscovered(const QBluetoothDeviceInfo & device)
{
    QString address = device.address().toString();
    QString name = device.name();
    QString rssi = QString::number(device.rssi());
    int i = ui->BTClient_deviceList->rowCount();
    ui->BTClient_deviceList->setRowCount(i + 1);
    ui->BTClient_deviceList->setItem(i, 0, new QTableWidgetItem(name));
    ui->BTClient_deviceList->setItem(i, 1, new QTableWidgetItem(address));
    ui->BTClient_deviceList->setItem(i, 2, new QTableWidgetItem(tr("Discovered")));
    ui->BTClient_deviceList->setItem(i, 3, new QTableWidgetItem(rssi));
    ui->BTClient_targetAddrBox->addItem(address);
    ui->BTClient_targetAddrBox->adjustSize();
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
    else if(newType == Connection::UDP)
    {
        ui->Net_localAddrBox->show();
        ui->Net_localAddrBox->setEditable(true);
        ui->Net_localPortEdit->show();
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
        QHostAddress addr;
        if(convOk && addr.setAddress(ui->Net_remoteAddrEdit->text()))
            m_connection->UDP_setRemote(addr, port);
    }
}

