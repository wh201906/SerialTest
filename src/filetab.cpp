#include "filetab.h"
#include "ui_filetab.h"

#include <QFileDialog>
#include <QDebug>

FileTab::FileTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileTab)
{
    ui->setupUi(this);

    m_checksumThread = new QThread();
    m_checksumCalc = new AsyncCRC();
    m_checksumCalc->setNotify(true);
    m_checksumCalc->setParam(32, 0x04C11DB7ULL, 0xFFFFFFFFULL, true, true, 0xFFFFFFFFULL); // zlib CRC-32
    m_checksumCalc->moveToThread(m_checksumThread);
    m_checksumThread->start();
    connect(m_checksumCalc, &AsyncCRC::result, this, &FileTab::onChecksumUpdated);

    ui->centralLayout->setStretchFactor(ui->statusEdit, 1);
    on_Raw_throttleGrp_buttonClicked(nullptr);
}

FileTab::~FileTab()
{
    delete ui;
}

void FileTab::on_fileBrowseButton_clicked()
{

}

void FileTab::on_Raw_throttleGrp_buttonClicked(QAbstractButton* button)
{
    Q_UNUSED(button)
    ui->Raw_throttleByteBox->setEnabled(ui->Raw_throttleByteButton->isChecked());
    ui->Raw_throttleMsBox->setEnabled(ui->Raw_throttleMsButton->isChecked());
}

void FileTab::on_checksumButton_clicked()
{
    ui->checksumLabel->setText(tr("Calculating..."));
    QMetaObject::invokeMethod(m_checksumCalc, "reset", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_checksumCalc, "addData", Qt::QueuedConnection, Q_ARG(QByteArray, QByteArray::fromHex(ui->filePathEdit->text().toLatin1())));

    // only for debug
    int len = 7;
    QByteArray data = QByteArray::fromHex(ui->filePathEdit->text().toLatin1());
    AsyncCRC crc[len];
    crc[0].setParam(8, 0X07); // CRC-8
    crc[1].setParam(16, 0x1021, 0ULL, true, true); // CRC-16/KERMIT
    crc[2].setParam(16, 0x8005, 0xFFFFULL, true, true); // CRC-16/MODBUS
    crc[3].setParam(16, 0x1021); // CRC-16/XMODEM
    crc[4].setParam(32, 0x04C11DB7ULL, 0xFFFFFFFFULL, true, true, 0xFFFFFFFFULL); // CRC-32
    crc[5].setParam(32, 0x04C11DB7ULL, 0xFFFFFFFFULL, false, false, 0xFFFFFFFFULL); // CRC-32/BZIP2
    crc[6].setParam(32, 0x000000AFULL); // CRC-32/XFER

    for(int i = 0; i < len; i++)
    {
        connect(&crc[i], &AsyncCRC::result, [](quint64 result)
        {
            qDebug() << QString::number(result, 16);
        });
        crc[i].setNotify(true);
        crc[i].reset();
        crc[i].addData(data);
    }
}

void FileTab::onChecksumUpdated(quint32 checksum)
{
    ui->checksumLabel->setText(QString::number(checksum, 16));
}
