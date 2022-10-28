#include "ctrltab.h"
#include "ui_ctrltab.h"

#include "controlitem.h"
#include "util.h"

#include <QDateTime>
#ifndef Q_OS_ANDROID
#include <QFileDialog>
#else
#include <QClipboard>
#endif
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QScroller>
#include <QMimeData>

CtrlTab::CtrlTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CtrlTab)
{
    ui->setupUi(this);

    connect(ui->ctrl_addCMDButton, &QPushButton::clicked, this, &CtrlTab::addCtrlItem);
    connect(ui->ctrl_addSliderButton, &QPushButton::clicked, this, &CtrlTab::addCtrlItem);
    connect(ui->ctrl_addCheckBoxButton, &QPushButton::clicked, this, &CtrlTab::addCtrlItem);
    connect(ui->ctrl_addSpinBoxButton, &QPushButton::clicked, this, &CtrlTab::addCtrlItem);

    commentRegExp = new QRegularExpression("^#.+$", QRegularExpression::MultilineOption);
    commentRegExp->optimize();

    ui->ctrl_dataEdit->hide();
    QScroller::grabGesture(ui->ctrl_itemArea);
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

// remember to change on_ctrl_importButton_clicked() if related code is changed
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
    for(auto it = list.begin(); it != list.end(); ++it)
        (*it)->deleteLater();
}

// remember to change addCtrlItem() if related code is changed
void CtrlTab::on_ctrl_importButton_clicked()
{
#ifdef Q_OS_ANDROID
    if(ui->ctrl_importButton->text() == tr("Import"))
    {
        ui->ctrl_dataEdit->setReadOnly(false);
        ui->ctrl_dataEdit->setPlainText("# " + tr("Paste the exported data in the box."));
        ui->ctrl_dataEdit->appendPlainText(""); // new line;
        ui->ctrl_itemArea->setVisible(false);
        ui->ctrl_dataEdit->setVisible(true);
        ui->ctrl_importButton->setText(tr("Done"));
        // on PC, the file dialog is a modal dialog so the user can't edit the page when importing/exporting
        // on Android, the editing function should be disabled when importing/exporting
        ui->ctrl_clearButton->setEnabled(false);
        ui->ctrl_exportButton->setEnabled(false);
        ui->ctrl_addCMDButton->setEnabled(false);
        ui->ctrl_addSliderButton->setEnabled(false);
        ui->ctrl_addCheckBoxButton->setEnabled(false);
        ui->ctrl_addSpinBoxButton->setEnabled(false);
    }
    else
    {
        loadCtrlPanel(ui->ctrl_dataEdit->toPlainText());
        ui->ctrl_dataEdit->clear();
        ui->ctrl_itemArea->setVisible(true);
        ui->ctrl_dataEdit->setVisible(false);
        ui->ctrl_importButton->setText(tr("Import"));
        ui->ctrl_clearButton->setEnabled(true);
        ui->ctrl_exportButton->setEnabled(true);
        ui->ctrl_addCMDButton->setEnabled(true);
        ui->ctrl_addSliderButton->setEnabled(true);
        ui->ctrl_addCheckBoxButton->setEnabled(true);
        ui->ctrl_addSpinBoxButton->setEnabled(true);
    }
#else
    bool flag = true;
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, tr("Import Control Panel"), QString(), tr("Config files") + " (*.json);;" + tr("All files") + " (*.*)");
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    flag &= file.open(QFile::ReadOnly | QFile::Text);
    loadCtrlPanel(QString::fromUtf8(file.readAll()));
    file.close();
    QMessageBox::information(this, tr("Info"), flag ? tr("Successed!") : tr("Failed!"));
#endif
}

void CtrlTab::loadCtrlPanel(const QString& data)
{
    QBoxLayout* p = static_cast<QBoxLayout*>(ui->ctrl_itemContents->layout());
    QString dataStr = data;
    dataStr.replace(*commentRegExp, "");
    QJsonArray dataArray = QJsonDocument::fromJson(dataStr.toUtf8()).array();
    for(auto it = dataArray.begin(); it != dataArray.end(); ++it)
    {
        ControlItem* c = new ControlItem();
        connect(c, &ControlItem::send, this, &CtrlTab::send);
        connect(c, &ControlItem::destroyed, this, &CtrlTab::onCtrlItemDestroyed);
        connect(this, &CtrlTab::newDataCodec, c, &ControlItem::setDataCodec);
        c->setDataCodec(dataCodec);
        p->insertWidget(ctrlItemCount++, c);
        if(!c->load(it->toObject()))
            c->deleteLater();
    }
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
        ui->ctrl_dataEdit->setReadOnly(true);
        ui->ctrl_dataEdit->setPlainText("# " + tr("Copy all text in this box and save it to somewhere."));
        ui->ctrl_dataEdit->appendPlainText("# " + tr("To import, click the Import button, then paste the text back."));
        const QList<ControlItem*> list = ui->ctrl_itemContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly);
        QJsonArray dataArray;
        for(auto it = list.begin(); it != list.end(); ++it)
            dataArray.append((*it)->save());
        QJsonDocument jsonData;
        jsonData.setArray(dataArray);
        ui->ctrl_dataEdit->appendPlainText(QString::fromUtf8(jsonData.toJson(QJsonDocument::Compact)));
        QApplication::clipboard()->setText(ui->ctrl_dataEdit->toPlainText());
        ui->ctrl_itemArea->setVisible(false);
        ui->ctrl_dataEdit->setVisible(true);
        ui->ctrl_exportButton->setText(tr("Done"));
        Util::showToast(tr("Copied to clipboard"));
        ui->ctrl_clearButton->setEnabled(false);
        ui->ctrl_importButton->setEnabled(false);
        ui->ctrl_addCMDButton->setEnabled(false);
        ui->ctrl_addSliderButton->setEnabled(false);
        ui->ctrl_addCheckBoxButton->setEnabled(false);
        ui->ctrl_addSpinBoxButton->setEnabled(false);
    }
    else
    {
        ui->ctrl_dataEdit->clear();
        ui->ctrl_itemArea->setVisible(true);
        ui->ctrl_dataEdit->setVisible(false);
        ui->ctrl_exportButton->setText(tr("Export"));
        ui->ctrl_clearButton->setEnabled(true);
        ui->ctrl_importButton->setEnabled(true);
        ui->ctrl_addCMDButton->setEnabled(true);
        ui->ctrl_addSliderButton->setEnabled(true);
        ui->ctrl_addCheckBoxButton->setEnabled(true);
        ui->ctrl_addSpinBoxButton->setEnabled(true);
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
    fileName = QFileDialog::getSaveFileName(this, tr("Export Control Panel"), "ctrl_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".json", tr("Config files") + " (*.json);;" + tr("All files") + " (*.*)");
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    flag &= file.open(QFile::WriteOnly | QFile::Text);
    QJsonArray dataArray;
    for(auto it = list.begin(); it != list.end(); ++it)
        dataArray.append((*it)->save());
    QJsonDocument jsonData;
    jsonData.setArray(dataArray);
    flag &= file.write(jsonData.toJson()) != -1;
    file.close();
    QMessageBox::information(this, tr("Info"), flag ? tr("Successed!") : tr("Failed!"));
#endif
}

void CtrlTab::dragEnterEvent(QDragEnterEvent *event)
{
    QList<QUrl> urlList = event->mimeData()->urls();
    if(urlList.size() == 1 && !Util::getValidLocalFilename(urlList).isEmpty())
        event->acceptProposedAction();
}

void CtrlTab::dropEvent(QDropEvent *event)
{
    QString filename = Util::getValidLocalFilename(event->mimeData()->urls());
    if(!filename.isEmpty())
    {
        if(filename.isEmpty())
            return;
        QFile file(filename);
        file.open(QFile::ReadOnly | QFile::Text);
        loadCtrlPanel(QString::fromUtf8(file.readAll()));
        file.close();
    }
}
