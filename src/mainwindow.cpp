#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QBluetoothLocalDevice>
#ifdef Q_OS_ANDROID
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
    setStyleSheet("QCheckBox{min-width:15px;min-height:15px;}QCheckBox::indicator{min-width:15px;min-height:15px;}");

    // on Android, use default.
    MySettings::init(QSettings::NativeFormat);
#else

    IOConnection->setType(Connection::SerialPort);

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
    deviceLabel = new QLabel();
    stateButton = new QPushButton();
    TxLabel = new QLabel();
    RxLabel = new QLabel();
    connArgsLabel = new QLabel;
    serialPinout = new SerialPinout();
    connect(IOConnection, &Connection::SP_signalsChanged, serialPinout, &SerialPinout::setPinout);
    connect(serialPinout, &SerialPinout::enableStateChanged, IOConnection, &Connection::setPolling);

    rawReceivedData = new QByteArray();
    rawSendedData = new QByteArray();
    RxUIBuf = new QByteArray();

    deviceTab = new DeviceTab();
    deviceTab->setConnection(IOConnection);
    connect(deviceTab, &DeviceTab::connTypeChanged, this, &MainWindow::updateStatusBar);
    connect(IOConnection, &Connection::stateChanged, this, &MainWindow::updateStatusBar);
    ui->funcTab->insertTab(0, deviceTab, tr("Connect"));
    dataTab = new DataTab(rawReceivedData, rawSendedData);
    dataTab->setConnection(IOConnection);
    connect(deviceTab, &DeviceTab::connTypeChanged, dataTab, &DataTab::onConnTypeChanged);
    connect(dataTab, &DataTab::send, this, &MainWindow::sendData);
    ui->funcTab->insertTab(1, dataTab, tr("Data"));
    plotTab = new PlotTab();
    connect(dataTab, &DataTab::setPlotDecoder, plotTab, &PlotTab::setDecoder);
    ui->funcTab->insertTab(2, plotTab, tr("Plot"));
    ctrlTab = new CtrlTab();
    connect(ctrlTab, &CtrlTab::send, this, &MainWindow::sendData);
    connect(dataTab, &DataTab::setDataCodec, ctrlTab, &CtrlTab::setDataCodec);
    ui->funcTab->insertTab(3, ctrlTab, tr("Control"));
    deviceTab->getAvailableTypes(true);
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
    if(IOConnection->state() != Connection::Unconnected)
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
    statusBar()->addPermanentWidget(deviceLabel, 1);
    statusBar()->addPermanentWidget(stateButton, 1);
    statusBar()->addPermanentWidget(RxLabel, 1);
    statusBar()->addPermanentWidget(TxLabel, 1);
    statusBar()->addPermanentWidget(connArgsLabel, 1);
    statusBar()->addPermanentWidget(serialPinout, 1);
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
    statusBar()->addPermanentWidget(onTopBox, 1);
    dockInit();
#endif
    stateButton->setMinimumHeight(1);
    stateButton->setStyleSheet("*{text-align:left;}");

    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    Connection::Type type;
    type = IOConnection->type();
    if(type == Connection::SerialPort)
    {
        serialPinout->show();
        Connection::SerialPortArgument arg;
        arg = IOConnection->getSerialPortArgument();
        const QString stopbits[4] = {"", tr("OneStop"), tr("TwoStop"), tr("OneAndHalfStop")};
        const QString parities[6] = {tr("NoParity"), "", tr("EvenParity"), tr("OddParity"), tr("SpaceParity"), tr("MarkParity")};
        if(IOConnection->isConnected())
        {
            deviceLabel->setText(tr("Port") + ": " + arg.name);
            QString text;
            text.append((tr("BaudRate") + ": %1 ").arg(arg.baudRate));
            text.append((tr("DataBits") + ": %1 ").arg(arg.dataBits));
            text.append(tr("StopBits") + ": " + stopbits[(int)arg.stopBits] + " ");
            text.append(tr("Parity") + ": " + parities[(int)arg.parity] + " ");
            // the value of flowcontrol is not specified
            connArgsLabel->setText(text);
        }
        else
        {
            deviceLabel->setText(tr("Port") + ": ");
            connArgsLabel->setText("");
        }
    }
    else if(type == Connection::BT_Client)
    {
        serialPinout->hide();
        if(IOConnection->isConnected())
        {
            deviceLabel->setText(tr("Address") + ": " + IOConnection->getBTArgument().deviceAddress.toString());
            QString text;
            text.append((tr("Device Name") + ": %1 ").arg(IOConnection->BTClient_remoteName()));
#ifdef Q_OS_ANDROID
            if(IOConnection->BT_localAddress().toString() != "02:00:00:00:00:00")
#else
            if(true)
#endif
                text.append((tr("Local") + ": %1 ").arg(IOConnection->BT_localAddress().toString()));
            connArgsLabel->setText(text);
        }
        else
        {
            deviceLabel->setText(tr("Address") + ": ");
            connArgsLabel->setText("");
        }
    }
    Connection::State currState = IOConnection->state();
    if(currState == Connection::Connected)
        stateButton->setText(tr("State") + ": √");
    else if(currState == Connection::Bound || currState == Connection::Connecting)
        stateButton->setText(tr("State") + ": ...");
    else
        stateButton->setText(tr("State") + ": X");
    updateRxTxLen();
}

void MainWindow::updateRxTxLen(bool updateRx, bool updateTx)
{
    if(updateRx)
        RxLabel->setText(tr("Rx") + ": " + QString::number(rawReceivedData->length()));
    if(updateTx)
        TxLabel->setText(tr("Tx") + ": " + QString::number(rawSendedData->length()));
}

void MainWindow::onIODeviceConnected()
{
    qDebug() << "IODevice Connected";
    updateUITimer->start();
    if(IOConnection->type() == Connection::SerialPort)
    {
        if(serialPinout->getEnableState())
            IOConnection->setPolling(true);

        Connection::SerialPortArgument arg;
        arg = IOConnection->getSerialPortArgument();
        deviceTab->saveDevicesPreference(arg.name);
    }
    updateStatusBar();
    dataTab->onConnEstablished();
}

void MainWindow::onIODeviceDisconnected()
{
    qDebug() << "IODevice Disconnected";
    updateUITimer->stop();
    updateStatusBar();
    updateRxUI();
}

void MainWindow::onIODeviceConnectFailed()
{
    if(IOConnection->type() == Connection::SerialPort)
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open the serial port."));
    }
    if(IOConnection->type() == Connection::BT_Client)
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot establish the connection."));
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
    updateRxTxLen(true, false);
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
    IOConnection->write(data);
    rawSendedData->append(data);
    dataTab->appendSendedData(data);
    updateRxTxLen(false, true);
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

