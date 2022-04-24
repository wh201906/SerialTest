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

    IOConnection = new Connection();
    connect(IOConnection, &Connection::connected, this, &MainWindow::onIODeviceConnected);
    connect(IOConnection, &Connection::disconnected, this, &MainWindow::onIODeviceDisconnected);
    connect(IOConnection, &Connection::connectFailed, this, &MainWindow::onIODeviceConnectFailed);
#ifdef Q_OS_ANDROID
    QBluetoothLocalDevice lDevice;
    if(!lDevice.isValid())
        QMessageBox::information(this, tr("Error"), tr("Bluetooth is invalid!"));
    else if(lDevice.hostMode() == QBluetoothLocalDevice::HostPoweredOff)
    {
        QMessageBox::information(this, tr("Error"), tr("Please enable Bluetooth!"));
        lDevice.powerOn();
    }
    IOConnection->setType(Connection::BT_Client);
    setStyleSheet("QCheckBox{min-width:15px;min-height:15px;}QCheckBox::indicator{min-width:15px;min-height:15px;}");

    // on Android, use default.
    MySettings::init(QSettings::NativeFormat);

#else

    IOConnection->setType(Connection::SerialPort);

    baudRateLabel = new QLabel();
    dataBitsLabel = new QLabel();
    stopBitsLabel = new QLabel();
    parityLabel = new QLabel();
    serialPinout = new SerialPinout();
    connect(IOConnection, &Connection::SP_signalsChanged, serialPinout, &SerialPinout::setPinout);
    onTopBox = new QCheckBox(tr("On Top"));
    connect(serialPinout, &SerialPinout::enableStateChanged, IOConnection, &Connection::setPolling);
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
    deviceLabel = new QLabel();
    stateButton = new QPushButton();
    TxLabel = new QLabel();
    RxLabel = new QLabel();

    rawReceivedData = new QByteArray();
    rawSendedData = new QByteArray();
    RxUIBuf = new QByteArray();

    deviceTab = new DeviceTab();
    connect(deviceTab, &DeviceTab::closeDevice, this, &MainWindow::closeDevice);
    connect(deviceTab, &DeviceTab::openDevice, this, &MainWindow::openDevice);
    ui->funcTab->insertTab(0, deviceTab, tr("Port"));
    dataTab = new DataTab(rawReceivedData, rawSendedData);
    dataTab->setIODevice(IOConnection);
    connect(dataTab, &DataTab::setRxLabelText, RxLabel, &QLabel::setText);
    connect(dataTab, &DataTab::setTxLabelText, TxLabel, &QLabel::setText);
    connect(dataTab, &DataTab::send, this, &MainWindow::sendData);
    ui->funcTab->insertTab(1, dataTab, tr("Data"));
    plotTab = new PlotTab();
    connect(dataTab, &DataTab::setPlotDecoder, plotTab, &PlotTab::setDecoder);
    ui->funcTab->insertTab(2, plotTab, tr("Plot"));
    ctrlTab = new CtrlTab();
    connect(ctrlTab, &CtrlTab::send, this, &MainWindow::sendData);
    connect(dataTab, &DataTab::setDataCodec, ctrlTab, &CtrlTab::setDataCodec);
    ui->funcTab->insertTab(3, ctrlTab, tr("Control"));
    initTabs();

    updateUITimer = new QTimer();
    updateUITimer->setInterval(20);

    connect(IOConnection, &Connection::readyRead, this, &MainWindow::readData, Qt::QueuedConnection);


    connect(updateUITimer, &QTimer::timeout, this, &MainWindow::updateRxUI);
    connect(stateButton, &QPushButton::clicked, this, &MainWindow::onStateButtonClicked);

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

void MainWindow::initTabs()
{
    // these functions must be called after class initialization with fixed order
    deviceTab->initSettings();
    dataTab->initSettings();
    plotTab->initQCP();
    plotTab->initSettings();
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    contextMenu->exec(event->globalPos());
}

void MainWindow::onStateButtonClicked()
{
    if(IOConnection->isConnected())
    {
        IOConnection->close();
    }
    else
    {
        if(!IOConnection->reopen())
        {
            QMessageBox::warning(this, tr("Error"), tr("Plz connect to a port first."));
            return;
        }
    }
}

void MainWindow::initUI()
{
    statusBar()->addWidget(deviceLabel, 1);
    statusBar()->addWidget(stateButton, 1);
    statusBar()->addWidget(RxLabel, 1);
    statusBar()->addWidget(TxLabel, 1);
#ifdef Q_OS_ANDROID
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
    statusBar()->addWidget(baudRateLabel, 1);
    statusBar()->addWidget(dataBitsLabel, 1);
    statusBar()->addWidget(stopBitsLabel, 1);
    statusBar()->addWidget(parityLabel, 1);
    statusBar()->addWidget(serialPinout, 1);
    statusBar()->addWidget(onTopBox, 1);
    dockInit();
#endif

    stateButton->setMinimumHeight(1);
    stateButton->setStyleSheet("*{text-align:left;}");

    stateUpdate();
}

#ifdef Q_OS_ANDROID
void MainWindow::openDevice(const QString &name)
{
    Connection::BTArgument arg;
    arg.deviceAddress = QBluetoothAddress(name);
    IOConnection->setArgument(arg);
    IOConnection->open();
}
#else
void MainWindow::openDevice(const QString& name, const qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits, QSerialPort::Parity parity, QSerialPort::FlowControl flowControl)
{
    if(IOConnection->isConnected())
    {
        QMessageBox::warning(this, tr("Error"), tr("The port has been opened."));
        return;
    }
    Connection::SerialPortArgument arg;
    arg.name = name;
    arg.baudRate = baudRate;
    arg.dataBits = dataBits;
    arg.stopBits = stopBits;
    arg.parity = parity;
    arg.flowControl = flowControl;
    IOConnection->setArgument(arg);
    IOConnection->open();
}
#endif

void MainWindow::closeDevice()
{
    IOConnection->close();
}

void MainWindow::stateUpdate()
{
    QString deviceName;
    Connection::Type type;
    type = IOConnection->type();
    if(type == Connection::SerialPort)
    {
        Connection::SerialPortArgument arg;
        arg = IOConnection->getSerialPortArgument();
        deviceName = arg.name;
        const QString stopbits[4] = {"", tr("OneStop"), tr("TwoStop"), tr("OneAndHalfStop")};
        const QString parities[6] = {tr("NoParity"), "", tr("EvenParity"), tr("OddParity"), tr("SpaceParity"), tr("MarkParity")};
#ifndef Q_OS_ANDROID
        if(IOConnection->isConnected())
        {
            baudRateLabel->setText(tr("BaudRate") + ": " + QString::number(arg.baudRate));
            dataBitsLabel->setText(tr("DataBits") + ": " + QString::number(arg.dataBits));
            stopBitsLabel->setText(tr("StopBits") + ": " + stopbits[(int)arg.stopBits]);
            parityLabel->setText(tr("Parity") + ": " + parities[(int)arg.parity]);
        }
        else
        {
            baudRateLabel->setText(tr("BaudRate") + ": ");
            dataBitsLabel->setText(tr("DataBits") + ": ");
            stopBitsLabel->setText(tr("StopBits") + ": ");
            parityLabel->setText(tr("Parity") + ": ");
        }
#endif
    }
    else if(type == Connection::BT_Client)
    {
        if(IOConnection->isConnected())
            deviceName = IOConnection->getBTArgument().deviceAddress.toString();
    }
    if(IOConnection->isConnected())
        stateButton->setText(tr("State") + ": √");
    else
        stateButton->setText(tr("State") + ": X");
    deviceLabel->setText(tr("Port") + ": " + deviceName);
    RxLabel->setText(tr("Rx") + ": " + QString::number(rawReceivedData->length()));
    TxLabel->setText(tr("Tx") + ": " +  QString::number(rawSendedData->length()));
}

void MainWindow::onIODeviceConnected()
{
    qDebug() << "IODevice Connected";
    updateUITimer->start();
#ifndef Q_OS_ANDROID
    if(serialPinout->getEnableState())
        IOConnection->setPolling(true);
#endif
    stateUpdate();
    deviceTab->refreshDevicesInfo();
#ifndef Q_OS_ANDROID
    Connection::SerialPortArgument arg;
    arg = IOConnection->getSerialPortArgument();
    dataTab->setFlowCtrl(arg.flowControl != QSerialPort::HardwareControl, IOConnection->SP_isRequestToSend(), IOConnection->SP_isDataTerminalReady());

    deviceTab->saveDevicesPreference(arg.name);
#endif
}

void MainWindow::onIODeviceDisconnected()
{
    qDebug() << "IODevice Disconnected";
    updateUITimer->stop();
    stateUpdate();
    deviceTab->refreshDevicesInfo();
    updateRxUI();
}

void MainWindow::onIODeviceConnectFailed()
{
    if(IOConnection->type() == Connection::SerialPort)
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open the serial port."));
    }
}

// Rx/Tx Data
// **********************************************************************************************************************************************

void MainWindow::readData()
{
    QByteArray newData = IOConnection->readAll();
    if(newData.isEmpty())
        return;
    rawReceivedData->append(newData);
    RxLabel->setText(tr("Rx") + ": " + QString::number(rawReceivedData->length()));
    RxUIBuf->append(newData);
    QApplication::processEvents();
}

void MainWindow::sendData(const QByteArray& data)
{
    if(!IOConnection->isConnected())
    {
        QMessageBox::warning(this, tr("Error"), tr("No port is opened."));
        dataTab->setRepeat(false);
        return;
    }
    rawSendedData->append(data);
    dataTab->syncSendedEditWithData();
    IOConnection->write(data);
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

#endif

