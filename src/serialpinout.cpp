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

void SerialPinout::initSettings()
{
    m_settings = MySettings::defaultSettings();
    loadPreference();

    // savePreference() is called in mouseDoubleClickEvent() and on_enaBox_clicked()
}

bool SerialPinout::getEnableState()
{
    return ui->enaBox->isChecked();
}

void SerialPinout::setEnableState(bool state)
{
    // this function will not emit enableStateChanged() signal
    onEnableStateChanged(state);
}

void SerialPinout::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    activeBGId++;
    activeBGId %= 3;
    savePreference();
}

void SerialPinout::on_enaBox_clicked(bool checked)
{
    // this function will emit enableStateChanged() signal
    // It indicates that the user changed the enable state
    onEnableStateChanged(checked);
    emit enableStateChanged(checked);
    // Store the user preference
    savePreference();
}

void SerialPinout::loadPreference()
{
    m_settings->beginGroup("SerialTest_Pinout");
    ui->enaBox->setChecked(m_settings->value("Enabled", false).toBool());
    activeBGId = m_settings->value("ColorId", 0).toInt();
    activeBGId %= 3;
    m_settings->endGroup();
    on_enaBox_clicked(ui->enaBox->isChecked());
}

void SerialPinout::savePreference()
{
    m_settings->beginGroup("SerialTest_Pinout");
    m_settings->setValue("Enabled", ui->enaBox->isChecked());
    m_settings->setValue("ColorId", activeBGId);
    m_settings->endGroup();
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
    ui->enaBox->setChecked(state);
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
