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

    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPortsInfo);
    connect(port, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(ui->sendEdit, &QLineEdit::returnPressed, this, &MainWindow::on_sendButton_clicked);
    connect(port, &QSerialPort::errorOccurred, this, &MainWindow::onErrorOccurred);
    refreshPortsInfo();
    initUI();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readData()
{
    QByteArray newData = port->readAll();
    rawReceivedData->append(newData);
    syncEditWithData();
    RxLabel->setText("Rx: " + QString::number(rawReceivedData->size()));
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
}

void MainWindow::refreshPortsInfo()
{
    QStringList labels;
    labels << "PortName" << "Description" << "Manufacturer" << "SerialNumber" << "IsBusy" << "IsNull" << "SystemLocation" << "VendorID" << "ProductID" << "BaudRates";
    ui->portTable->clear();
    ui->portBox->clear();
    ui->portTable->setColumnCount(10);
    ui->portTable->setHorizontalHeaderLabels(labels);
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
    ui->portBox->setCurrentIndex(row);
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
}

void MainWindow::on_closeButton_clicked()
{
    port->close();
    portState = false;
    stateUpdate();
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
        RxLabel->setText("Rx: " + QString::number(rawReceivedData->size()));
        TxLabel->setText("Tx: " + QString::number(rawSendedData->size()));
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
    else if(ui->suffixEndCharButton->isChecked())
        data += ui->suffixEndCharEdit->text().toLatin1();
    rawSendedData->append(data);
    syncEditWithData();
    port->write(data);
    TxLabel->setText("Tx: " + QString::number(rawSendedData->size()));
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
    if(isReceivedDataHex)
        ui->receivedEdit->setText(rawReceivedData->toHex(' '));
    else
        ui->receivedEdit->setText(*rawReceivedData);
    if(isSendedDataHex)
        ui->sendedEdit->setText(rawSendedData->toHex(' '));
    else
        ui->sendedEdit->setText(*rawSendedData);
}

void MainWindow::on_receivedClearButton_clicked()
{
    rawReceivedData->clear();
    RxLabel->setText("Rx: " + QString::number(rawReceivedData->size()));
    syncEditWithData();
}

void MainWindow::on_sendedButton_clicked()
{
    rawSendedData->clear();
    TxLabel->setText("Tx: " + QString::number(rawSendedData->size()));
    syncEditWithData();
}
