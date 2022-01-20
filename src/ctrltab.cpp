#include "ctrltab.h"
#include "ui_ctrltab.h"

#include "controlitem.h"

#include <QDateTime>
#ifndef Q_OS_ANDROID
#include <QFileDialog>
#endif
#include <QMessageBox>

CtrlTab::CtrlTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CtrlTab)
{
    ui->setupUi(this);

    connect(ui->ctrl_addCMDButton, &QPushButton::clicked, this, &CtrlTab::addCtrlItem);
    connect(ui->ctrl_addSliderButton, &QPushButton::clicked, this, &CtrlTab::addCtrlItem);
    connect(ui->ctrl_addCheckBoxButton, &QPushButton::clicked, this, &CtrlTab::addCtrlItem);
    connect(ui->ctrl_addSpinBoxButton, &QPushButton::clicked, this, &CtrlTab::addCtrlItem);

    ui->ctrl_dataEdit->setVisible(false);
}

CtrlTab::~CtrlTab()
{
    delete ui;
}

void CtrlTab::setDataCodec(QTextCodec *codec)
{
    dataCodec = codec;
    emit newDataCodec(dataCodec);
}

void CtrlTab::addCtrlItem()
{
    ControlItem::Type type = ControlItem::Command;
    QString buttonName = sender()->objectName();
    if(buttonName.contains("CMD"))
        type = ControlItem::Command;
    else if(buttonName.contains("Slider"))
        type = ControlItem::Slider;
    else if(buttonName.contains("CheckBox"))
        type = ControlItem::CheckBox;
    else if(buttonName.contains("SpinBox"))
        type = ControlItem::SpinBox;
    QBoxLayout* p = static_cast<QBoxLayout*>(ui->ctrl_itemContents->layout());
    ControlItem* c = new ControlItem(type);
    connect(c, &ControlItem::send, this, &CtrlTab::send);
    connect(c, &ControlItem::destroyed, this, &CtrlTab::onCtrlItemDestroyed);
    connect(this, &CtrlTab::newDataCodec, c, &ControlItem::setDataCodec);
    c->setDataCodec(dataCodec);
    p->insertWidget(ctrlItemCount++, c);
}

void CtrlTab::onCtrlItemDestroyed()
{
    ctrlItemCount--;
}


void CtrlTab::on_ctrl_clearButton_clicked()
{
    const QList<ControlItem*> list = ui->ctrl_itemContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly);
    for(auto it = list.begin(); it != list.end(); it++)
        (*it)->deleteLater();
}

void CtrlTab::on_ctrl_importButton_clicked()
{
#ifdef Q_OS_ANDROID
    if(ui->ctrl_importButton->text() == tr("Import"))
    {
        ui->ctrl_dataEdit->setPlainText("# " + tr("Paste the exported data in the box."));
        ui->ctrl_dataEdit->appendPlainText(""); // new line;
        ui->ctrl_itemArea->setVisible(false);
        ui->ctrl_dataEdit->setVisible(true);
        ui->ctrl_importButton->setText(tr("Done"));
    }
    else
    {
        QBoxLayout* p = static_cast<QBoxLayout*>(ui->ctrl_itemContents->layout());
        QStringList dataList = ui->ctrl_dataEdit->toPlainText().split("\n", QString::SkipEmptyParts);
        for(auto it = dataList.begin(); it != dataList.end(); it++)
        {
            if(it->at(0) == '#')
                continue;
            ControlItem* c = new ControlItem(ControlItem::Command);
            connect(c, &ControlItem::send, this, &CtrlTab::send);
            connect(c, &ControlItem::destroyed, this, &CtrlTab::onCtrlItemDestroyed);
            p->insertWidget(ctrlItemCount++, c);
            if(!c->load(*it))
                c->deleteLater();
        }
        ui->ctrl_dataEdit->clear();
        ui->ctrl_itemArea->setVisible(true);
        ui->ctrl_dataEdit->setVisible(false);
        ui->ctrl_importButton->setText(tr("Import"));
    }
#else
    bool flag = true;
    const QList<ControlItem*> list = ui->ctrl_itemContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly);
    QString fileName;
    QBoxLayout* p = static_cast<QBoxLayout*>(ui->ctrl_itemContents->layout());
    fileName = QFileDialog::getOpenFileName(this, tr("Import Control Panel"));
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    flag &= file.open(QFile::ReadOnly | QFile::Text);
    QStringList dataList = QString(file.readAll()).split("\n", QString::SkipEmptyParts);
    for(auto it = dataList.begin(); it != dataList.end(); it++)
    {
        if(it->at(0) == '#')
            continue;
        ControlItem* c = new ControlItem(ControlItem::Command);
        connect(c, &ControlItem::send, this, &CtrlTab::send);
        connect(c, &ControlItem::destroyed, this, &CtrlTab::onCtrlItemDestroyed);
        p->insertWidget(ctrlItemCount++, c);
        if(!c->load(*it))
            c->deleteLater();
    }
    file.close();
    QMessageBox::information(this, tr("Info"), flag ? tr("Successed!") : tr("Failed!"));
#endif
}


void CtrlTab::on_ctrl_exportButton_clicked()
{
#ifdef Q_OS_ANDROID
    if(ui->ctrl_exportButton->text() == tr("Export"))
    {
        if(ctrlItemCount == 0)
        {
            QMessageBox::information(this, tr("Info"), tr("Please add item first"));
            return;
        }
        const QList<ControlItem*> list = ui->ctrl_itemContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly);
        ui->ctrl_dataEdit->setPlainText("# " + tr("Copy all text in this box and save it to somewhere."));
        ui->ctrl_dataEdit->appendPlainText("# " + tr("To import, click the Import button, then paste the text back."));
        for(auto it = list.begin(); it != list.end(); it++)
            ui->ctrl_dataEdit->appendPlainText((*it)->save());
        ui->ctrl_itemArea->setVisible(false);
        ui->ctrl_dataEdit->setVisible(true);
        ui->ctrl_exportButton->setText(tr("Done"));
    }
    else
    {
        ui->ctrl_dataEdit->clear();
        ui->ctrl_itemArea->setVisible(true);
        ui->ctrl_dataEdit->setVisible(false);
        ui->ctrl_exportButton->setText(tr("Export"));
    }
#else
    if(ctrlItemCount == 0)
    {
        QMessageBox::information(this, tr("Info"), tr("Please add item first"));
        return;
    }
    bool flag = true;
    const QList<ControlItem*> list = ui->ctrl_itemContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly);
    QString fileName;
    fileName = QFileDialog::getSaveFileName(this, tr("Export Control Panel"), "ctrl_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".txt");
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    flag &= file.open(QFile::WriteOnly | QFile::Text);
    for(auto it = list.begin(); it != list.end(); it++)
        flag &= file.write(((*it)->save() + "\n").toUtf8()) != -1;
    file.close();
    QMessageBox::information(this, tr("Info"), flag ? tr("Successed!") : tr("Failed!"));
#endif
}
