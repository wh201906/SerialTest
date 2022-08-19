#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "filexceiver.h"

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
    connect(IOConnection, &Connection::stateChanged, this, &MainWindow::updateStatusBar);

#ifdef Q_OS_ANDROID
    setStyleSheet("QCheckBox{min-width:15px;min-height:15px;}QCheckBox::indicator{min-width:15px;min-height:15px;}");
#else

    IOConnection->setType(Connection::SerialPort);

    onTopBox = new QCheckBox(tr("On Top"));
    connect(onTopBox, &QCheckBox::clicked, this, &MainWindow::onTopBoxClicked);

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
    stateButton = new QPushButton();
    TxLabel = new QLabel();
    RxLabel = new QLabel();
    connArgsLabel = new QLabel;
    serialPinout = new SerialPinout();
    connect(IOConnection, &Connection::SP_signalsChanged, serialPinout, &SerialPinout::setPinout);
    connect(serialPinout, &SerialPinout::enableStateChanged, IOConnection, &Connection::setPolling);
    serialPinout->initSettings();

    deviceTab = new DeviceTab();
    deviceTab->setConnection(IOConnection);
    connect(deviceTab, &DeviceTab::connTypeChanged, this, &MainWindow::updateStatusBar);
    connect(deviceTab, &DeviceTab::connTypeChanged, this, &MainWindow::updateWindowTitle);
    connect(deviceTab, &DeviceTab::argumentChanged, this, &MainWindow::updateStatusBar);
    connect(deviceTab, &DeviceTab::clientCountChanged, this, &MainWindow::updateStatusBar);
    connect(IOConnection, &Connection::BT_clientConnected, deviceTab, &DeviceTab::onClientCountChanged);
    connect(IOConnection, &Connection::TCP_clientConnected, deviceTab, &DeviceTab::onClientCountChanged);
    connect(IOConnection, &Connection::BT_clientDisconnected, deviceTab, &DeviceTab::onClientCountChanged);
    connect(IOConnection, &Connection::TCP_clientDisconnected, deviceTab, &DeviceTab::onClientCountChanged);
    ui->funcTab->insertTab(0, deviceTab, tr("Connect"));

    dataTab = new DataTab(&rawReceivedData, &rawSendedData);
    dataTab->setConnection(IOConnection);
    connect(deviceTab, &DeviceTab::connTypeChanged, dataTab, &DataTab::onConnTypeChanged);
    connect(dataTab, &DataTab::send, this, &MainWindow::sendData);
    connect(dataTab, &DataTab::updateRxTxLen, this, &MainWindow::updateRxTxLen);
    connect(dataTab, &DataTab::clearReceivedData, this, &MainWindow::clearReceivedData);
    connect(dataTab, &DataTab::clearSendedData, this, &MainWindow::clearSendedData);
    connect(dataTab, &DataTab::setTxDataRecording, this, &MainWindow::setTxDataRecording);
    connect(dataTab, &DataTab::showUpTab, this, &MainWindow::showUpTab);
    ui->funcTab->insertTab(1, dataTab, tr("Data"));

    plotTab = new PlotTab();
    connect(dataTab, &DataTab::setPlotDecoder, plotTab, &PlotTab::setDecoder);
    ui->funcTab->insertTab(2, plotTab, tr("Plot"));

    ctrlTab = new CtrlTab();
    connect(ctrlTab, &CtrlTab::send, this, &MainWindow::sendData);
    connect(dataTab, &DataTab::setDataCodec, ctrlTab, &CtrlTab::setDataCodec);
    ui->funcTab->insertTab(3, ctrlTab, tr("Control"));

    fileTab = new FileTab();
    connect(fileTab, &FileTab::showUpTab, this, &MainWindow::showUpTab);
    connect(fileTab->fileXceiver(), &FileXceiver::send, this, &MainWindow::sendData);
    ui->funcTab->insertTab(4, fileTab, tr("File"));

    settingsTab = new SettingsTab();
    connect(settingsTab, &SettingsTab::opacityChanged, this, &MainWindow::onOpacityChanged); // not a slot function, but works fine.
    connect(settingsTab, &SettingsTab::fullScreenStateChanged, this, &MainWindow::setFullScreen);
    ui->funcTab->insertTab(5, settingsTab, tr("Settings"));

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
    fileTab->initSettings();
    settingsTab->initSettings();
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    contextMenu->exec(event->globalPos());
}

void MainWindow::keyReleaseEvent(QKeyEvent* e)
{
    if(e->key() == Qt::Key_Back)
    {
#ifdef Q_OS_ANDROID
        // press Key_Back twice to exit, rather than once
        qint64 currTick = QDateTime::currentMSecsSinceEpoch();
        if(currTick - m_keyBackTick > 3000)
        {
            // block the first release of Key_Back
            m_keyBackTick = currTick;
            Util::showToast(tr("Press Back again to exit."));
        }
        else // exit
        {
            Util::showToast(tr("Closing...")); // exit might be blocked by FileTab for 3s
            QMainWindow::keyReleaseEvent(e);
        }
#endif
    }
    else // bypass
        QMainWindow::keyReleaseEvent(e);
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
    statusBar()->addPermanentWidget(stateButton, 0);
    statusBar()->addPermanentWidget(connArgsLabel, 1);
    statusBar()->addPermanentWidget(RxLabel, 0);
    statusBar()->addPermanentWidget(TxLabel, 0);
    statusBar()->addPermanentWidget(serialPinout, 0);
#ifdef Q_OS_ANDROID

    // Strange resize behavior on Android
    // Need a fixed size
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(QApplication::primaryScreen()->availableGeometry().size());

    QtAndroid::androidActivity().callMethod<void>("handleStartIntent");
#else
    statusBar()->addPermanentWidget(onTopBox, 0);
    dockInit();
#endif
    stateButton->setMinimumHeight(1);
    stateButton->setStyleSheet("*{text-align:left;}");

    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    Connection::Type type;
    QString connArgsText;
    type = IOConnection->type();
    if(type == Connection::SerialPort)
    {
        serialPinout->show();
        if(IOConnection->isConnected())
        {
            const QString stopbits[4] = {"", tr("OneStop"), tr("TwoStop"), tr("OneAndHalfStop")};
            const QString parities[6] = {tr("NoParity"), "", tr("EvenParity"), tr("OddParity"), tr("SpaceParity"), tr("MarkParity")};

            Connection::SerialPortArgument arg = IOConnection->getSerialPortArgument();
            connArgsText.append((tr("Port") + ": %1 ").arg(arg.name));
            connArgsText.append((tr("BaudRate") + ": %1 ").arg(arg.baudRate));
            connArgsText.append((tr("DataBits") + ": %1 ").arg(arg.dataBits));
            connArgsText.append(tr("StopBits") + ": " + stopbits[(int)arg.stopBits] + " ");
            connArgsText.append(tr("Parity") + ": " + parities[(int)arg.parity] + " ");
            // the value of flowcontrol is not specified
        }
    }
    else if(type == Connection::BT_Client || type == Connection::BLE_Central)
    {
        serialPinout->hide();
        if(IOConnection->isConnected())
        {
            QString remoteName = IOConnection->BT_remoteName();
            connArgsText.append((tr("Remote") + ": %1 ").arg(IOConnection->getBTArgument().deviceAddress.toString()));
            if(!remoteName.isEmpty())
                connArgsText.append((tr("Remote Name") + ": %1 ").arg(IOConnection->BT_remoteName()));
#ifdef Q_OS_ANDROID
            if(IOConnection->BT_localAddress().toString() != "02:00:00:00:00:00")
#else
            if(true)
#endif
            {
                connArgsText.append((tr("Local") + ": %1 ").arg(IOConnection->BT_localAddress().toString()));
            }
        }
    }
    else if(type == Connection::BT_Server)
    {
        serialPinout->hide();
        if(IOConnection->state() != Connection::Unconnected)
        {
#ifdef Q_OS_ANDROID
            if(IOConnection->BT_localAddress().toString() != "02:00:00:00:00:00")
#else
            if(true)
#endif
            {
                connArgsText.append((tr("Local") + ": %1 ").arg(IOConnection->BT_localAddress().toString()));
            }
        }
        connArgsText.append((tr("Connected Clients") + ": %1 ").arg(IOConnection->BTServer_clientCount()));
    }
    else if(type == Connection::BLE_Peripheral)
    {
        serialPinout->hide();
    }
    else if(type == Connection::TCP_Client)
    {
        serialPinout->hide();
        if(IOConnection->isConnected())
        {
            Connection::NetworkArgument netArg = IOConnection->getNetworkArgument();
            connArgsText.append((tr("Local") + ": (%1, %2) ").arg(netArg.localAddress.toString()).arg(netArg.localPort));
            connArgsText.append((tr("Remote") + ": (%1, %2) ").arg(netArg.remoteName).arg(netArg.remotePort));
        }
    }
    else if(type == Connection::TCP_Server)
    {
        serialPinout->hide();
        if(IOConnection->state() != Connection::Unconnected)
        {
            Connection::NetworkArgument netArg = IOConnection->getNetworkArgument();
            QString localAddr = netArg.localAddress == QHostAddress::Any ? tr("Any") : netArg.localAddress.toString();
            connArgsText.append((tr("Local") + ": (%1, %2) ").arg(localAddr).arg(netArg.localPort));
        }
        connArgsText.append((tr("Connected Clients") + ": %1 ").arg(IOConnection->TCPServer_clientCount()));
    }
    else if(type == Connection::UDP)
    {
        serialPinout->hide();
        Connection::NetworkArgument netArg = IOConnection->getNetworkArgument();
        if(IOConnection->isConnected())
        {
            QString localAddr = netArg.localAddress == QHostAddress::Any ? tr("Any") : netArg.localAddress.toString();
            connArgsText.append((tr("Local") + ": (%1, %2) ").arg(localAddr).arg(netArg.localPort));
        }
        connArgsText.append((tr("Remote") + ": (%1, %2) ").arg(netArg.remoteName).arg(netArg.remotePort));
    }
    connArgsLabel->setText(connArgsText);
    Connection::State currState = IOConnection->state();
    if(currState == Connection::Connected)
        stateButton->setText(tr("State") + ": √");
    else if(currState == Connection::Bound || currState == Connection::Connecting)
        stateButton->setText(tr("State") + ": ...");
    else
        stateButton->setText(tr("State") + ": X");
    updateRxTxLen();
}

void MainWindow::updateWindowTitle(Connection::Type type)
{
    setWindowTitle("SerialTest - " + Connection::getTypeName(type));
}

void MainWindow::clearSendedData()
{
    rawSendedData.clear();
    m_TxCount = 0;
    updateRxTxLen(false, true);
}

void MainWindow::clearReceivedData()
{
    rawReceivedData.clear();
    m_RxCount = 0;
    updateRxTxLen(true, false);
}

void MainWindow::setTxDataRecording(bool enabled)
{
    m_TxDataRecording = enabled;
}

void MainWindow::showUpTab(int id)
{
#ifdef Q_OS_ANDROID
    ui->funcTab->setCurrentIndex(id);
#else
    dockList[id]->setVisible(true);
    dockList[id]->raise();
#endif
}

void MainWindow::setFullScreen(bool isFullScreen)
{
    if(isFullScreen)
    {
        setWindowState(windowState() | Qt::WindowFullScreen);
        showFullScreen();
    }
    else
    {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        showNormal();
        show();
    }
}

void MainWindow::updateRxTxLen(bool updateRx, bool updateTx)
{
    if(updateRx)
        RxLabel->setText(tr("Rx") + ": " + QString::number(m_RxCount));
    if(updateTx)
        TxLabel->setText(tr("Tx") + ": " + QString::number(m_TxCount));
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
        deviceTab->saveSPPreference(arg);
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
    rawReceivedData += newData;
    m_RxCount += newData.length();
    updateRxTxLen(true, false);
    RxUIBuf += newData;
    QApplication::processEvents();
}

void MainWindow::sendData(const QByteArray& data)
{
    if(!IOConnection->isConnected())
    {
        QMessageBox::warning(this, tr("Error"), tr("No port is opened."));
        dataTab->setRepeat(false);
        fileTab->stop();
        return;
    }
    qint64 len = IOConnection->write(data);
    // this happens if an error occurred,
    // or the Tx switch of all clients are disabled.
    if(len <= 0)
        return;
    if(m_TxDataRecording)
    {
        rawSendedData += data;
        dataTab->appendSendedData(data);
    }
    m_TxCount += len;
    updateRxTxLen(false, true);
}

// TODO:
// use the same RxDecoder for edit/plot
// maybe standalone decoder?
void MainWindow::updateRxUI()
{
    if(RxUIBuf.isEmpty())
        return;
    if(dataTab->getRxRealtimeState())
        dataTab->appendReceivedData(RxUIBuf);
    if(plotTab->enabled())
        plotTab->newData(RxUIBuf);
    if(fileTab->receiving())
        fileTab->fileXceiver()->newData(RxUIBuf);
    RxUIBuf.clear();
}


void MainWindow::onOpacityChanged(qreal value)
{
    setWindowOpacity(value);
#ifndef Q_OS_ANDROID
    for(auto dock : qAsConst(dockList))
    {
        if(dock->isFloating())
            dock->setWindowOpacity(value);
    }
#endif
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
        connect(dock, &QDockWidget::topLevelChanged, this, &MainWindow::onDockTopLevelChanged);
        addDockWidget(Qt::BottomDockWidgetArea, dock);
        if(!dockList.isEmpty())
            tabifyDockWidget(dockList[0], dock);
        dockList.append(dock);
    }
    ui->funcTab->hide();
    ui->centralwidget->hide();
    dockList[0]->setVisible(true);
    dockList[0]->raise();
}

void MainWindow::onTopBoxClicked(bool checked)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, checked);
    show();
}

void MainWindow::onDockTopLevelChanged(bool topLevel)
{
    if(topLevel) // some widget is floating now
        onOpacityChanged(windowOpacity());
}

#endif

