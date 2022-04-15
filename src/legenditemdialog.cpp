#include "legenditemdialog.h"
#include "ui_legenditemdialog.h"

LegendItemDialog::LegendItemDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LegendItemDialog)
{
    ui->setupUi(this);
    colorDialog = new QColorDialog;
}

LegendItemDialog::LegendItemDialog(const QString &name, const QColor &color, QWidget *parent) : LegendItemDialog(parent)
{
    setName(name);
    setColor(color);
}

LegendItemDialog::~LegendItemDialog()
{
    delete ui;
}

void LegendItemDialog::on_colorButton_clicked()
{
    if(colorDialog->exec() != QDialog::Accepted)
        return;
    m_color = colorDialog->currentColor();
    QString qss = QString("background-color: %1").arg(m_color.name());
    ui->colorButton->setStyleSheet(qss);
    colorDialog->setCurrentColor(m_color);
}

void LegendItemDialog::on_nameEdit_editingFinished()
{
    m_name = ui->nameEdit->text();
}


QColor LegendItemDialog::getColor()
{
    return m_color;
};

QString LegendItemDialog::getName()
{
    return m_name;
};

void LegendItemDialog::setName(const QString& name)
{
    m_name = name;
    ui->nameEdit->setText(m_name);
}

void LegendItemDialog::setColor(const QColor& color)
{
    if(!color.isValid())
        return;
    m_color = color;
    QString qss = QString("background-color: %1").arg(color.name());
    ui->colorButton->setStyleSheet(qss);
    colorDialog->setCustomColor(0, color);
    colorDialog->setCurrentColor(color);
}
