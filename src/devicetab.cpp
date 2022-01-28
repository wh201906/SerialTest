#include "devicetab.h"
#include "ui_devicetab.h"

#include <QDebug>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#include <QBluetoothUuid>
#include <QBluetoothLocalDevice>
#else
#include <QSerialPort>
#include <QSerialPortInfo>
#endif

DeviceTab::DeviceTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceTab)
{
    ui->setupUi(this);

#ifdef Q_OS_ANDROID
    BTdiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(BTdiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceTab::BTdeviceDiscovered);
    connect(BTdiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &DeviceTab::BTdiscoverFinished);
    connect(BTdiscoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &DeviceTab::BTdiscoverFinished);
#endif

    initUI();
    connect(ui->refreshDevicesButton, &QPushButton::clicked, this, &DeviceTab::refreshDevicesInfo);
    refreshDevicesInfo();
}

DeviceTab::~DeviceTab()
{
    delete ui;
}

void DeviceTab::initSettings()
{
    settings = MySettings::defaultSettings();
}

void DeviceTab::refreshDevicesInfo()
{
    ui->deviceTable->clearContents();
    ui->deviceBox->clear();
#ifdef Q_OS_ANDROID
    ui->refreshDevicesButton->setText(tr("Searching..."));
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
    QAndroidJniObject array = helper.callObjectMethod("getBondedDevices", "()[Ljava/lang/String;");
    int arraylen = env->GetArrayLength(array.object<jarray>());
    qDebug() << "arraylen:" << arraylen;
    ui->deviceTable->setRowCount(arraylen);
    for(int i = 0; i < arraylen; i++)
    {
        QString info = QAndroidJniObject::fromLocalRef(env->GetObjectArrayElement(array.object<jobjectArray>(), i)).toString();
        QString address = info.left(info.indexOf(' '));
        QString name = info.right(info.length() - info.indexOf(' ') - 1);
        qDebug() << address << name;
        ui->deviceTable->setItem(i, HDeviceName, new QTableWidgetItem(name));
        ui->deviceTable->setItem(i, HSystemLocation, new QTableWidgetItem(address));
        ui->deviceTable->setItem(i, HDescription, new QTableWidgetItem(tr("Bonded")));
        ui->deviceBox->addItem(address);
    }

    BTdiscoveryAgent->start();
#else
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    ui->deviceTable->setRowCount(ports.size());
    for(int i = 0; i < ports.size(); i++)
    {
        ui->deviceTable->setItem(i, HDeviceName, new QTableWidgetItem(ports[i].portName()));
        ui->deviceBox->addItem(ports[i].portName());
        ui->deviceTable->setItem(i, HDescription, new QTableWidgetItem(ports[i].description()));
        ui->deviceTable->setItem(i, HManufacturer, new QTableWidgetItem(ports[i].manufacturer()));
        ui->deviceTable->setItem(i, HSerialNumber, new QTableWidgetItem(ports[i].serialNumber()));
        ui->deviceTable->setItem(i, HIsNull, new QTableWidgetItem(ports[i].isNull() ? "Yes" : "No"));
        ui->deviceTable->setItem(i, HSystemLocation, new QTableWidgetItem(ports[i].systemLocation()));
        ui->deviceTable->setItem(i, HVendorID, new QTableWidgetItem(QString::number(ports[i].vendorIdentifier())));
        ui->deviceTable->setItem(i, HProductID, new QTableWidgetItem(QString::number(ports[i].productIdentifier())));

        QList<qint32> baudRateList = ports[i].standardBaudRates();
        QString baudRates = "";
        for(int j = 0; j < baudRates.size(); j++)
        {
            baudRates += QString::number(baudRateList[j]) + ", ";
        }
        ui->deviceTable->setItem(i, HBaudRates, new QTableWidgetItem(baudRates));
    }
#endif

}

void DeviceTab::initUI()
{
#ifdef Q_OS_ANDROID
    ui->baudRateLabel->setVisible(false);
    ui->baudRateBox->setVisible(false);
    ui->advancedBox->setVisible(false);
    ui->deviceTable->hideColumn(HManufacturer);
    ui->deviceTable->hideColumn(HSerialNumber);
    ui->deviceTable->hideColumn(HIsNull);
    ui->deviceTable->hideColumn(HVendorID);
    ui->deviceTable->hideColumn(HProductID);
    ui->deviceTable->hideColumn(HBaudRates);

    ui->deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->deviceTable->horizontalHeaderItem(HDeviceName)->setText(tr("DeviceName"));
    ui->deviceTable->horizontalHeaderItem(HDescription)->setText(tr("Type"));
    ui->deviceTable->horizontalHeaderItem(HSystemLocation)->setText(tr("MAC Address"));

    ui->deviceLabel->setText(tr("Device") + ":");
    ui->deviceBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
#else
    ui->flowControlBox->addItem(tr("NoFlowControl"));
    ui->flowControlBox->addItem(tr("HardwareControl"));
    ui->flowControlBox->addItem(tr("SoftwareControl"));
    ui->flowControlBox->setItemData(0, QSerialPort::NoFlowControl);
    ui->flowControlBox->setItemData(1, QSerialPort::HardwareControl);
    ui->flowControlBox->setItemData(2, QSerialPort::SoftwareControl);
    ui->flowControlBox->setCurrentIndex(0);
    ui->parityBox->addItem(tr("NoParity"));
    ui->parityBox->addItem(tr("EvenParity"));
    ui->parityBox->addItem(tr("OddParity"));
    ui->parityBox->addItem(tr("SpaceParity"));
    ui->parityBox->addItem(tr("MarkParity"));
    ui->parityBox->setItemData(0, QSerialPort::NoParity);
    ui->parityBox->setItemData(1, QSerialPort::EvenParity);
    ui->parityBox->setItemData(2, QSerialPort::OddParity);
    ui->parityBox->setItemData(3, QSerialPort::SpaceParity);
    ui->parityBox->setItemData(4, QSerialPort::MarkParity);
    ui->parityBox->setCurrentIndex(0);
    ui->stopBitsBox->addItem("1");
    ui->stopBitsBox->addItem("1.5");
    ui->stopBitsBox->addItem("2");
    ui->stopBitsBox->setItemData(0, QSerialPort::OneStop);
    ui->stopBitsBox->setItemData(1, QSerialPort::OneAndHalfStop);
    ui->stopBitsBox->setItemData(2, QSerialPort::TwoStop);
    ui->stopBitsBox->setCurrentIndex(0);
    ui->dataBitsBox->addItem("5");
    ui->dataBitsBox->addItem("6");
    ui->dataBitsBox->addItem("7");
    ui->dataBitsBox->addItem("8");
    ui->dataBitsBox->setItemData(0, QSerialPort::Data5);
    ui->dataBitsBox->setItemData(1, QSerialPort::Data6);
    ui->dataBitsBox->setItemData(2, QSerialPort::Data7);
    ui->dataBitsBox->setItemData(3, QSerialPort::Data8);
    ui->dataBitsBox->setCurrentIndex(3);

    ui->deviceLabel->setText(tr("Port") + ":");
#endif
    on_advancedBox_clicked(false);
}

void DeviceTab::on_advancedBox_clicked(bool checked)
{
    ui->dataBitsLabel->setVisible(checked);
    ui->dataBitsBox->setVisible(checked);
    ui->stopBitsLabel->setVisible(checked);
    ui->stopBitsBox->setVisible(checked);
    ui->parityLabel->setVisible(checked);
    ui->parityBox->setVisible(checked);
    ui->flowControlLabel->setVisible(checked);
    ui->flowControlBox->setVisible(checked);
}

void DeviceTab::on_openButton_clicked()
{
#ifdef Q_OS_ANDROID
    emit openDevice(ui->deviceBox->currentText());
#else
    emit openDevice(ui->deviceBox->currentText(), ui->baudRateBox->currentText().toInt(), (QSerialPort::DataBits)ui->dataBitsBox->currentData().toInt(), (QSerialPort::StopBits)ui->stopBitsBox->currentData().toInt(), (QSerialPort::Parity)ui->parityBox->currentData().toInt(), (QSerialPort::FlowControl)ui->flowControlBox->currentData().toInt());
#endif
}

void DeviceTab::on_closeButton_clicked()
{
    emit closeDevice();
}

void DeviceTab::on_deviceTable_cellClicked(int row, int column)
{
    Q_UNUSED(column);
    ui->deviceBox->setCurrentIndex(row);
#ifndef Q_OS_ANDROID
    QStringList preferences = settings->childGroups();
    QStringList::iterator it;


    // search preference by <vendorID>-<productID>
    QString id = ui->deviceTable->item(row, HVendorID)->text();  // vendor id
    id += "-";
    id += ui->deviceTable->item(row, HProductID)->text(); // product id
    for(it = preferences.begin(); it != preferences.end(); it++)
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
    id = ui->deviceTable->item(row, HDeviceName)->text();
    for(it = preferences.begin(); it != preferences.end(); it++)
    {
        if(*it == id)
        {
            loadDevicesPreference(id);
            break;
        }
    }
#endif
}

// platform specific
// **********************************************************************************************************************************************

#ifndef Q_OS_ANDROID
void DeviceTab::saveDevicesPreference(const QString& deviceName)
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
    settings->setValue("BaudRate", ui->baudRateBox->currentText());
    settings->setValue("DataBitsID", ui->dataBitsBox->currentIndex());
    settings->setValue("StopBitsID", ui->stopBitsBox->currentIndex());
    settings->setValue("ParityID", ui->parityBox->currentIndex());
    settings->setValue("FlowControlID", ui->flowControlBox->currentIndex());
    settings->endGroup();
}

void DeviceTab::loadDevicesPreference(const QString& id)
{
    settings->beginGroup(id);
    ui->baudRateBox->setEditText(settings->value("BaudRate").toString());
    ui->dataBitsBox->setCurrentIndex(settings->value("DataBitsID").toInt());
    ui->stopBitsBox->setCurrentIndex(settings->value("StopBitsID").toInt());
    ui->parityBox->setCurrentIndex(settings->value("ParityID").toInt());
    ui->flowControlBox->setCurrentIndex(settings->value("FlowControlID").toInt());
    settings->endGroup();
}
#else
void DeviceTab::BTdiscoverFinished()
{
    ui->refreshDevicesButton->setText(tr("Refresh"));
}

void DeviceTab::BTdeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    QString address = device.address().toString();
    QString name = device.name();
    int i = ui->deviceTable->rowCount();
    ui->deviceTable->setRowCount(i + 1);
    ui->deviceTable->setItem(i, HDeviceName, new QTableWidgetItem(name));
    ui->deviceTable->setItem(i, HSystemLocation, new QTableWidgetItem(address));
    ui->deviceTable->setItem(i, HDescription, new QTableWidgetItem(tr("Discovered")));
    ui->deviceBox->addItem(address);
    ui->deviceBox->adjustSize();
    qDebug() << name
             << address
             << device.isValid()
             << device.rssi()
             << device.majorDeviceClass()
             << device.minorDeviceClass()
             << device.serviceClasses()
             << device.manufacturerData();
}
#endif
