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
}

ControlItem::~ControlItem()
{
    delete ui;
}

void ControlItem::on_slider_valueChanged(int value)
{
    ui->sliderLabel->setText(QString::number(value));
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

