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

    refreshPortsInfo();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::refreshPortsInfo()
{
    QStringList labels;
    labels << "PortName" << "Description" << "Manufacturer" << "SerialNumber" << "IsBusy" << "IsNull" << "SystemLocation" << "VendorID" << "ProductID" << "BaudRates";
    ui->portTable->clear();
    ui->portTable->setColumnCount(10);
    ui->portTable->setHorizontalHeaderLabels(labels);
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    ui->portTable->setRowCount(ports.size());
    for(int i = 0; i < ports.size(); i++)
    {
        ui->portTable->setItem(i, 0, new QTableWidgetItem(ports[i].portName()));
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
