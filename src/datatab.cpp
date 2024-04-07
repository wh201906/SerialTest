#include "datatab.h"
#include "util.h"
#include "ui_datatab.h"

#include <QTimer>
#include <QTextCodec>
#include <QMessageBox>
#include <QClipboard>
#include <QFileDialog>
#include <QSerialPort>
#include <QDateTime>
#include <QDebug>

DataTab::DataTab(QByteArray* RxBuf, QVector<Metadata>* RxMetadataBuf, QByteArray* TxBuf, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataTab),
    rawReceivedData(RxBuf),
    RxMetadata(RxMetadataBuf),
    rawSendedData(TxBuf)
{
    ui->setupUi(this);
#ifdef Q_OS_ANDROID
    m_currInstance = this;

    // register native method
    JNINativeMethod methods[] {{"shareText", "(Ljava/lang/String;)V", reinterpret_cast<void *>(onSharedTextReceived)}};
    QAndroidJniEnvironment env;
    jclass javaClass = env->FindClass("priv/wh201906/serialtest/MainActivity");
    env->RegisterNatives(javaClass,
                         methods,
                         sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(javaClass);

#endif
    repeatTimer = new QTimer();
    RxSlider = ui->receivedEdit->verticalScrollBar();
    ui->dataTabSplitter->handle(1)->installEventFilter(this); // the id of the 1st visible handle is 1 rather than 0

    connect(ui->sendEdit, &QLineEdit::returnPressed, this, &DataTab::on_sendButton_clicked);
    connect(repeatTimer, &QTimer::timeout, this, &DataTab::on_sendButton_clicked);
}

DataTab::~DataTab()
{
    delete ui;
}

void DataTab::onConnTypeChanged(Connection::Type type)
{
    ui->data_flowControlBox->setVisible(type == Connection::SerialPort);
}

void DataTab::initSettings()
{
    settings = MySettings::defaultSettings();
    loadPreference();

    connect(ui->receivedHexBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->receivedLatestBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->receivedTimestampBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->receivedRealtimeBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->sendedHexBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->sendedEnableBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->data_unescapeBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->data_suffixBox, &QGroupBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->data_suffixTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DataTab::saveDataPreference);
    connect(ui->data_suffixEdit, &QLineEdit::editingFinished, this, &DataTab::saveDataPreference);
    // this might be changed by the program, so use stateChanged() rather than clicked()
    connect(ui->data_repeatCheckBox, &QCheckBox::stateChanged, this, &DataTab::saveDataPreference);
    connect(ui->repeatDelayEdit, &QLineEdit::editingFinished, this, &DataTab::saveDataPreference);
    connect(ui->data_flowDTRBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
    connect(ui->data_flowRTSBox, &QCheckBox::clicked, this, &DataTab::saveDataPreference);
}

bool DataTab::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->dataTabSplitter->handle(1))
    {
        // double click the handle to reset the size
        if(event->type() == QEvent::MouseButtonDblClick)
        {
            QList<int> newSizes = ui->dataTabSplitter->sizes(); // 2 elements
            newSizes[1] += newSizes[0];
            newSizes[0] = newSizes[1] * 0.5;
            newSizes[1] -= newSizes[0];
            ui->dataTabSplitter->setSizes(newSizes);
        }
        // save layout when mouse button is released
        else if(event->type() == QEvent::MouseButtonRelease)
        {
            QList<int> sizes = ui->dataTabSplitter->sizes(); // 2 elements
            double ratio = (double)sizes[0] / (sizes[0] + sizes[1]);
            settings->beginGroup("SerialTest_Data");
            settings->setValue("SplitRatio", ratio);
            settings->endGroup();
        }
    }
    return false;
}

void DataTab::showEvent(QShowEvent *ev)
{
    Q_UNUSED(ev)
    // ui->dataTabSplitter->sizes() will return all 0 if the widgets are invisible
    // the widgets are visible after this event happens
    settings->beginGroup("SerialTest_Data");
    QList<int> newSizes = ui->dataTabSplitter->sizes();
    double ratio = settings->value("SplitRatio", 0.5).toDouble();
    settings->endGroup();

    newSizes[1] += newSizes[0];
    newSizes[0] = newSizes[1] * ratio;
    newSizes[1] -= newSizes[0];

    ui->dataTabSplitter->setSizes(newSizes);
}

void DataTab::on_data_encodingSetButton_clicked()
{
    QTextCodec* newCodec;
    QComboBox* box;
    box = ui->data_encodingNameBox;
    newCodec = QTextCodec::codecForName(box->currentText().toLatin1());
    if(newCodec != nullptr)
    {
        if(RxDecoder != nullptr)
            delete RxDecoder;
        dataCodec = newCodec;
        box->setCurrentText(dataCodec->name());
        emit setDataCodec(dataCodec);
        RxDecoder = dataCodec->makeDecoder(); // clear state machine
        emit setPlotDecoder(dataCodec->makeDecoder());// clear state machine, standalone decoder for DataTab/PlotTab
        settings->beginGroup("SerialTest_Data");
        settings->setValue("Encoding_Name", ui->data_encodingNameBox->currentText());
        settings->endGroup();
    }
    else
    {
        QMessageBox::information(this, tr("Info"), ui->data_encodingNameBox->currentText() + " " + tr("is not a valid encoding."));
        if(dataCodec != nullptr)
            box->setCurrentText(dataCodec->name());
        else
        {
            box->setCurrentText("UTF-8");
            on_data_encodingSetButton_clicked();
        }
    }
}

void DataTab::saveDataPreference()
{
    if(settings->group() != "")
        return;
    settings->beginGroup("SerialTest_Data");
    settings->setValue("Recv_Hex", ui->receivedHexBox->isChecked());
    settings->setValue("Recv_Latest", ui->receivedLatestBox->isChecked());
    settings->setValue("Recv_Timestamp", ui->receivedTimestampBox->isChecked());
    settings->setValue("Recv_Realtime", ui->receivedRealtimeBox->isChecked());
    settings->setValue("Send_Hex", ui->sendedHexBox->isChecked());
    settings->setValue("Send_Enabled", ui->sendedEnableBox->isChecked());
    settings->setValue("Send_Unescape", ui->data_unescapeBox->isChecked());
    settings->setValue("Suffix_Enabled", ui->data_suffixBox->isChecked());
    settings->setValue("Suffix_Type", ui->data_suffixTypeBox->currentIndex());
    settings->setValue("Suffix_Context", ui->data_suffixEdit->text());
    settings->setValue("Repeat_Enabled", ui->data_repeatCheckBox->isChecked());
    settings->setValue("Repeat_Delay", ui->repeatDelayEdit->text());
    settings->setValue("Flow_DTR", ui->data_flowDTRBox->isChecked());
    settings->setValue("Flow_RTS", ui->data_flowRTSBox->isChecked());
    //Encoding_Name will not be saved there, because it need to be verified
    settings->endGroup();
}

// settings->setValue\((.+), ui->(.+)->currentIndex.+
// ui->$2->setCurrentIndex(settings->value($1).toInt());
// settings->setValue\((.+), ui->(.+)->text.+
// ui->$2->setText(settings->value($1).toString());

void DataTab::loadPreference()
{
    // default preferences are defined there
    // setChecked() will trigger on_xxx_stateChanged(), but on_xxx_clicked() will not be triggered
    settings->beginGroup("SerialTest_Data");
    ui->receivedHexBox->setChecked(settings->value("Recv_Hex", false).toBool());
    ui->receivedLatestBox->setChecked(settings->value("Recv_Latest", true).toBool());
    ui->receivedTimestampBox->setChecked(settings->value("Recv_Timestamp", false).toBool());
    ui->receivedRealtimeBox->setChecked(settings->value("Recv_Realtime", true).toBool());
    ui->sendedHexBox->setChecked(settings->value("Send_Hex", false).toBool());
    ui->sendedEnableBox->setChecked(settings->value("Send_Enabled", true).toBool());
    ui->data_unescapeBox->setChecked(settings->value("Send_Unescape", false).toBool());
    ui->data_suffixBox->setChecked(settings->value("Suffix_Enabled", false).toBool());
    ui->data_suffixTypeBox->setCurrentIndex(settings->value("Suffix_Type", 2).toInt());
    ui->data_suffixEdit->setText(settings->value("Suffix_Context", "").toString());
    ui->data_repeatCheckBox->setChecked(settings->value("Repeat_Enabled", false).toBool());
    ui->repeatDelayEdit->setText(settings->value("Repeat_Delay", 1000).toString());
    ui->data_flowDTRBox->setChecked(settings->value("Flow_DTR", false).toBool());
    ui->data_flowRTSBox->setChecked(settings->value("Flow_RTS", false).toBool());
    ui->data_encodingNameBox->setCurrentText(settings->value("Encoding_Name", "UTF-8").toString());
    ui->sendEdit->setText(settings->value("Data", "").toString());
    settings->endGroup();
    on_data_encodingSetButton_clicked();
}

void DataTab::on_data_suffixTypeBox_currentIndexChanged(int index)
{
    ui->data_suffixEdit->setVisible(index != 2 && index != 3);
    ui->data_suffixEdit->setPlaceholderText(tr("Suffix") + ((index == 1) ? "(Hex)" : ""));
}

void DataTab::on_sendedHexBox_stateChanged(int arg1)
{
    isSendedDataHex = (arg1 == Qt::Checked);
    syncSendedEditWithData();
}

void DataTab::on_receivedHexBox_stateChanged(int arg1)
{
    isReceivedDataHex = (arg1 == Qt::Checked);
    syncReceivedEditWithData();
}


void DataTab::on_receivedTimestampBox_stateChanged(int arg1)
{
    RxTimestampEnabled = (arg1 == Qt::Checked);
    syncReceivedEditWithData();
}

void DataTab::on_receivedClearButton_clicked()
{
    clearRxData();
    emit clearGraph();
}

void DataTab::clearRxData()
{
    lastReceivedByte = '\0'; // anything but '\r'
    emit clearReceivedData();
    syncReceivedEditWithData();
}

void DataTab::on_sendedClearButton_clicked()
{
    emit clearSendedData();
    syncSendedEditWithData();
}

void DataTab::on_sendEdit_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    repeatTimer->stop();
    ui->data_repeatCheckBox->setChecked(false);
}

void DataTab::on_data_repeatCheckBox_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked)
    {
        repeatTimer->setInterval(ui->repeatDelayEdit->text().toInt());
        repeatTimer->start();
    }
    else
        repeatTimer->stop();
}

void DataTab::on_receivedCopyButton_clicked()
{
    QString selection = ui->receivedEdit->textCursor().selectedText();
    if(selection.isEmpty())
        QApplication::clipboard()->setText(ui->receivedEdit->toPlainText());
    else
        QApplication::clipboard()->setText(selection);
}

void DataTab::on_sendedCopyButton_clicked()
{
    QString selection = ui->sendedEdit->textCursor().selectedText();
    if(selection.isEmpty())
        QApplication::clipboard()->setText(ui->sendedEdit->toPlainText());
    else
        QApplication::clipboard()->setText(selection);
}

void DataTab::on_receivedExportButton_clicked()
{
    bool flag = true;
    QString fileName, selection;
    fileName = QFileDialog::getSaveFileName(this, tr("Export received data"), "recv_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".txt");
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    selection = ui->receivedEdit->textCursor().selectedText();
    if(selection.isEmpty())
    {
        flag &= file.open(QFile::WriteOnly);
        flag &= file.write(*rawReceivedData) != -1;
    }
    else
    {
        flag &= file.open(QFile::WriteOnly | QFile::Text);
        flag &= file.write(dataCodec->fromUnicode(selection.replace(QChar(0x2029), '\n'))) != -1;
    }
    file.close();
    QMessageBox::information(this, tr("Info"), flag ? tr("Successed!") : tr("Failed!"));
}

void DataTab::on_sendedExportButton_clicked()
{
    bool flag = true;
    QString fileName, selection;
    fileName = QFileDialog::getSaveFileName(this, tr("Export sended data"), "send_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".txt");
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    selection = ui->sendedEdit->textCursor().selectedText();
    if(selection.isEmpty())
    {
        flag &= file.open(QFile::WriteOnly);
        flag &= file.write(*rawSendedData) != -1;
    }
    else
    {
        flag &= file.open(QFile::WriteOnly | QFile::Text);
        flag &= file.write(dataCodec->fromUnicode(selection.replace(QChar(0x2029), '\n'))) != -1;
    }
    file.close();
    QMessageBox::information(this, tr("Info"), flag ? tr("Successed!") : tr("Failed!"));
}

void DataTab::on_receivedUpdateButton_clicked()
{
    syncReceivedEditWithData();
}

void DataTab::on_sendButton_clicked()
{
    QByteArray data;
    if(isSendedDataHex)
        data = QByteArray::fromHex(ui->sendEdit->text().toLatin1());
    else
    {
        if(unescapeSendedData)
            data = Util::unescape(ui->sendEdit->text(), dataCodec);
        else
            data = dataCodec->fromUnicode(ui->sendEdit->text());
    }
    if(ui->data_suffixBox->isChecked())
    {
        if(ui->data_suffixTypeBox->currentIndex() == 0)
        {
            if(unescapeSendedData)
                data += Util::unescape(ui->data_suffixEdit->text(), dataCodec);
            else
                data += dataCodec->fromUnicode(ui->data_suffixEdit->text());
        }
        else if(ui->data_suffixTypeBox->currentIndex() == 1)
            data += QByteArray::fromHex(ui->data_suffixEdit->text().toLatin1());
        else if(ui->data_suffixTypeBox->currentIndex() == 2)
            data += "\r\n";
        else if(ui->data_suffixTypeBox->currentIndex() == 3)
            data += "\n";
    }

    emit send(data);
}

void DataTab::syncReceivedEditWithData()
{
    RxSlider->blockSignals(true);
    if(isReceivedDataHex)
    {
        if(RxTimestampEnabled)
        {
            ui->receivedEdit->clear();
            for(const Metadata& item : qAsConst(*RxMetadata))
            {
                QByteArray dataItem = rawReceivedData->mid(item.pos, item.len);
                ui->receivedEdit->appendPlainText(stringWithTimestamp(dataItem.toHex(' '), item.timestamp));
            }
        }
        else
        {
            ui->receivedEdit->setPlainText(rawReceivedData->toHex(' ') + ' ');
        }
    }
    else
    {
        if(RxTimestampEnabled)
        {
            ui->receivedEdit->clear();
            for(const Metadata& item : qAsConst(*RxMetadata))
            {
                QByteArray dataItem = rawReceivedData->mid(item.pos, item.len);
                ui->receivedEdit->appendPlainText(stringWithTimestamp(dataCodec->toUnicode(dataItem), item.timestamp));
            }
        }
        else
        {
            // sync, use QTextCodec
            ui->receivedEdit->setPlainText(dataCodec->toUnicode(*rawReceivedData));
        }
    }
    RxSlider->blockSignals(false);
//    qDebug() << toHEX(*rawReceivedData);
}

void DataTab::syncSendedEditWithData()
{
    if(isSendedDataHex)
        ui->sendedEdit->setPlainText(rawSendedData->toHex(' ') + ' ');
    else
        ui->sendedEdit->setPlainText(dataCodec->toUnicode(*rawSendedData));
}

void DataTab::setConnection(Connection* conn)
{
    m_connection = conn;
}

void DataTab::onConnEstablished()
{
    if(m_connection->type() == Connection::SerialPort)
    {
        ui->data_flowRTSBox->setVisible(m_connection->getSerialPortArgument().flowControl != QSerialPort::HardwareControl);
        // sync states from serial to UI
        // ui->data_flowRTSBox->setChecked(m_connection->SP_isRequestToSend());
        // ui->data_flowDTRBox->setChecked(m_connection->SP_isDataTerminalReady());

        // sync states from UI to serial
        //
        // Some devices, such as Quectel EC200U/EC600U/EG912U, may not support getting or setting the DTR/RTS signals.
        // When attempting these operations on such devices, they may report QSerialPort::UnknownError instead of QSerialPort::UnsupportedOperationError.
        // This can cause the Connection class to close the device unexpectedly.
        // These errors should be ignored when syncing the DTR/RTS state with the device.

        const auto oldErrorList = m_connection->SP_getIgnoredErrorList();
        auto errorList = oldErrorList;
        errorList.append(QSerialPort::UnknownError);
        m_connection->SP_setIgnoredErrorList(errorList);
        if(ui->data_flowDTRBox->isChecked() != m_connection->SP_isDataTerminalReady())
            on_data_flowDTRBox_clicked(ui->data_flowDTRBox->isChecked());
        if(ui->data_flowRTSBox->isChecked() != m_connection->SP_isRequestToSend())
            on_data_flowRTSBox_clicked(ui->data_flowRTSBox->isChecked());
        m_connection->SP_setIgnoredErrorList(oldErrorList);
    }
}

void DataTab::setRepeat(bool state)
{
    ui->data_repeatCheckBox->setChecked(state);
    // stateChanged() will be emitted
}

bool DataTab::getRxRealtimeState()
{
    return ui->receivedRealtimeBox->isChecked();
}


void DataTab::appendSendedData(const QByteArray& data)
{
    ui->sendedEdit->moveCursor(QTextCursor::End);
    if(isSendedDataHex)
    {
        ui->sendedEdit->insertPlainText(data.toHex(' ') + ' ');
        TxHexCounter += data.length();
        if(TxHexCounter > 5000)
        {
            ui->sendedEdit->insertPlainText("\n");
            TxHexCounter = 0;
        }
    }
    else
    {
        ui->sendedEdit->insertPlainText(dataCodec->toUnicode(data));
    }
}

// TODO:
// split sync process, add processEvents()
// void MainWindow::syncEditWithData()
void DataTab::appendReceivedData(const QByteArray &data, const QVector<Metadata>& metadata)
{
    // Record cursor position and selection
    QTextCursor textCursor = ui->receivedEdit->textCursor();
    int sliderPos;

    if(!ui->receivedLatestBox->isChecked())
    {
        // Record slider position
        sliderPos = RxSlider->sliderPosition();
    }

    ui->receivedEdit->moveCursor(QTextCursor::End);
    if(isReceivedDataHex)
    {
        qint64 offset = 0;
        QByteArray dataItem;
        if(RxTimestampEnabled)
        {
            for(const Metadata& item : metadata)
            {
                dataItem = data.mid(offset, item.len);
                offset += item.len;
                ui->receivedEdit->appendPlainText(stringWithTimestamp(dataItem.toHex(' ') + ' ', item.timestamp));
            }
        }

        dataItem = data.mid(offset);
        ui->receivedEdit->insertPlainText(dataItem.toHex(' ') + ' ');
        RxHexCounter += dataItem.length();
        // QPlainTextEdit is not good at handling long line
        // Seperate for better realtime receiving response
        if(RxHexCounter > 5000)
        {
            ui->receivedEdit->insertPlainText("\n");
            RxHexCounter = 0;
        }
    }
    else
    {
        qint64 offset = 0;
        QByteArray dataItem;
        if(RxTimestampEnabled)
        {
            for(const Metadata& item : metadata)
            {
                dataItem = data.mid(offset, item.len);
                offset += item.len;
                if(lastReceivedByte == '\r' && !dataItem.isEmpty() && *dataItem.cbegin() == '\n')
                    ui->receivedEdit->appendPlainText(stringWithTimestamp(RxDecoder->toUnicode(dataItem.right(dataItem.size() - 1)), item.timestamp));
                else
                    ui->receivedEdit->appendPlainText(stringWithTimestamp(RxDecoder->toUnicode(dataItem), item.timestamp));
                lastReceivedByte = *dataItem.crbegin();
            }
        }

        dataItem = data.mid(offset);
        // append, use QTextDecoder
        // if \r and \n are received seperatedly, the rawReceivedData will be fine, but the receivedEdit will have one more empty line
        // just ignore one of them
        if(lastReceivedByte == '\r' && !dataItem.isEmpty() && *dataItem.cbegin() == '\n')
            ui->receivedEdit->insertPlainText(RxDecoder->toUnicode(dataItem.right(dataItem.size() - 1)));
        else
            ui->receivedEdit->insertPlainText(RxDecoder->toUnicode(dataItem));
        lastReceivedByte = *dataItem.crbegin();
    }
    ui->receivedEdit->setTextCursor(textCursor);
    if(!ui->receivedLatestBox->isChecked())
    {
        // Restore slider position
        RxSlider->setSliderPosition(sliderPos);
    }
    else
    {
        // Sometimes the slider position is a few lines above the maximum position
        RxSlider->setSliderPosition(RxSlider->maximum());
    }
}

void DataTab::on_data_flowDTRBox_clicked(bool checked)
{
    m_connection->SP_setDataTerminalReady(checked);
    // sync state
    ui->data_flowDTRBox->setCheckState(m_connection->SP_isDataTerminalReady() ? Qt::Checked : Qt::Unchecked);
}

void DataTab::on_data_flowRTSBox_clicked(bool checked)
{
    m_connection->SP_setRequestToSend(checked);
    // sync state
    ui->data_flowRTSBox->setCheckState(m_connection->SP_isRequestToSend() ? Qt::Checked : Qt::Unchecked);
}

void DataTab::on_data_unescapeBox_stateChanged(int arg1)
{
    unescapeSendedData = (arg1 == Qt::Checked);
}

void DataTab::on_sendedEdit_selectionChanged()
{
    if(ui->sendedEdit->textCursor().hasSelection())
    {
        ui->sendedExportButton->setText(tr("Export Selected"));
        ui->sendedCopyButton->setText(tr("Copy Selected"));
    }
    else
    {
        ui->sendedExportButton->setText(tr("Export"));
        ui->sendedCopyButton->setText(tr("Copy All"));
    }
}


void DataTab::on_receivedEdit_selectionChanged()
{
    if(ui->receivedEdit->textCursor().hasSelection())
    {
        ui->receivedExportButton->setText(tr("Export Selected"));
        ui->receivedCopyButton->setText(tr("Copy Selected"));
    }
    else
    {
        ui->receivedExportButton->setText(tr("Export"));
        ui->receivedCopyButton->setText(tr("Copy All"));
    }
}

void DataTab::on_sendedEnableBox_stateChanged(int arg1)
{
    emit setTxDataRecording(arg1 == Qt::Checked);
}

void DataTab::showUpTabHelper(int tabID)
{
    emit showUpTab(tabID);
}

inline QString DataTab::stringWithTimestamp(const QString& str, qint64 timestamp)
{
    return ('[' + QDateTime::fromMSecsSinceEpoch(timestamp).toString(Qt::ISODateWithMs) + "] " + str);
}

void DataTab::onRecordDataChanged(bool enabled)
{
    if(enabled)
    {
        connect(ui->sendEdit, &QLineEdit::editingFinished, this, &DataTab::recordDataToBeSent);
        recordDataToBeSent();
    }
    else
    {
        disconnect(ui->sendEdit, &QLineEdit::editingFinished, this, &DataTab::recordDataToBeSent);
        settings->beginGroup("SerialTest_Data");
        settings->remove("Data");
        settings->endGroup();
    }
}

void DataTab::onClearBehaviorChanged(bool clearBoth)
{
    acceptClearSignal = clearBoth;
}

void DataTab::onRxClearSignalReceived()
{
    if(acceptClearSignal)
    {
        on_receivedClearButton_clicked();
    }
}

void DataTab::recordDataToBeSent()
{
    settings->beginGroup("SerialTest_Data");
    settings->setValue("Data", ui->sendEdit->text());
    settings->endGroup();
}

#ifdef Q_OS_ANDROID
void DataTab::onSharedTextReceived(JNIEnv *env, jobject thiz, jstring text)
{
    // append the received text to the end of the sendEdit
    Q_UNUSED(thiz)
    const char* str = env->GetStringUTFChars(text, nullptr);
    //QTimer::singleShot(0, QApplication::instance(), )
    QString ori = m_currInstance->ui->sendEdit->text();
    m_currInstance->ui->sendEdit->setText(ori + QString(str));
    env->ReleaseStringUTFChars(text, str);
    m_currInstance->showUpTabHelper(1);
}

DataTab* DataTab::m_currInstance = nullptr;

#endif


