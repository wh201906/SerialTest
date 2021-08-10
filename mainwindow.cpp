#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qsrand(QTime::currentTime().msecsSinceStartOfDay());
    port = new QSerialPort();
    info = new QSerialPortInfo();
    portLabel = new QLabel();
    stateButton = new QPushButton();
    baudRateLabel = new QLabel();
    dataBitsLabel = new QLabel();
    stopBitsLabel = new QLabel();
    parityLabel = new QLabel();
    TxLabel = new QLabel();
    RxLabel = new QLabel();
    onTopBox = new QCheckBox(tr("On Top"));
    portState = false;

    rawReceivedData = new QByteArray();
    rawSendedData = new QByteArray();
    plotBuf = new QByteArray();

    repeatTimer = new QTimer();

    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPortsInfo);
    connect(port, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(ui->sendEdit, &QLineEdit::returnPressed, this, &MainWindow::on_sendButton_clicked);
    connect(port, &QSerialPort::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(repeatTimer, &QTimer::timeout, this, &MainWindow::on_sendButton_clicked);
    connect(onTopBox, &QCheckBox::clicked, this, &MainWindow::onTopBoxClicked);
    connect(stateButton, &QPushButton::clicked, this, &MainWindow::onStateButtonClicked);

    RxSlider = ui->receivedEdit->verticalScrollBar();
    connect(RxSlider, &QScrollBar::valueChanged, this, &MainWindow::onRxSliderValueChanged);
    connect(RxSlider, &QScrollBar::sliderMoved, this, &MainWindow::onRxSliderMoved);

    settings = new QSettings("preference.ini", QSettings::IniFormat);

    refreshPortsInfo();
    initUI();

    ui->qcpWidget->axisRect()->setupFullAxesBox(true);
    ui->qcpWidget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes);
    on_plot_dataNumBox_valueChanged(ui->plot_dataNumBox->value());
    plotCounter = 0;
    connect(ui->qcpWidget, &QCustomPlot::legendDoubleClick, this, &MainWindow::onQCPLegendDoubleClick);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onStateButtonClicked()
{
    qDebug() << port->portName();
    if(port->portName().isEmpty())
    {
        QMessageBox::warning(this, "Error", "Plz connect to a port first");
        return;
    }
    if(portState)
    {
        port->close();
        portState = false;
    }
    else
    {
        portState = port->open(QSerialPort::ReadWrite);
        if(!portState)
            QMessageBox::warning(this, "Error", tr("Cannot open the serial port."));
    }
    stateUpdate();
}

void MainWindow::onTopBoxClicked(bool checked)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, checked);
    show();
}

void MainWindow::onRxSliderValueChanged(int value)
{
    // qDebug() << "valueChanged" << value;
    currRxSliderPos = value;
}

void MainWindow::onRxSliderMoved(int value)
{
    // slider is moved by user
    // qDebug() << "sliderMoved" << value;
    userRequiredRxSliderPos = value;
}

void MainWindow::readData()
{
    if(processingOutput)
        return;
    processingOutput = true;
    QByteArray newData = port->readAll();
    if(newData.isEmpty())
        return;
    appendReceivedData(newData);
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
    if(ui->plot_realTimeBox->isChecked())
    {
        int i;
        QStringList dataList;
        plotBuf->append(newData);
        while((i = plotBuf->indexOf(ui->plot_frameSpEdit->text())) != -1)
        {
            dataList = ((QString)(plotBuf->left(i))).split(ui->plot_dataSpEdit->text());
            plotBuf->remove(0, i + ui->plot_dataSpEdit->text().length());
            plotCounter++;
            for(i = 0; i < ui->plot_dataNumBox->value() && i < dataList.length(); i++)
            {
                ui->qcpWidget->graph(i)->addData(plotCounter, dataList[i].toDouble());
            }
        }
        ui->qcpWidget->replot(QCustomPlot::rpQueuedReplot);
    }
    QApplication::processEvents();
    processingOutput = false;
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

    stateButton->setMinimumHeight(1);
    stateButton->setStyleSheet("*{text-align:left;}");
    statusBar()->addWidget(portLabel, 1);
    statusBar()->addWidget(stateButton, 1);
    statusBar()->addWidget(baudRateLabel, 1);
    statusBar()->addWidget(dataBitsLabel, 1);
    statusBar()->addWidget(stopBitsLabel, 1);
    statusBar()->addWidget(parityLabel, 1);
    statusBar()->addWidget(RxLabel, 1);
    statusBar()->addWidget(TxLabel, 1);
    statusBar()->addWidget(onTopBox, 1);

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
}

void MainWindow::on_portTable_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    QStringList preferences = settings->childGroups();
    QStringList::iterator it;
    ui->portBox->setCurrentIndex(row);

    // search preference by <vendorID>-<productID>
    QString id = ui->portTable->item(row, HVendorID)->text();  // vendor id
    id += "-";
    id += ui->portTable->item(row, HProductID)->text(); // product id
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
    id = ui->portTable->item(row, HPortName)->text();
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
        QMessageBox::warning(this, "Error", tr("Cannot open the serial port."));
        return;
    }
    portState = true;
    stateUpdate();
    // refreshPortsInfo(); // this takes a lot of time
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
    // refreshPortsInfo(); // this takes a lot of time
}

void MainWindow::stateUpdate()
{
    QString stopbits[4] = {"", "OneStop", "TwoStop", "OneAndHalfStop"};
    QString parities[6] = {"NoParity", "", "EvenParity", "OddParity", "SpaceParity", "MarkParity"};
    portLabel->setText("Port: " + port->portName());
    if(portState)
    {
        stateButton->setText("State: √");
        baudRateLabel->setText("BaudRate: " + QString::number(port->baudRate()));
        dataBitsLabel->setText("DataBits: " + QString::number(port->dataBits()));
        stopBitsLabel->setText("StopBits: " + stopbits[(int)port->stopBits()]);
        parityLabel->setText("Parity: " + parities[(int)port->parity()]);
    }
    else
    {
        stateButton->setText("State: X");
        baudRateLabel->setText("BaudRate: ");
        dataBitsLabel->setText("DataBits: ");
        stopBitsLabel->setText("StopBits: ");
        parityLabel->setText("Parity: ");
    }
    RxLabel->setText("Rx: " + QString::number(rawReceivedData->length()));
    TxLabel->setText("Tx: " + QString::number(rawSendedData->length()));
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
    syncSendedEditWithData();
    port->write(data);
    TxLabel->setText("Tx: " + QString::number(rawSendedData->length()));
}


void MainWindow::on_sendedHexBox_stateChanged(int arg1)
{
    isSendedDataHex = (arg1 == Qt::Checked);
    syncSendedEditWithData();
}

void MainWindow::on_receivedHexBox_stateChanged(int arg1)
{
    isReceivedDataHex = (arg1 == Qt::Checked);
    syncReceivedEditWithData();
}

void MainWindow::syncReceivedEditWithData()
{
    RxSlider->blockSignals(true);
    if(isReceivedDataHex)
        ui->receivedEdit->setPlainText(toHEX(*rawReceivedData));
    else
        ui->receivedEdit->setPlainText(*rawReceivedData);
    RxSlider->blockSignals(false);
//    qDebug() << toHEX(*rawReceivedData);
}

void MainWindow::syncSendedEditWithData()
{
    if(isSendedDataHex)
        ui->sendedEdit->setPlainText(toHEX(*rawSendedData));
    else
        ui->sendedEdit->setPlainText(*rawSendedData);
}

// TODO:
// split sync process, add processEvents()
// void MainWindow::syncEditWithData()

void MainWindow::appendReceivedData(QByteArray& data)
{
    QTextCursor cursor;
    int pos;
    bool chopped = false;
    pos = RxSlider->sliderPosition();
    rawReceivedData->append(data);

    // if \r and \n are received seperatedly, the rawReceivedData will be fine, but the receivedEdit will have a empty line
    // just ignore one of them
    if(!data.isEmpty() && *(data.end() - 1) == '\r')
    {
        data.chop(1);
        chopped = true;
    }

    cursor = ui->receivedEdit->textCursor();
    ui->receivedEdit->moveCursor(QTextCursor::End);
    if(isReceivedDataHex)
        ui->receivedEdit->insertPlainText(toHEX(data));
    else
        ui->receivedEdit->insertPlainText(data);
    if(chopped)
        data.append('\r'); // undo data.chop(1);
    ui->receivedEdit->setTextCursor(cursor);
    RxSlider->setSliderPosition(pos);
}

void MainWindow::on_receivedClearButton_clicked()
{
    rawReceivedData->clear();
    RxLabel->setText("Rx: " + QString::number(rawReceivedData->length()));
    syncReceivedEditWithData();
}

void MainWindow::on_sendedClearButton_clicked()
{
    rawSendedData->clear();
    TxLabel->setText("Tx: " + QString::number(rawSendedData->length()));
    syncSendedEditWithData();
}

void MainWindow::on_suffixCharEdit_textChanged(const QString &arg1)
{
    ui->suffixByteEdit->setText(arg1.toLatin1().toHex());
}

void MainWindow::on_suffixByteEdit_textChanged(const QString &arg1)
{
    QByteArray newChar = QByteArray::fromHex(arg1.toLatin1());
    if(newChar.length() == 1 && newChar.at(0) >= 0x20 && newChar.at(0) < 0x7F)
        ui->suffixCharEdit->setText(newChar);
}

void MainWindow::on_sendEdit_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
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

void MainWindow::on_receivedExportButton_clicked()
{
    bool flag = true;
    QString fileName, selection;
    fileName = QFileDialog::getSaveFileName(this, tr("Export received data"), QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".txt");
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    flag &= file.open(QFile::WriteOnly | QFile::Text);
    selection = ui->receivedEdit->textCursor().selectedText().replace(QChar(0x2029), '\n');
    if(selection.isEmpty())
        flag &= file.write(ui->receivedEdit->toPlainText().toUtf8()) != -1;
    else
        flag &= file.write(selection.replace(QChar(0x2029), '\n').toUtf8()) != -1;
    file.close();
    QMessageBox::information(this, tr("Info"), flag ? tr("Successed!") : tr("Failed!"));
}

void MainWindow::on_sendedExportButton_clicked()
{
    bool flag = true;
    QString fileName, selection;
    fileName = QFileDialog::getSaveFileName(this, tr("Export sended data"), QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".txt");
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    flag &= file.open(QFile::WriteOnly | QFile::Text);
    selection = ui->sendedEdit->textCursor().selectedText().replace(QChar(0x2029), '\n');
    if(selection.isEmpty())
        flag &= file.write(ui->sendedEdit->toPlainText().toUtf8()) != -1;
    else
        flag &= file.write(selection.replace(QChar(0x2029), '\n').toUtf8()) != -1;
    file.close();
    QMessageBox::information(this, tr("Info"), flag ? tr("Successed!") : tr("Failed!"));

}

QByteArray MainWindow::toHEX(const QByteArray& array)
{
    if(array.isEmpty())
        return QByteArray();
    QByteArray hex(3 * array.size(), Qt::Uninitialized);
    char* hexData = hex.data();
    const uchar *data = (const uchar *)array.data();
    for(qsizetype i = 0, o = 0; i < array.size(); ++i)
    {
        hexData[o++] = hexTable[data[i] * 2];
        hexData[o++] = hexTable[data[i] * 2 + 1];
        hexData[o++] = ' ';
    }
    return hex;
}

const char MainWindow::hexTable[256 * 2 + 1] =
{
    "000102030405060708090A0B0C0D0E0F"
    "101112131415161718191A1B1C1D1E1F"
    "202122232425262728292A2B2C2D2E2F"
    "303132333435363738393A3B3C3D3E3F"
    "404142434445464748494A4B4C4D4E4F"
    "505152535455565758595A5B5C5D5E5F"
    "606162636465666768696A6B6C6D6E6F"
    "707172737475767778797A7B7C7D7E7F"
    "808182838485868788898A8B8C8D8E8F"
    "909192939495969798999A9B9C9D9E9F"
    "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
    "B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
    "C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
    "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
    "E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
    "F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF"
};

void MainWindow::on_plot_dataNumBox_valueChanged(int arg1)
{
    int delta = arg1 - ui->qcpWidget->graphCount();
    if(delta > 0)
    {
        for(int i = 0; i < delta; i++)
            ui->qcpWidget->addGraph()->setPen(QColor(rand() % 235 + 10, rand() % 235 + 10, rand() % 235 + 10));
    }
    else if(delta < 0)
    {
        delta = -delta;
        for(int i = 0; i < delta; i++)
            ui->qcpWidget->removeGraph(ui->qcpWidget->graphCount() - 1);
    }
}


void MainWindow::on_plot_clearButton_clicked()
{
    int num;
    plotCounter = 0;
    num = ui->qcpWidget->graphCount();
    for(int i = 0; i < num; i++)
        ui->qcpWidget->graph(i)->data()->clear(); // use data()->clear() rather than data().clear()
    ui->qcpWidget->replot();
}


void MainWindow::on_plot_legendCheckBox_stateChanged(int arg1)
{
    ui->qcpWidget->legend->setVisible(arg1 == Qt::Checked);
}


void MainWindow::on_plot_advancedBox_stateChanged(int arg1)
{
    ui->plot_advancedWidget->setVisible(arg1 == Qt::Checked);
}

void MainWindow::onQCPLegendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item)
{
    // Rename a graph by double clicking on its legend item
    Q_UNUSED(legend)
    if(item)  // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
    {
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Legend:"), tr("New graph name:"), QLineEdit::Normal, plItem->plottable()->name(), &ok);
        if(ok)
        {
            plItem->plottable()->setName(newName);
            ui->qcpWidget->replot();
        }
    }
}

