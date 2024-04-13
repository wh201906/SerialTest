#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "filexceiver.h"

#include <QDateTime>
#include <QBluetoothLocalDevice>
#ifdef Q_OS_ANDROID
#include <QAndroidJniEnvironment>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // might not be empty(specified by -stylesheet option)
    m_appDefaultQss = qApp->styleSheet();
    contextMenu = new QMenu();

    IOConnection = new Connection();
    connect(IOConnection, &Connection::connected, this, &MainWindow::onIODeviceConnected);
    connect(IOConnection, &Connection::disconnected, this, &MainWindow::onIODeviceDisconnected);
    connect(IOConnection, QOverload<const QString&>::of(&Connection::connectFailed), this, QOverload<const QString&>::of(&MainWindow::onIODeviceConnectFailed));
    connect(IOConnection, QOverload<const QStringList&>::of(&Connection::connectFailed), this, QOverload<const QStringList&>::of(&MainWindow::onIODeviceConnectFailed));
    connect(IOConnection, &Connection::stateChanged, this, &MainWindow::updateStatusBar);

#ifdef Q_OS_ANDROID
    setStyleSheet("QCheckBox{min-width:15px;min-height:15px;}QCheckBox::indicator{min-width:15px;min-height:15px;}");
#else
    IOConnection->setType(Connection::SerialPort);
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

    dataTab = new DataTab(&rawReceivedData, &RxMetadata, &rawSendedData);
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
    connect(dataTab, &DataTab::clearGraph, plotTab, &PlotTab::onClearSignalReceived);
    connect(plotTab, &PlotTab::clearRxData, dataTab, &DataTab::onRxClearSignalReceived);
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
    connect(settingsTab, &SettingsTab::themeChanged, this, &MainWindow::onThemeChanged);
    connect(settingsTab, &SettingsTab::opacityChanged, this, &MainWindow::onOpacityChanged); // not a slot function, but works fine.
    connect(settingsTab, &SettingsTab::fullScreenStateChanged, this, &MainWindow::setFullScreen);
    connect(settingsTab, &SettingsTab::TouchScrollStateChanged, deviceTab, &DeviceTab::setTouchScroll);
    connect(settingsTab, &SettingsTab::TouchScrollStateChanged, ctrlTab, &CtrlTab::setTouchScroll);
    connect(settingsTab, &SettingsTab::TouchScrollStateChanged, settingsTab, &SettingsTab::setTouchScroll);
    connect(settingsTab, &SettingsTab::updateAvailableDeviceTypes, deviceTab, &DeviceTab::getAvailableTypes);
    connect(settingsTab, &SettingsTab::themeChanged, plotTab, &PlotTab::onThemeChanged);
    connect(settingsTab, &SettingsTab::recordDataChanged, dataTab, &DataTab::onRecordDataChanged);
    connect(settingsTab, &SettingsTab::mergeTimestampChanged, this, &MainWindow::onMergeTimestampChanged);
    connect(settingsTab, &SettingsTab::timestampIntervalChanged, this, &MainWindow::onTimestampIntervalChanged);
    connect(settingsTab, &SettingsTab::clearBehaviorChanged, dataTab, &DataTab::onClearBehaviorChanged);
    connect(settingsTab, &SettingsTab::clearBehaviorChanged, plotTab, &PlotTab::onClearBehaviorChanged);
    ui->funcTab->insertTab(5, settingsTab, tr("Settings"));

    deviceTab->getAvailableTypes(true);
    initTabs();

    updateUITimer = new QTimer();
    updateUITimer->setInterval(20);

    connect(IOConnection, &Connection::readyRead, this, &MainWindow::readData, Qt::QueuedConnection);
    connect(updateUITimer, &QTimer::timeout, this, &MainWindow::updateRxUI);
    connect(stateButton, &QPushButton::clicked, this, &MainWindow::onStateButtonClicked);

    initUI();


    dockAllWindows = new QAction(tr("Dock all windows"), this);
    connect(dockAllWindows, &QAction::triggered, [ = ]()
    {
        for(int i = 0; i < dockList.size(); i++)
            dockList[i]->setFloating(false);
    });
    contextMenu->addAction(dockAllWindows);
    contextMenu->addSeparator();

    myInfo = new QAction("wh201906", this);
    // APP_VERSION is defined in the .pro file
    currVersion = new QAction(tr("Ver: ") + APP_VERSION, this);
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    const QString windowStateData = QString::fromLatin1(saveState().toBase64());
    settings->beginGroup("SerialTest");
    settings->setValue("WindowState", windowStateData);
    settings->endGroup();
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
            QMessageBox::warning(this, tr("Error"), tr("Please connect to something first."));
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

    settings->beginGroup("SerialTest");
    bool dockEnabled = settings->value("Android_Dock", false).toBool();
    settings->endGroup();
    if(dockEnabled)
    {
        dockInit();
    }
#else
    onTopBox = new QCheckBox(tr("On Top"));
    connect(onTopBox, &QCheckBox::clicked, this, &MainWindow::onTopBoxClicked);

    settings->beginGroup("SerialTest");
    bool checked = settings->value("OnTop", false).toBool();
    settings->endGroup();
    onTopBox->setChecked(checked);

    statusBar()->addPermanentWidget(onTopBox, 0);
    QTimer::singleShot(0, this, &MainWindow::onTopBoxClicked); // run it after UI initialization

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
        stateButton->setText(tr("State") + ": OK");
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
    RxMetadata.clear();
    m_RxCount = 0;
    updateRxTxLen(true, false);
}

void MainWindow::setTxDataRecording(bool enabled)
{
    m_TxDataRecording = enabled;
}

void MainWindow::showUpTab(int tabID)
{
    if(ui->funcTab->isVisible())
        ui->funcTab->setCurrentIndex(tabID);
    else
    {
        dockList[tabID]->setVisible(true);
        dockList[tabID]->raise();
    }
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
    Connection::Type type = IOConnection->type();
    if(type == Connection::SerialPort)
    {
        if(serialPinout->getEnableState())
            IOConnection->setPolling(true);

        Connection::SerialPortArgument arg;
        arg = IOConnection->getSerialPortArgument();
        deviceTab->saveSPPreference(arg);
    }
    else if(type == Connection::TCP_Client)
    {
        Connection::NetworkArgument arg;
        // get raw user input
        arg = IOConnection->getNetworkArgument(false, false);
        deviceTab->saveTCPClientPreference(arg);
    }
    else if(type == Connection::UDP)
    {
        Connection::NetworkArgument arg;
        // get raw user input
        arg = IOConnection->getNetworkArgument(false, false);
        deviceTab->saveUDPPreference(arg);
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

void MainWindow::onIODeviceConnectFailed(const QString& info)
{
    Connection::Type type = IOConnection->type();
    QString msg;
    if(type == Connection::SerialPort)
    {
        msg = tr("Cannot open the serial port.");
    }
    else if(type == Connection::BT_Client || type == Connection::BLE_Central || type == Connection::TCP_Client)
    {
        msg = tr("Cannot establish the connection.");
    }
    else if(type == Connection::BT_Server || type == Connection::TCP_Server)
    {
        msg = tr("Cannot start the server.");
    }
    else if(type == Connection::UDP)
    {
        msg = tr("Cannot bind to the specified address and port.");
    }
    if(!info.isEmpty())
        msg += "\n" + info;
    QMessageBox::warning(this, tr("Error"), msg);
}

void MainWindow::onIODeviceConnectFailed(const QStringList& infoList)
{
    QString info;
    for(const QString& str : infoList)
        info += str + "\n";
    onIODeviceConnectFailed(info.trimmed());
}

// Rx/Tx Data
// **********************************************************************************************************************************************

void MainWindow::readData()
{
    QByteArray newData = IOConnection->readAll();
    if(newData.isEmpty())
        return;

    Metadata metadata(rawReceivedData.length(), newData.length(), QDateTime::currentMSecsSinceEpoch());
    if(m_mergeTimestamp && !RxMetadata.isEmpty() && metadata.timestamp - RxMetadata.last().timestamp < m_timestampInterval)
        RxMetadata.last().len += metadata.len;
    else
    {
        RxMetadata.append(metadata);
        RxUIMetadataBuf += metadata;
    }

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
        dataTab->appendReceivedData(RxUIBuf, RxUIMetadataBuf);
    if(plotTab->enabled())
        plotTab->newData(RxUIBuf);
    if(fileTab->receiving())
        fileTab->fileXceiver()->newData(RxUIBuf);
    RxUIBuf.clear();
    RxUIMetadataBuf.clear();
}


void MainWindow::onOpacityChanged(qreal value)
{
    setWindowOpacity(value);
    for(auto dock : qAsConst(dockList))
    {
        if(dock->isFloating())
            dock->setWindowOpacity(value);
    }
}

void MainWindow::onThemeChanged(const QString& themeName)
{
    QFile themeFile;
    QTextStream themeStream;
    QString qssString = qApp->styleSheet(); // default behavior
    if(themeName == "(none)")
        qssString = m_appDefaultQss;
    else if(themeName == "qdss_dark")
    {
        themeFile.setFileName(":/qdarkstyle/dark/darkstyle.qss");
        themeFile.open(QFile::ReadOnly | QFile::Text);
        themeStream.setDevice(&themeFile);
        qssString = themeStream.readAll();
    }
    else if(themeName == "qdss_light")
    {
        themeFile.setFileName(":/qdarkstyle/light/lightstyle.qss");
        themeFile.open(QFile::ReadOnly | QFile::Text);
        themeStream.setDevice(&themeFile);
        qssString = themeStream.readAll();
    }
    qApp->setStyleSheet(qssString);
}

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
        // For saveState()/restoreState()
        dock->setObjectName(widget->objectName() + "DockWidget");
        connect(dock, &QDockWidget::topLevelChanged, this, &MainWindow::onDockTopLevelChanged);
        dock->installEventFilter(this);
        addDockWidget(Qt::BottomDockWidgetArea, dock);
        if(!dockList.isEmpty())
            tabifyDockWidget(dockList[0], dock);
        dockList.append(dock);
    }
    ui->funcTab->hide();
    ui->centralwidget->hide();
    dockList[0]->setVisible(true);
    dockList[0]->raise();

    // Restore the geometry and the state of dock widgets
    settings->beginGroup("SerialTest");
    const QByteArray windowStateData = QByteArray::fromBase64(settings->value("WindowState", "").toString().toLatin1());
    settings->endGroup();
    if(!windowStateData.isEmpty())
    {
        restoreState(windowStateData);
    }
}


void MainWindow::onDockTopLevelChanged(bool topLevel)
{
    if(topLevel) // some widget is floating now
        onOpacityChanged(windowOpacity());
}

void MainWindow::onMergeTimestampChanged(bool enabled)
{
    m_mergeTimestamp = enabled;
}

void MainWindow::onTimestampIntervalChanged(int interval)
{
    m_timestampInterval = interval;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(dockList.contains((QDockWidget*)watched))
    {
        QDockWidget* dock = qobject_cast<QDockWidget*>(watched);
        if(event->type() == QEvent::Close && dock->isFloating())
        {
            // ignore Alt+F4, just dock it.
            dock->setFloating(false);
            event->ignore(); // calling ignore() is necessary for QCloseEvent
            return true;
        }
    }
    return false;
}

// platform specific
// **********************************************************************************************************************************************

#ifndef Q_OS_ANDROID

void MainWindow::onTopBoxClicked()
{
    if(onTopBox == nullptr)
        return;
    bool checked = onTopBox->isChecked();
    setWindowFlag(Qt::WindowStaysOnTopHint, checked);
    show();
    settings->beginGroup("SerialTest");
    settings->setValue("OnTop", checked);
    settings->endGroup();
}

#endif

