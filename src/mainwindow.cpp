#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifdef Q_OS_ANDROID
#include <QBluetoothLocalDevice>
#include <QAndroidJniEnvironment>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    contextMenu = new QMenu();
#ifdef Q_OS_ANDROID
    BTSocket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);
    IODevice = BTSocket;
    connect(BTSocket, &QBluetoothSocket::connected, this, &MainWindow::onBTConnectionChanged);
    connect(BTSocket, &QBluetoothSocket::disconnected, this, &MainWindow::onBTConnectionChanged);
    connect(BTSocket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error), this, &MainWindow::onBTConnectionChanged);
    BTdiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(BTdiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &MainWindow::BTdeviceDiscovered);
    connect(BTdiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &MainWindow::BTdiscoverFinished);
    connect(BTdiscoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &MainWindow::BTdiscoverFinished);

    setStyleSheet("QCheckBox::indicator{min-width:15px;min-height:15px;}");

    // on Android, use default.
    MySettings::init(QSettings::NativeFormat);

#else
    serialPort = new QSerialPort();
    IODevice = serialPort;
    connect(serialPort, &QSerialPort::errorOccurred, this, &MainWindow::onSerialErrorOccurred);
    serialPortInfo = new QSerialPortInfo();

    baudRateLabel = new QLabel();
    dataBitsLabel = new QLabel();
    stopBitsLabel = new QLabel();
    parityLabel = new QLabel();
    onTopBox = new QCheckBox(tr("On Top"));
    connect(onTopBox, &QCheckBox::clicked, this, &MainWindow::onTopBoxClicked);

    // on PC, store preferences in files for portable use
    MySettings::init(QSettings::IniFormat, "preference.ini");


    dockAllWindows = new QAction(tr("Dock all windows"), this);
    connect(dockAllWindows, &QAction::triggered, [ = ]()
    {
        for(int i = 0; i < dockList.size(); i++)
            dockList[i]->setFloating(false);
    });
    contextMenu->addAction(dockAllWindows);
    contextMenu->addSeparator();
#endif
    settings = MySettings::defaultSettings();
    portLabel = new QLabel();
    stateButton = new QPushButton();
    TxLabel = new QLabel();
    RxLabel = new QLabel();

    rawReceivedData = new QByteArray();
    rawSendedData = new QByteArray();
    RxUIBuf = new QByteArray();

    dataTab = new DataTab(rawReceivedData, rawSendedData);
    dataTab->setIODevice(IODevice);
    connect(dataTab, &DataTab::setRxLabelText, RxLabel, &QLabel::setText);
    connect(dataTab, &DataTab::setTxLabelText, TxLabel, &QLabel::setText);
    connect(dataTab, &DataTab::send, this, &MainWindow::sendData);
    ui->funcTab->insertTab(1, dataTab, "DData");
    plotTab = new PlotTab();
    ui->funcTab->insertTab(2, plotTab, "PPlot");
    ctrlTab = new CtrlTab();
    connect(ctrlTab, &CtrlTab::send, this, &MainWindow::sendData);
    connect(dataTab, &DataTab::setDataCodec, ctrlTab, &CtrlTab::setDataCodec);
    ui->funcTab->insertTab(3, ctrlTab, "CCtrl");

    IODeviceState = false;
    updateUITimer = new QTimer();
    updateUITimer->setInterval(20);

    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPortsInfo);

    connect(IODevice, &QIODevice::readyRead, this, &MainWindow::readData, Qt::QueuedConnection);


    connect(updateUITimer, &QTimer::timeout, this, &MainWindow::updateRxUI);
    connect(stateButton, &QPushButton::clicked, this, &MainWindow::onStateButtonClicked);

    refreshPortsInfo();
    initUI();

    myInfo = new QAction("wh201906", this);
    currVersion = new QAction(tr("Ver: ") + QApplication::applicationVersion().section('.', 0, -2), this); // ignore the 4th version number
    checkUpdate = new QAction(tr("Check Update"), this);
    connect(myInfo, &QAction::triggered, [ = ]()
    {
        QDesktopServices::openUrl(QUrl("https://github.com/wh201906"));
    });
    connect(checkUpdate, &QAction::triggered, [ = ]()
    {
        QDesktopServices::openUrl(QUrl("https://github.com/wh201906/SerialTest/releases"));
    });

    contextMenu->addAction(myInfo);
    currVersion->setEnabled(false);
    contextMenu->addAction(currVersion);
    contextMenu->addAction(checkUpdate);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    contextMenu->exec(event->globalPos());
}

void MainWindow::onStateButtonClicked()
{
    QString portName;
#ifdef Q_OS_ANDROID
    portName = BTlastAddress;
#else
    portName = serialPort->portName();
#endif
    if(portName.isEmpty())
    {
        QMessageBox::warning(this, "Error", tr("Plz connect to a port first."));
        return;
    }
    if(IODeviceState)
    {
        IODevice->close();
        onIODeviceDisconnected();
    }
    else
    {
#ifdef Q_OS_ANDROID
        BTSocket->connectToService(QBluetoothAddress(BTlastAddress), QBluetoothUuid::SerialPort);
#else
        IODeviceState = IODevice->open(QIODevice::ReadWrite);
        if(IODeviceState)
            onIODeviceConnected();
        else
            QMessageBox::warning(this, "Error", tr("Cannot open the serial port."));
#endif
    }
}

void MainWindow::initUI()
{
    statusBar()->addWidget(portLabel, 1);
    statusBar()->addWidget(stateButton, 1);
    statusBar()->addWidget(RxLabel, 1);
    statusBar()->addWidget(TxLabel, 1);
#ifdef Q_OS_ANDROID
    ui->baudRateLabel->setVisible(false);
    ui->baudRateBox->setVisible(false);
    ui->advancedBox->setVisible(false);
    ui->portTable->hideColumn(HManufacturer);
    ui->portTable->hideColumn(HSerialNumber);
    ui->portTable->hideColumn(HIsNull);
    ui->portTable->hideColumn(HVendorID);
    ui->portTable->hideColumn(HProductID);
    ui->portTable->hideColumn(HBaudRates);

    ui->portTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->portTable->horizontalHeaderItem(HPortName)->setText(tr("DeviceName"));
    ui->portTable->horizontalHeaderItem(HDescription)->setText(tr("Type"));
    ui->portTable->horizontalHeaderItem(HSystemLocation)->setText(tr("MAC Address"));

    ui->portLabel->setText(tr("MAC Address") + ":");

    // keep screen on

    QAndroidJniObject helper("priv/wh201906/serialtest/BTHelper");
    QtAndroid::runOnAndroidThread([&]
    {
        helper.callMethod<void>("keepScreenOn", "(Landroid/app/Activity;)V", QtAndroid::androidActivity().object());
    });

    // Strange resize behavior on Android
    // Need a fixed size
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(QApplication::primaryScreen()->availableGeometry().size());

#else
    ui->flowControlBox->addItem(tr("NoFlowControl"));
    ui->flowControlBox->addItem(tr("HardwareControl"));
    ui->flowControlBox->addItem(tr("SoftwareControl"));
    ui->flowControlBox->setItemData(0, QSerialPort::NoFlowControl);
    ui->flowControlBox->setItemData(1, QSerialPort::HardwareControl);
    ui->flowControlBox->setItemData(2, QSerialPort::SoftwareControl);
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
    ui->stopBitsBox->addItem("1");
    ui->stopBitsBox->addItem("1.5");
    ui->stopBitsBox->addItem("2");
    ui->stopBitsBox->setItemData(0, QSerialPort::OneStop);
    ui->stopBitsBox->setItemData(1, QSerialPort::OneAndHalfStop);
    ui->stopBitsBox->setItemData(2, QSerialPort::TwoStop);
    ui->dataBitsBox->addItem("5");
    ui->dataBitsBox->addItem("6");
    ui->dataBitsBox->addItem("7");
    ui->dataBitsBox->addItem("8");
    ui->dataBitsBox->setItemData(0, QSerialPort::Data5);
    ui->dataBitsBox->setItemData(1, QSerialPort::Data6);
    ui->dataBitsBox->setItemData(2, QSerialPort::Data7);
    ui->dataBitsBox->setItemData(3, QSerialPort::Data8);
    ui->dataBitsBox->setCurrentIndex(3);

    statusBar()->addWidget(baudRateLabel, 1);
    statusBar()->addWidget(dataBitsLabel, 1);
    statusBar()->addWidget(stopBitsLabel, 1);
    statusBar()->addWidget(parityLabel, 1);
    statusBar()->addWidget(onTopBox, 1);
    dockInit();
#endif

    stateButton->setMinimumHeight(1);
    stateButton->setStyleSheet("*{text-align:left;}");

    on_advancedBox_clicked(false);
    stateUpdate();
}

void MainWindow::refreshPortsInfo()
{
    ui->portTable->clearContents();
    ui->portBox->clear();
#ifdef Q_OS_ANDROID
    ui->refreshPortsButton->setText(tr("Searching..."));
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
    ui->portTable->setRowCount(arraylen);
    for(int i = 0; i < arraylen; i++)
    {
        QString info = QAndroidJniObject::fromLocalRef(env->GetObjectArrayElement(array.object<jobjectArray>(), i)).toString();
        QString address = info.left(info.indexOf(' '));
        QString name = info.right(info.length() - info.indexOf(' ') - 1);
        qDebug() << address << name;
        ui->portTable->setItem(i, HPortName, new QTableWidgetItem(name));
        ui->portTable->setItem(i, HSystemLocation, new QTableWidgetItem(address));
        ui->portTable->setItem(i, HDescription, new QTableWidgetItem(tr("Bonded")));
        ui->portBox->addItem(address);
    }

    BTdiscoveryAgent->start();
#else
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    ui->portTable->setRowCount(ports.size());
    for(int i = 0; i < ports.size(); i++)
    {
        ui->portTable->setItem(i, HPortName, new QTableWidgetItem(ports[i].portName()));
        ui->portBox->addItem(ports[i].portName());
        ui->portTable->setItem(i, HDescription, new QTableWidgetItem(ports[i].description()));
        ui->portTable->setItem(i, HManufacturer, new QTableWidgetItem(ports[i].manufacturer()));
        ui->portTable->setItem(i, HSerialNumber, new QTableWidgetItem(ports[i].serialNumber()));
        ui->portTable->setItem(i, HIsNull, new QTableWidgetItem(ports[i].isNull() ? "Yes" : "No"));
        ui->portTable->setItem(i, HSystemLocation, new QTableWidgetItem(ports[i].systemLocation()));
        ui->portTable->setItem(i, HVendorID, new QTableWidgetItem(QString::number(ports[i].vendorIdentifier())));
        ui->portTable->setItem(i, HProductID, new QTableWidgetItem(QString::number(ports[i].productIdentifier())));

        QList<qint32> baudRateList = ports[i].standardBaudRates();
        QString baudRates = "";
        for(int j = 0; j < baudRates.size(); j++)
        {
            baudRates += QString::number(baudRateList[j]) + ", ";
        }
        ui->portTable->setItem(i, HBaudRates, new QTableWidgetItem(baudRates));
    }
#endif
}

void MainWindow::on_portTable_cellClicked(int row, int column)
{
    Q_UNUSED(column);
    ui->portBox->setCurrentIndex(row);
#ifndef Q_OS_ANDROID
    QStringList preferences = settings->childGroups();
    QStringList::iterator it;


    // search preference by <vendorID>-<productID>
    QString id = ui->portTable->item(row, HVendorID)->text();  // vendor id
    id += "-";
    id += ui->portTable->item(row, HProductID)->text(); // product id
    for(it = preferences.begin(); it != preferences.end(); it++)
    {
        if(*it == id)
        {
            loadPortPreference(id);
            break;
        }
    }
    if(it != preferences.end())
        return;

    // search preference by PortName
    id = ui->portTable->item(row, HPortName)->text();
    for(it = preferences.begin(); it != preferences.end(); it++)
    {
        if(*it == id)
        {
            loadPortPreference(id);
            break;
        }
    }
#endif
}

void MainWindow::on_advancedBox_clicked(bool checked)
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

void MainWindow::on_openButton_clicked()
{
#ifdef Q_OS_ANDROID
    BTSocket->connectToService(QBluetoothAddress(ui->portBox->currentText()), QBluetoothUuid::SerialPort);
#else
    serialPort->setPortName(ui->portBox->currentText());
    serialPort->setBaudRate(ui->baudRateBox->currentText().toInt());
    serialPort->setDataBits((QSerialPort::DataBits)ui->dataBitsBox->currentData().toInt());
    serialPort->setStopBits((QSerialPort::StopBits)ui->stopBitsBox->currentData().toInt());
    serialPort->setParity((QSerialPort::Parity)ui->parityBox->currentData().toInt());
    serialPort->setFlowControl((QSerialPort::FlowControl)ui->flowControlBox->currentData().toInt());
    if(serialPort->isOpen())
    {
        QMessageBox::warning(this, "Error", "The port has been opened.");
        return;
    }
    if(!serialPort->open(QSerialPort::ReadWrite))
    {
        QMessageBox::warning(this, "Error", tr("Cannot open the serial port."));
        return;
    }
    onIODeviceConnected();
    savePortPreference(serialPort->portName());
#endif
}

void MainWindow::on_closeButton_clicked()
{
    IODevice->close();
    onIODeviceDisconnected();
}

void MainWindow::stateUpdate()
{

    QString portName;
#ifdef Q_OS_ANDROID
    portName = BTSocket->peerName();
#else
    portName = serialPort->portName();
    QString stopbits[4] = {"", tr("OneStop"), tr("TwoStop"), tr("OneAndHalfStop")};
    QString parities[6] = {tr("NoParity"), "", tr("EvenParity"), tr("OddParity"), tr("SpaceParity"), tr("MarkParity")};
    if(IODeviceState)
    {
        baudRateLabel->setText(tr("BaudRate") + ": " + QString::number(serialPort->baudRate()));
        dataBitsLabel->setText(tr("DataBits") + ": " + QString::number(serialPort->dataBits()));
        stopBitsLabel->setText(tr("StopBits") + ": " + stopbits[(int)serialPort->stopBits()]);
        parityLabel->setText(tr("Parity") + ": " + parities[(int)serialPort->parity()]);
    }
    else
    {
        baudRateLabel->setText(tr("BaudRate") + ": ");
        dataBitsLabel->setText(tr("DataBits") + ": ");
        stopBitsLabel->setText(tr("StopBits") + ": ");
        parityLabel->setText(tr("Parity") + ": ");
    }
#endif
    if(IODeviceState)
        stateButton->setText(tr("State") + ": √");
    else
        stateButton->setText(tr("State") + ": X");
    portLabel->setText(tr("Port") + ": " + portName);
    RxLabel->setText(tr("Rx") + ": " + QString::number(rawReceivedData->length()));
    TxLabel->setText(tr("Tx") + ": " +  QString::number(rawSendedData->length()));
}

void MainWindow::onIODeviceConnected()
{


    qDebug() << "IODevice Connected";
    IODeviceState = true;
    updateUITimer->start();
    stateUpdate();
    refreshPortsInfo();
#ifndef Q_OS_ANDROID
    QSerialPort* port;
    port = dynamic_cast<QSerialPort*>(IODevice);
    if(port != nullptr)
    {
        dataTab->setFlowCtrl(port->flowControl() != QSerialPort::HardwareControl, port->isRequestToSend(), port->isDataTerminalReady());
    }
#endif
}

void MainWindow::onIODeviceDisconnected()
{
    qDebug() << "IODevice Disconnected";
    IODeviceState = false;
    updateUITimer->stop();
    stateUpdate();
    refreshPortsInfo();
    updateRxUI();
}

// Rx/Tx Data
// **********************************************************************************************************************************************

void MainWindow::readData()
{
    QByteArray newData = IODevice->readAll();
    if(newData.isEmpty())
        return;
    rawReceivedData->append(newData);
    RxLabel->setText(tr("Rx") + ": " + QString::number(rawReceivedData->length()));
    RxUIBuf->append(newData);
    QApplication::processEvents();
}

void MainWindow::sendData(const QByteArray& data)
{
    if(!IODeviceState)
    {
        QMessageBox::warning(this, tr("Error"), tr("No port is opened."));
        dataTab->setRepeat(false);
        return;
    }
    rawSendedData->append(data);
    dataTab->syncSendedEditWithData();
    IODevice->write(data);
    TxLabel->setText("Tx: " + QString::number(rawSendedData->length()));
}

// TODO:
// use the same RxDecoder for edit/plot
// maybe standalone decoder?
void MainWindow::updateRxUI()
{
    if(RxUIBuf->isEmpty())
        return;
    if(dataTab->getRxRealtimeState())
        dataTab->appendReceivedData(*RxUIBuf);
    plotTab->newData(*RxUIBuf);
    RxUIBuf->clear();
}

// platform specific
// **********************************************************************************************************************************************

#ifdef Q_OS_ANDROID
void MainWindow::BTdiscoverFinished()
{
    ui->refreshPortsButton->setText(tr("Refresh"));
}

void MainWindow::BTdeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    QString address = device.address().toString();
    QString name = device.name();
    int i = ui->portTable->rowCount();
    ui->portTable->setRowCount(i + 1);
    ui->portTable->setItem(i, HPortName, new QTableWidgetItem(name));
    ui->portTable->setItem(i, HSystemLocation, new QTableWidgetItem(address));
    ui->portTable->setItem(i, HDescription, new QTableWidgetItem(tr("Discovered")));
    ui->portBox->addItem(address);
    qDebug() << name
             << address
             << device.isValid()
             << device.rssi()
             << device.majorDeviceClass()
             << device.minorDeviceClass()
             << device.serviceClasses()
             << device.manufacturerData();
}

void MainWindow::onBTConnectionChanged()
{
    if(BTSocket->isOpen())
    {
        onIODeviceConnected();
        BTlastAddress = ui->portBox->currentText();
    }
    else
        onIODeviceDisconnected();
}
#else

void MainWindow::dockInit()
{
    setDockNestingEnabled(true);
    QDockWidget* dock;
    QWidget* widget;
    int count = ui->funcTab->count();
    for(int i = 0; i < count; i++)
    {
        dock = new QDockWidget(ui->funcTab->tabText(0), this);
        qDebug() << "dock name" << ui->funcTab->tabText(0);
        dock->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);// movable is necessary, otherwise the dock cannot be dragged
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        widget = ui->funcTab->widget(0);
        dock->setWidget(widget);
        addDockWidget(Qt::BottomDockWidgetArea, dock);
        if(!dockList.isEmpty())
            tabifyDockWidget(dockList[0], dock);
        dockList.append(dock);
    }
    ui->funcTab->setVisible(false);
    ui->centralwidget->setVisible(false);
    dockList[0]->setVisible(true);
    dockList[0]->raise();
}

void MainWindow::onTopBoxClicked(bool checked)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, checked);
    show();
}

void MainWindow::onSerialErrorOccurred(QSerialPort::SerialPortError error)
{
    qDebug() << error;
    if(error != QSerialPort::NoError && IODeviceState)
    {
        IODevice->close();
        onIODeviceDisconnected();
    }

}

void MainWindow::savePortPreference(const QString& portName)
{
    QSerialPortInfo info(portName);
    QString id;
    if(info.vendorIdentifier() != 0 && info.productIdentifier() != 0)
        id = QString::number(info.vendorIdentifier()) + "-" + QString::number(info.productIdentifier());
    else
        id = portName;
    settings->beginGroup(id);
    settings->setValue("BaudRate", ui->baudRateBox->currentText());
    settings->setValue("DataBitsID", ui->dataBitsBox->currentIndex());
    settings->setValue("StopBitsID", ui->stopBitsBox->currentIndex());
    settings->setValue("ParityID", ui->parityBox->currentIndex());
    settings->setValue("FlowControlID", ui->flowControlBox->currentIndex());
    settings->endGroup();
}

void MainWindow::loadPortPreference(const QString& id)
{
    settings->beginGroup(id);
    ui->baudRateBox->setEditText(settings->value("BaudRate").toString());
    ui->dataBitsBox->setCurrentIndex(settings->value("DataBitsID").toInt());
    ui->stopBitsBox->setCurrentIndex(settings->value("StopBitsID").toInt());
    ui->parityBox->setCurrentIndex(settings->value("ParityID").toInt());
    ui->flowControlBox->setCurrentIndex(settings->value("FlowControlID").toInt());
    settings->endGroup();
}
#endif

