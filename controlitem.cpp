#include "controlitem.h"
#include "ui_controlitem.h"

ControlItem::ControlItem(Type type, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlItem)
{
    ui->setupUi(this);

    on_prefixBox_stateChanged(Qt::Unchecked);
    on_suffixBox_stateChanged(Qt::Unchecked);

    ui->confGrp->setVisible(false);

    this->type = type;
    initUI();
}

void ControlItem::initUI()
{
    ui->CMDEdit->setVisible(type == Command);
    ui->sliderGrp->setVisible(type == Slider);
    ui->checkBox->setVisible(type == CheckBox);
    ui->spinBoxGrp->setVisible(type == SpinBox);

    ui->minEdit->setVisible(type == SpinBox || type == Slider);
    ui->maxEdit->setVisible(type == SpinBox || type == Slider);
    ui->stepEdit->setVisible(type == SpinBox || type == Slider);
    ui->autoBox->setVisible(type != Command);
    ui->hexBox->setVisible(type == Command);
    ui->prefixBox->setVisible(type != Command);

    on_minEdit_editingFinished();
    on_maxEdit_editingFinished();
    on_stepEdit_editingFinished();
    on_prefixBox_stateChanged(ui->prefixBox->checkState());
    on_prefixTypeBox_currentIndexChanged(ui->prefixTypeBox->currentIndex());
    on_suffixBox_stateChanged(ui->suffixBox->checkState());
    on_suffixTypeBox_currentIndexChanged(ui->suffixTypeBox->currentIndex());
    on_hexBox_stateChanged(ui->hexBox->checkState());
    on_autoBox_stateChanged(ui->autoBox->checkState());

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
    if(type == SpinBox)
        ui->spinBox->setMinimum(ui->minEdit->text().toDouble());
    else if(type == Slider)
        ui->slider->setMinimum(ui->minEdit->text().toInt());
}


void ControlItem::on_maxEdit_editingFinished()
{
    if(type == SpinBox)
        ui->spinBox->setMaximum(ui->maxEdit->text().toDouble());
    else if(type == Slider)
        ui->slider->setMaximum(ui->maxEdit->text().toInt());
}


void ControlItem::on_stepEdit_editingFinished()
{
    if(type == SpinBox)
        ui->spinBox->setSingleStep(ui->stepEdit->text().toDouble());
    else if(type == Slider)
        ui->slider->setSingleStep(ui->stepEdit->text().toInt());
}


void ControlItem::on_confButton_clicked()
{
    if(ui->confButton->text() == ">")
    {
        ui->confButton->setText("<");
        ui->confGrp->setVisible(true);
        ui->mainGrp->setVisible(false);
    }
    else
    {
        ui->confButton->setText(">");
        ui->confGrp->setVisible(false);
        ui->mainGrp->setVisible(true);
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
    ui->prefixEdit->setPlaceholderText(tr("Prefix") + ((index == 1) ? "(Hex)" : ""));
}


void ControlItem::on_suffixTypeBox_currentIndexChanged(int index)
{
    ui->suffixEdit->setVisible(index != 2);
    ui->suffixEdit->setPlaceholderText(tr("Suffix") + ((index == 1) ? "(Hex)" : ""));
}


void ControlItem::on_autoBox_stateChanged(int arg1)
{
    ui->sendButton->setVisible(arg1 != Qt::Checked);
    if(arg1 == Qt::Checked)
    {
        connect(ui->slider, &QSlider::sliderReleased, this, &ControlItem::on_sendButton_clicked);
        connect(ui->checkBox, &QCheckBox::stateChanged, this, &ControlItem::on_sendButton_clicked);
        connect(ui->spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ControlItem::on_sendButton_clicked);
    }
    else
    {
        disconnect(ui->slider, &QSlider::sliderReleased, this, &ControlItem::on_sendButton_clicked);
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
        data += QString::number(ui->slider->value()).toLatin1();
    }
    else if(type == CheckBox)
    {
        data += ui->checkBox->isChecked() ? "1" : "0";
    }
    else if(type == SpinBox)
    {
        data += QString::number(ui->spinBox->value()).toLatin1();
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
    emit ui->slider->sliderReleased();
}


void ControlItem::on_hexBox_stateChanged(int arg1)
{
    ui->CMDEdit->setPlaceholderText(tr("Command") + ((arg1 == Qt::Checked) ? "(Hex)" : ""));
}

bool ControlItem::load(QString& data)
{
    QStringList list = data.split(dataSplitter);
    if(list.length() < 14)
        return false;
    type = (Type)list[0].toUInt();
    ui->nameEdit->setText(list[1]);
    ui->prefixBox->setCheckState((Qt::CheckState)list[2].toUInt());
    ui->prefixTypeBox->setCurrentIndex(list[3].toUInt());
    ui->prefixEdit->setText(list[4]);
    ui->suffixBox->setCheckState((Qt::CheckState)list[5].toUInt());
    ui->suffixTypeBox->setCurrentIndex(list[6].toUInt());
    ui->suffixEdit->setText(list[7]);
    ui->hexBox->setCheckState((Qt::CheckState)list[8].toUInt());
    ui->autoBox->setCheckState((Qt::CheckState)list[9].toUInt());
    ui->minEdit->setText(list[10]);
    ui->maxEdit->setText(list[11]);
    ui->stepEdit->setText(list[12]);

    ui->slider->blockSignals(true);
    ui->checkBox->blockSignals(true);
    ui->spinBox->blockSignals(true);
    if(type == Command)
    {
        ui->CMDEdit->setText(list[13]);
    }
    else if(type == Slider)
    {
        ui->sliderEdit->setText(list[13]);
        ui->slider->setValue(list[13].toInt());
    }
    else if(type == CheckBox)
    {
        ui->checkBox->setCheckState((Qt::CheckState)list[13].toUInt());
    }
    else if(type == SpinBox)
    {
        ui->spinBox->setValue(list[13].toDouble());
    }
    ui->slider->blockSignals(false);
    ui->checkBox->blockSignals(false);
    ui->spinBox->blockSignals(false);

    initUI();
    return true;
}

QString ControlItem::save()
{
    QString data;
    data = QString::number(type) + dataSplitter;
    data += ui->nameEdit->text() + dataSplitter;
    data += QString::number(ui->prefixBox->checkState()) + dataSplitter;
    data += QString::number(ui->prefixTypeBox->currentIndex()) + dataSplitter;
    data += ui->prefixEdit->text() + dataSplitter;
    data += QString::number(ui->suffixBox->checkState()) + dataSplitter;
    data += QString::number(ui->suffixTypeBox->currentIndex()) + dataSplitter;
    data += ui->suffixEdit->text() + dataSplitter;
    data += QString::number(ui->hexBox->checkState()) + dataSplitter;
    data += QString::number(ui->autoBox->checkState()) + dataSplitter;
    data += ui->minEdit->text() + dataSplitter;
    data += ui->maxEdit->text() + dataSplitter;
    data += ui->stepEdit->text() + dataSplitter;
    if(type == Command)
        data += ui->CMDEdit->text();
    else if(type == Slider)
        data += QString::number(ui->slider->value());
    else if(type == CheckBox)
        data += QString::number(ui->checkBox->checkState());
    else if(type == SpinBox)
        data += QString::number(ui->spinBox->value());
    return data;
}
