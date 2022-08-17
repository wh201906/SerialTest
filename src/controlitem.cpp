#include "controlitem.h"
#include "ui_controlitem.h"
#include "util.h"

#include <QTextCodec>
#include <QTimer>

ControlItem::ControlItem(Type type, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlItem)
{
    ui->setupUi(this);

    on_prefixBox_stateChanged(Qt::Unchecked);
    on_suffixBox_stateChanged(Qt::Unchecked);

    ui->confGrp->hide();

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
    ui->autoLabel->setVisible(type != Command);
    ui->hexBox->setVisible(type == Command);
    ui->hexLabel->setVisible(type == Command);

    on_minEdit_editingFinished();
    on_maxEdit_editingFinished();
    on_stepEdit_editingFinished();
    on_prefixTypeBox_currentIndexChanged(ui->prefixTypeBox->currentIndex());
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
    if(m_sliderPageChanged)
    {
        m_sliderPageChanged = false;
        // emit ui->slider->sliderReleased();
        // on_sendButton_clicked()
        // the on_sendButton_clicked() should be called a little bit later,
        // otherwise, the slider will go wrong
        if(ui->autoBox->isChecked())
            QTimer::singleShot(200, this, &ControlItem::on_sendButton_clicked);
    }
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
    Q_UNUSED(arg1)
    on_prefixTypeBox_currentIndexChanged(ui->prefixTypeBox->currentIndex());
}


void ControlItem::on_suffixBox_stateChanged(int arg1)
{
    Q_UNUSED(arg1)
    on_suffixTypeBox_currentIndexChanged(ui->suffixTypeBox->currentIndex());
}


void ControlItem::on_deleteButton_clicked()
{
    deleteLater();
}


void ControlItem::on_prefixTypeBox_currentIndexChanged(int index)
{
    ui->prefixTypeBox->setVisible(ui->prefixBox->isChecked());
    ui->prefixEdit->setVisible(index != 2 && index != 3 && ui->prefixBox->isChecked());
    ui->prefixEdit->setPlaceholderText(tr("Prefix") + ((index == 1) ? "(Hex)" : ""));
}


void ControlItem::on_suffixTypeBox_currentIndexChanged(int index)
{
    ui->suffixTypeBox->setVisible(ui->suffixBox->isChecked());
    ui->suffixEdit->setVisible(index != 2 && index != 3 && ui->suffixBox->isChecked());
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
        {
            if(ui->unescapeBox->isChecked())
                data = Util::unescape(ui->prefixEdit->text(), dataCodec);
            else
                data = dataCodec->fromUnicode(ui->prefixEdit->text());
        }
        else if(ui->prefixTypeBox->currentIndex() == 1)
            data = QByteArray::fromHex(ui->prefixEdit->text().toLatin1());
        else if(ui->prefixTypeBox->currentIndex() == 2)
            data = "\r\n";
        else if(ui->prefixTypeBox->currentIndex() == 3)
            data = "\n";
    }

    if(type == Command)
    {
        if(ui->hexBox->isChecked())
            data += QByteArray::fromHex(ui->CMDEdit->text().toLatin1());
        else
        {
            if(ui->unescapeBox->isChecked())
                data += Util::unescape(ui->CMDEdit->text(), dataCodec);
            else
                data += dataCodec->fromUnicode(ui->CMDEdit->text());
        }

    }
    else if(type == Slider)
    {
        data += dataCodec->fromUnicode(QString::number(ui->slider->value()));
    }
    else if(type == CheckBox)
    {
        data += dataCodec->fromUnicode(ui->checkBox->isChecked() ? "1" : "0");
    }
    else if(type == SpinBox)
    {
        data += dataCodec->fromUnicode(QString::number(ui->spinBox->value()));
    }


    if(ui->suffixBox->isChecked())
    {
        if(ui->suffixTypeBox->currentIndex() == 0)
        {
            if(ui->unescapeBox->isChecked())
                data += Util::unescape(ui->suffixEdit->text(), dataCodec);
            else
                data += dataCodec->fromUnicode(ui->suffixEdit->text());
        }
        else if(ui->suffixTypeBox->currentIndex() == 1)
            data += QByteArray::fromHex(ui->suffixEdit->text().toLatin1());
        else if(ui->suffixTypeBox->currentIndex() == 2)
            data += "\r\n";
        else if(ui->suffixTypeBox->currentIndex() == 3)
            data += "\n";
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

bool ControlItem::load(const QJsonObject& dict)
{
    type = (Type)dict["type"].toInt();
    ui->nameEdit->setText(dict["name"].toString());
    ui->prefixBox->setChecked(dict["prefixEnabled"].toBool());
    ui->prefixTypeBox->setCurrentIndex(dict["prefixType"].toInt());
    ui->prefixEdit->setText(dict["prefix"].toString());
    ui->suffixBox->setChecked(dict["suffixEnabled"].toBool());
    ui->suffixTypeBox->setCurrentIndex(dict["suffixType"].toInt());
    ui->suffixEdit->setText(dict["suffix"].toString());
    ui->hexBox->setChecked(dict["hex"].toBool());
    ui->unescapeBox->setChecked(dict["unescape"].toBool());
    ui->autoBox->setChecked(dict["auto"].toBool());
    ui->minEdit->setText(dict["min"].toString());
    ui->maxEdit->setText(dict["max"].toString());
    ui->stepEdit->setText(dict["step"].toString());

    ui->slider->blockSignals(true);
    ui->checkBox->blockSignals(true);
    ui->spinBox->blockSignals(true);
    if(type == Command)
    {
        ui->CMDEdit->setText(dict["content"].toString());
    }
    else if(type == Slider)
    {
        ui->slider->setValue(dict["content"].toInt());
        ui->sliderEdit->setText(QString::number(ui->slider->value()));
    }
    else if(type == CheckBox)
    {
        ui->checkBox->setChecked(dict["content"].toBool());
    }
    else if(type == SpinBox)
    {
        ui->spinBox->setValue(dict["content"].toDouble());
    }
    ui->slider->blockSignals(false);
    ui->checkBox->blockSignals(false);
    ui->spinBox->blockSignals(false);

    initUI();
    return true;
}

QJsonObject ControlItem::save()
{
    QJsonObject data;

    data["type"] = type;
    data["name"] = ui->nameEdit->text();
    data["prefixEnabled"] = ui->prefixBox->isChecked();
    data["prefixType"] = ui->prefixTypeBox->currentIndex();
    data["prefix"] = ui->prefixEdit->text();
    data["suffixEnabled"] = ui->suffixBox->isChecked();
    data["suffixType"] = ui->suffixTypeBox->currentIndex();
    data["suffix"] = ui->suffixEdit->text();
    data["hex"] = ui->hexBox->isChecked();
    data["unescape"] = ui->unescapeBox->isChecked();
    data["auto"] = ui->autoBox->isChecked();
    data["min"] = ui->minEdit->text();
    data["max"] = ui->maxEdit->text();
    data["step"] = ui->stepEdit->text();
    if(type == Command)
        data["content"] = ui->CMDEdit->text();
    else if(type == Slider)
        data["content"] = ui->slider->value();
    else if(type == CheckBox)
        data["content"] = ui->checkBox->isChecked();
    else if(type == SpinBox)
        data["content"] = ui->spinBox->value();

    return data;
}

void ControlItem::setDataCodec(QTextCodec *codec)
{
    this->dataCodec = codec;
}

void ControlItem::on_slider_actionTriggered(int action)
{
    if(action == QSlider::SliderPageStepAdd || action == QSlider::SliderPageStepSub)
    {
        m_sliderPageChanged = true;
    }
}

