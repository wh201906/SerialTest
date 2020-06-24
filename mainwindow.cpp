#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    port = new QSerialPort();
    info = new QSerialPortInfo();
    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPortsInfo);
    connect(port, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(ui->sendEdit, &QLineEdit::returnPressed, this, &MainWindow::on_sendButton_clicked);

    refreshPortsInfo();
    initUI();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readData()
{
    ui->receivedEdit->append(port->readAll());
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
    on_advancedBox_clicked(false);
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
    port->open(QSerialPort::ReadWrite);

}

void MainWindow::on_closeButton_clicked()
{
    port->close();
}

void MainWindow::stateUpdate()
{

}

void MainWindow::on_sendButton_clicked()
{
    ui->sendedEdit->append(ui->sendEdit->text());
    port->write((ui->sendEdit->text() + "\r\n").toLatin1());
}

