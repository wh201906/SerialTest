#include "serialpinout.h"
#include "ui_serialpinout.h"

SerialPinout::SerialPinout(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SerialPinout)
{
    ui->setupUi(this);
    onEnableStateChanged(false);
}

SerialPinout::~SerialPinout()
{
    delete ui;
}

bool SerialPinout::getEnableState()
{
    return ui->enaBox->isChecked();
}

void SerialPinout::setEnableState(bool state)
{
    onEnableStateChanged(state);
}

void SerialPinout::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    activeBGId++;
    activeBGId %= 3;
}

void SerialPinout::on_enaBox_clicked(bool checked)
{
    onEnableStateChanged(checked);
    emit enableStateChanged(checked);
}

void SerialPinout::onEnableStateChanged(bool state)
{
    if(state)
    {
        ui->CTSLabel->setVisible(true);
        ui->DCDLabel->setVisible(true);
        ui->DSRLabel->setVisible(true);
        ui->RILabel->setVisible(true);
        ui->line1->setVisible(true);
        ui->line2->setVisible(true);
        ui->line3->setVisible(true);
        ui->line4->setVisible(true);
        ui->enaBox->setText("");
    }
    else
    {
        ui->CTSLabel->setVisible(false);
        ui->DCDLabel->setVisible(false);
        ui->DSRLabel->setVisible(false);
        ui->RILabel->setVisible(false);
        ui->line1->setVisible(false);
        ui->line2->setVisible(false);
        ui->line3->setVisible(false);
        ui->line4->setVisible(false);
        ui->enaBox->setText(tr("Pinouts"));
    }
}

void SerialPinout::setPinout(QSerialPort::PinoutSignals signal)
{

    if(signal.testFlag(QSerialPort::ClearToSendSignal))
        ui->CTSLabel->setStyleSheet(activeBGList[activeBGId]);
    else
        ui->CTSLabel->setStyleSheet(noBG);
    if(signal.testFlag(QSerialPort::DataCarrierDetectSignal))
        ui->DCDLabel->setStyleSheet(activeBGList[activeBGId]);
    else
        ui->DCDLabel->setStyleSheet(noBG);
    if(signal.testFlag(QSerialPort::DataSetReadySignal))
        ui->DSRLabel->setStyleSheet(activeBGList[activeBGId]);
    else
        ui->DSRLabel->setStyleSheet(noBG);
    if(signal.testFlag(QSerialPort::RingIndicatorSignal))
        ui->RILabel->setStyleSheet(activeBGList[activeBGId]);
    else
        ui->RILabel->setStyleSheet(noBG);
}
