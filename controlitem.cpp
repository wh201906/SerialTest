#include "controlitem.h"
#include "ui_controlitem.h"

ControlItem::ControlItem(Type type, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlItem)
{
    ui->setupUi(this);

    on_prefixBox_stateChanged(Qt::Unchecked);
    on_suffixBox_stateChanged(Qt::Unchecked);

    ui->CMDEdit->setVisible(type == Command);
    ui->sliderGrp->setVisible(type == Slider);
    ui->checkBox->setVisible(type == CheckBox);
    ui->spinBoxGrp->setVisible(type == SpinBox);

    ui->intBox->setVisible(type == SpinBox);
    ui->minEdit->setVisible(type == SpinBox || type == Slider);
    ui->maxEdit->setVisible(type == SpinBox || type == Slider);
    ui->stepEdit->setVisible(type == SpinBox || type == Slider);
    ui->autoBox->setVisible(type != Command);
    ui->hexBox->setVisible(type == Command);
    ui->prefixBox->setVisible(type != Command);

    ui->confGrp->setVisible(false);

    this->type = type;
}

ControlItem::~ControlItem()
{
    delete ui;
}

void ControlItem::on_slider_valueChanged(int value)
{
    ui->sliderEdit->setText(QString::number(value));
}


void ControlItem::on_spinBoxUpButton_clicked()
{
    ui->spinBox->stepUp();
}


void ControlItem::on_spinBoxDownButton_clicked()
{
    ui->spinBox->stepDown();
}


void ControlItem::on_minEdit_editingFinished()
{
    ui->spinBox->setMinimum(ui->minEdit->text().toDouble());
    ui->slider->setMinimum(ui->minEdit->text().toInt());
}


void ControlItem::on_maxEdit_editingFinished()
{
    ui->spinBox->setMaximum(ui->maxEdit->text().toDouble());
    ui->slider->setMaximum(ui->maxEdit->text().toInt());
}


void ControlItem::on_stepEdit_editingFinished()
{
    ui->spinBox->setSingleStep(ui->stepEdit->text().toDouble());
    ui->slider->setSingleStep(ui->stepEdit->text().toInt());
}


void ControlItem::on_confButton_clicked()
{
    if(ui->confButton->text() == ">")
    {
        ui->confButton->setText("<");
        ui->confGrp->setVisible(true);
    }
    else
    {
        ui->confButton->setText(">");
        ui->confGrp->setVisible(false);
    }
}


void ControlItem::on_prefixBox_stateChanged(int arg1)
{
    ui->prefixTypeBox->setVisible(arg1 == Qt::Checked);
    ui->prefixEdit->setVisible(arg1 == Qt::Checked);
}


void ControlItem::on_suffixBox_stateChanged(int arg1)
{
    ui->suffixTypeBox->setVisible(arg1 == Qt::Checked);
    ui->suffixEdit->setVisible(arg1 == Qt::Checked);
}


void ControlItem::on_deleteButton_clicked()
{
    deleteLater();
}


void ControlItem::on_prefixTypeBox_currentIndexChanged(int index)
{
    ui->prefixEdit->setVisible(index != 2);
}


void ControlItem::on_suffixTypeBox_currentIndexChanged(int index)
{
    ui->suffixEdit->setVisible(index != 2);
}


void ControlItem::on_autoBox_stateChanged(int arg1)
{
    ui->sendButton->setVisible(arg1 != Qt::Checked);
    if(arg1 == Qt::Checked)
    {
        connect(ui->slider, &QSlider::valueChanged, this, &ControlItem::on_sendButton_clicked);
        connect(ui->checkBox, &QCheckBox::stateChanged, this, &ControlItem::on_sendButton_clicked);
        connect(ui->spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ControlItem::on_sendButton_clicked);
    }
    else
    {
        disconnect(ui->slider, &QSlider::valueChanged, this, &ControlItem::on_sendButton_clicked);
        disconnect(ui->checkBox, &QCheckBox::stateChanged, this, &ControlItem::on_sendButton_clicked);
        disconnect(ui->spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ControlItem::on_sendButton_clicked);
    }
}


void ControlItem::on_sendButton_clicked()
{
    QByteArray data;
    if(ui->prefixBox->isChecked())
    {
        if(ui->prefixTypeBox->currentIndex() == 0)
            data = ui->prefixEdit->text().toLatin1();
        else if(ui->prefixTypeBox->currentIndex() == 1)
            data = QByteArray::fromHex(ui->prefixEdit->text().toLatin1());
        else if(ui->prefixTypeBox->currentIndex() == 2)
            data = "\r\n";
    }

    if(type == Command)
    {
        if(ui->hexBox->isChecked())
            data = QByteArray::fromHex(ui->CMDEdit->text().toLatin1());
        else
            data = ui->CMDEdit->text().toLatin1();
    }
    else if(type == Slider)
    {
        data += QString::number(ui->slider->value());
    }
    else if(type == CheckBox)
    {
        data += ui->checkBox->isChecked() ? "1" : "0";
    }
    else if(type == SpinBox)
    {
        if(ui->intBox->isChecked())
            data += QString::number((int)ui->spinBox->value());
        else
            data += QString::number(ui->spinBox->value());
    }


    if(ui->suffixBox->isChecked())
    {
        if(ui->suffixTypeBox->currentIndex() == 0)
            data += ui->suffixEdit->text().toLatin1();
        else if(ui->suffixTypeBox->currentIndex() == 1)
            data += QByteArray::fromHex(ui->suffixEdit->text().toLatin1());
        else if(ui->suffixTypeBox->currentIndex() == 2)
            data += "\r\n";
    }
    emit send(data);
}


void ControlItem::on_sliderEdit_editingFinished()
{
    int val = ui->sliderEdit->text().toInt();
    if(val < ui->slider->minimum())
        val = ui->slider->minimum();
    else if(val > ui->slider->maximum())
        val = ui->slider->maximum();
    ui->sliderEdit->setText(QString::number(val));

    if(val == ui->slider->value())
        return;
    ui->slider->setValue(val);
}

