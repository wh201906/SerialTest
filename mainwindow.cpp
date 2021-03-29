#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    port = new QSerialPort();
    info = new QSerialPortInfo();
    portLabel = new QLabel();
    stateLabel = new QLabel();
    baudRateLabel = new QLabel();
    dataBitsLabel = new QLabel();
    stopBitsLabel = new QLabel();
    parityLabel = new QLabel();
    TxLabel = new QLabel();
    RxLabel = new QLabel();
    portState = false;

    rawReceivedData = new QByteArray();
    rawSendedData = new QByteArray();

    repeatTimer = new QTimer();

    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPortsInfo);
    connect(port, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(ui->sendEdit, &QLineEdit::returnPressed, this, &MainWindow::on_sendButton_clicked);
    connect(port, &QSerialPort::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(repeatTimer, &QTimer::timeout, this, &MainWindow::on_sendButton_clicked);

    RxSlider = ui->receivedEdit->verticalScrollBar();
    connect(RxSlider, &QScrollBar::valueChanged, this, &MainWindow::onRxSliderValueChanged);
    connect(RxSlider, &QScrollBar::sliderMoved, this, &MainWindow::onRxSliderMoved);

    settings = new QSettings("preference.ini", QSettings::IniFormat);

    refreshPortsInfo();
    initUI();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onRxSliderValueChanged(int value)
{

    qDebug() << "valueChanged" << value;
    if(userRequiredRxSliderPos == value)
        currRxSliderPos = value;
    else
        RxSlider->setSliderPosition(currRxSliderPos);
}

void MainWindow::onRxSliderMoved(int value)
{
    // slider is moved by user
    qDebug() << "sliderMoved" << value;
    userRequiredRxSliderPos = value;
}

void MainWindow::readData()
{
    QByteArray newData = port->readAll();
    rawReceivedData->append(newData);
    syncEditWithData();
    if(ui->receivedLatestBox->isChecked())
    {
        userRequiredRxSliderPos = RxSlider->maximum();
        RxSlider->setSliderPosition(RxSlider->maximum());
    }
    else
    {
        userRequiredRxSliderPos = currRxSliderPos;
        RxSlider->setSliderPosition(currRxSliderPos);
    }
    RxLabel->setText("Rx: " + QString::number(rawReceivedData->length()));
}

void MainWindow::initUI()
{
    ui->flowControlBox->addItem("NoFlowControl");
    ui->flowControlBox->addItem("HardwareControl");
    ui->flowControlBox->addItem("SoftwareControl");
    ui->flowControlBox->setItemData(0, QSerialPort::NoFlowControl);
    ui->flowControlBox->setItemData(1, QSerialPort::HardwareControl);
    ui->flowControlBox->setItemData(2, QSerialPort::SoftwareControl);
    ui->parityBox->addItem("NoParity");
    ui->parityBox->addItem("EvenParity");
    ui->parityBox->addItem("OddParity");
    ui->parityBox->addItem("SpaceParity");
    ui->parityBox->addItem("MarkParity");
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

    statusBar()->addWidget(portLabel, 1);
    statusBar()->addWidget(stateLabel, 1);
    statusBar()->addWidget(baudRateLabel, 1);
    statusBar()->addWidget(dataBitsLabel, 1);
    statusBar()->addWidget(stopBitsLabel, 1);
    statusBar()->addWidget(parityLabel, 1);
    statusBar()->addWidget(RxLabel, 1);
    statusBar()->addWidget(TxLabel, 1);

    on_advancedBox_clicked(false);
    stateUpdate();
//    qDebug() << port->isOpen() << port->isReadable() << port->isWritable() << port->error();

    dockInit();
}

void MainWindow::refreshPortsInfo()
{
    ui->portTable->clearContents();
    ui->portBox->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    ui->portTable->setRowCount(ports.size());
    for(int i = 0; i < ports.size(); i++)
    {
        ui->portTable->setItem(i, 0, new QTableWidgetItem(ports[i].portName()));
        ui->portBox->addItem(ports[i].portName());
        ui->portTable->setItem(i, 1, new QTableWidgetItem(ports[i].description()));
        ui->portTable->setItem(i, 2, new QTableWidgetItem(ports[i].manufacturer()));
        ui->portTable->setItem(i, 3, new QTableWidgetItem(ports[i].serialNumber()));
        ui->portTable->setItem(i, 4, new QTableWidgetItem(ports[i].isBusy() ? "Yes" : "No"));
        ui->portTable->setItem(i, 5, new QTableWidgetItem(ports[i].isNull() ? "Yes" : "No"));
        ui->portTable->setItem(i, 6, new QTableWidgetItem(ports[i].systemLocation()));
        ui->portTable->setItem(i, 7, new QTableWidgetItem(QString::number(ports[i].vendorIdentifier())));
        ui->portTable->setItem(i, 8, new QTableWidgetItem(QString::number(ports[i].productIdentifier())));

        QList<qint32> baudRateList = ports[i].standardBaudRates();
        QString baudRates = "";
        for(int j = 0; j < baudRates.size(); j++)
        {
            baudRates += QString::number(baudRateList[j]) + ", ";
        }
        ui->portTable->setItem(i, 9, new QTableWidgetItem(baudRates));
    }
}

void MainWindow::on_portTable_cellDoubleClicked(int row, int column)
{
    QStringList preferences = settings->childGroups();
    QStringList::iterator it;
    ui->portBox->setCurrentIndex(row);

    // search preference by <vendorID>-<productID>
    QString id = ui->portTable->item(row, 7)->text();  // vendor id
    id += "-";
    id += ui->portTable->item(row, 8)->text(); // product id
    for(it = preferences.begin(); it != preferences.end(); it++)
    {
        if(*it == id)
        {
            loadPreference(id);
            break;
        }
    }
    if(it != preferences.end())
        return;

    // search preference by PortName
    id = ui->portTable->item(row, 0)->text();
    for(it = preferences.begin(); it != preferences.end(); it++)
    {
        if(*it == id)
        {
            loadPreference(id);
            break;
        }
    }
}

void MainWindow::loadPreference(const QString& id)
{
    settings->beginGroup(id);
    ui->baudRateBox->setEditText(settings->value("BaudRate").toString());
    ui->dataBitsBox->setCurrentIndex(settings->value("DataBitsID").toInt());
    ui->stopBitsBox->setCurrentIndex(settings->value("StopBitsID").toInt());
    ui->parityBox->setCurrentIndex(settings->value("ParityID").toInt());
    ui->flowControlBox->setCurrentIndex(settings->value("FlowControlID").toInt());
    settings->endGroup();
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
    port->setPortName(ui->portBox->currentText());
    port->setBaudRate(ui->baudRateBox->currentText().toInt());
    port->setDataBits((QSerialPort::DataBits)ui->dataBitsBox->currentData().toInt());
    port->setStopBits((QSerialPort::StopBits)ui->stopBitsBox->currentData().toInt());
    port->setParity((QSerialPort::Parity)ui->parityBox->currentData().toInt());
    port->setFlowControl((QSerialPort::FlowControl)ui->flowControlBox->currentData().toInt());
    if(port->isOpen())
    {
        QMessageBox::warning(this, "Error", "The port has been opened.");
        return;
    }
    if(!port->open(QSerialPort::ReadWrite))
    {
        QMessageBox::warning(this, "Error", "Cannot open the serial port.");
        return;
    }
    portState = true;
    stateUpdate();
    refreshPortsInfo();
    savePreference(port->portName());
}

void MainWindow::savePreference(const QString& portName)
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


void MainWindow::on_closeButton_clicked()
{
    port->close();
    portState = false;
    stateUpdate();
    refreshPortsInfo();
}

void MainWindow::stateUpdate()
{
    QString paritys[5] = {"NoParity", "EvenParity", "OddParity", "SpaceParity", "MarkParity"};
    portLabel->setText("Port: " + port->portName());
    if(portState)
    {
        stateLabel->setText("State: √");
        baudRateLabel->setText("BaudRate: " + QString::number(port->baudRate()));
        dataBitsLabel->setText("DataBits: " + QString::number(port->dataBits()));
        stopBitsLabel->setText("StopBits: " + QString::number((port->stopBits() == QSerialPort::OneAndHalfStop) ? 1.5 : port->stopBits()));
        parityLabel->setText("Parity: " + paritys[(int)port->parity()]);
        RxLabel->setText("Rx: " + QString::number(rawReceivedData->length()));
        TxLabel->setText("Tx: " + QString::number(rawSendedData->length()));
    }
    else
    {
        stateLabel->setText("State: X");
        baudRateLabel->setText("BaudRate: ");
        dataBitsLabel->setText("DataBits: ");
        stopBitsLabel->setText("StopBits: ");
        parityLabel->setText("Parity: ");
        RxLabel->setText("Rx: 0");
        TxLabel->setText("Tx: 0");
    }


}

void MainWindow::onErrorOccurred(QSerialPort::SerialPortError error)
{
    qDebug() << error;
    if(error != QSerialPort::NoError && portState)
    {
        portState = false;
        stateUpdate();
        port->close();
    }

}

void MainWindow::on_sendButton_clicked()
{
    QByteArray data;
    if(!portState)
    {
        QMessageBox::warning(this, "Error", "No port is opened.");
        return;
    }
    if(isSendedDataHex)
        data = QByteArray::fromHex(ui->sendEdit->text().toLatin1());
    else
        data = ui->sendEdit->text().toLatin1();
    if(ui->suffixCRLFButton->isChecked())
        data += "\r\n";
    else if(ui->suffixCharButton->isChecked())
        data += ui->suffixCharEdit->text().toLatin1();
    else if(ui->suffixByteButton->isChecked())
        data += QByteArray::fromHex(ui->suffixByteEdit->text().toLatin1());
    rawSendedData->append(data);
    syncEditWithData();
    port->write(data);
    TxLabel->setText("Tx: " + QString::number(rawSendedData->length()));
}


void MainWindow::on_sendedHexBox_stateChanged(int arg1)
{
    isSendedDataHex = (arg1 == Qt::Checked);
    syncEditWithData();
}

void MainWindow::on_receivedHexBox_stateChanged(int arg1)
{
    isReceivedDataHex = (arg1 == Qt::Checked);
    syncEditWithData();
}

void MainWindow::syncEditWithData()
{
    RxSlider->blockSignals(true);
    if(isReceivedDataHex)
        ui->receivedEdit->setPlainText(rawReceivedData->toHex(' '));
    else
        ui->receivedEdit->setPlainText(*rawReceivedData);
    if(isSendedDataHex)
        ui->sendedEdit->setPlainText(rawSendedData->toHex(' '));
    else
        ui->sendedEdit->setPlainText(*rawSendedData);
    RxSlider->blockSignals(false);
}

void MainWindow::on_receivedClearButton_clicked()
{
    rawReceivedData->clear();
    RxLabel->setText("Rx: " + QString::number(rawReceivedData->length()));
    syncEditWithData();
}

void MainWindow::on_sendedClearButton_clicked()
{
    rawSendedData->clear();
    TxLabel->setText("Tx: " + QString::number(rawSendedData->length()));
    syncEditWithData();
}

void MainWindow::on_suffixCharEdit_textChanged(const QString &arg1)
{
    ui->suffixByteEdit->setText(arg1.toLatin1().toHex());
}

void MainWindow::on_suffixByteEdit_textChanged(const QString &arg1)
{
    QByteArray newChar = QByteArray::fromHex(arg1.toLatin1());
    if(newChar.length() == 1 && newChar.at(0) >= 0x20 && newChar.at(0) <= 0x7F)
        ui->suffixCharEdit->setText(newChar);
}

void MainWindow::on_sendEdit_textChanged(const QString &arg1)
{
    repeatTimer->stop();
    ui->repeatBox->setChecked(false);
}

void MainWindow::on_repeatBox_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked)
    {
        repeatTimer->setInterval(ui->repeatDelayEdit->text().toInt());
        repeatTimer->start();
    }
    else
        repeatTimer->stop();
}

void MainWindow::dockInit()
{
    setDockNestingEnabled(true);
    QDockWidget* dock;
    QWidget* widget;
    int count = ui->funcTab->count();
    qDebug() << "dock count" << count;
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

void MainWindow::on_receivedCopyButton_clicked()
{
    QApplication::clipboard()->setText(ui->receivedEdit->toPlainText());
}

void MainWindow::on_sendedCopyButton_clicked()
{
    QApplication::clipboard()->setText(ui->sendedEdit->toPlainText());
}
