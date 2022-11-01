#include "filetab.h"
#include "util.h"
#include "ui_filetab.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QMimeData>
#include <QElapsedTimer>
#include <QDateTime>

FileTab* FileTab::m_currInstance = nullptr;

FileTab::FileTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileTab)
{
    ui->setupUi(this);

    m_checksumThread = new QThread(this);
    m_checksumCalc = new AsyncCRC();
    m_fileXceiverThread = new QThread(this);
    m_fileXceiver = new FileXceiver();
    m_intValidator = new QIntValidator(this);
    m_intValidator->setBottom(0);

    m_checksumCalc->setNotify(true);
    m_checksumCalc->setParam(32, 0x04C11DB7ULL, 0xFFFFFFFFULL, true, true, 0xFFFFFFFFULL); // CRC-32
    m_checksumCalc->moveToThread(m_checksumThread);
    m_checksumThread->start();
    connect(m_checksumCalc, &AsyncCRC::result, this, &FileTab::onChecksumUpdated);
    connect(m_checksumCalc, &AsyncCRC::fileError, this, &FileTab::onChecksumError);

    m_fileXceiver->moveToThread(m_fileXceiverThread);
    m_fileXceiverThread->start();
    connect(m_fileXceiver, &FileXceiver::startResult, this, &FileTab::onStartResultArrived);
    connect(m_fileXceiver, &FileXceiver::dataTransmitted, this, &FileTab::onDataTransmitted);
    connect(m_fileXceiver, &FileXceiver::dataReceived, this, &FileTab::onDataReceived);
    connect(m_fileXceiver, &FileXceiver::finished, this, &FileTab::onFinished);


    m_currInstance = this;

#ifdef Q_OS_ANDROID

    // register native method
    JNINativeMethod methods[] {{"shareFile", "(Ljava/lang/String;)V", reinterpret_cast<void *>(onSharedFileReceived)}};
    QAndroidJniEnvironment env;
    jclass javaClass = env->FindClass("priv/wh201906/serialtest/MainActivity");
    env->RegisterNatives(javaClass,
                         methods,
                         sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(javaClass);

#endif

    ui->centralLayout->setStretchFactor(ui->statusEdit, 1);
    ui->protoBox->addItem(tr("Raw"), QVariant::fromValue(FileXceiver::RawProtocol));
    ui->RawTx_throttleByteBox->setValidator(m_intValidator);
    ui->RawTx_throttleMsBox->setValidator(m_intValidator);
    ui->RawTx_throttleWaitMsBox->setValidator(m_intValidator);
    ui->RawRx_autostopByteBox->setValidator(m_intValidator);

    on_tipsBackButton_clicked();

    connect(ui->protoBox, &QComboBox::currentTextChanged, this, &FileTab::onModeProtocolChanged);
    connect(ui->sendModeButton, &QRadioButton::toggled, this, &FileTab::onModeProtocolChanged); // just set one of them, the slot will be called when setChecked() is called



    // not implemented yet
    ui->RawTx_throttleMsButton->hide();
    ui->RawTx_throttleMsBox->hide();
}

FileTab::~FileTab()
{
    QMetaObject::invokeMethod(m_fileXceiver, "stop", Qt::QueuedConnection);
    delete ui;
    delete m_checksumCalc;
    m_checksumThread->terminate();
    m_checksumThread->wait(1000);
    delete m_fileXceiver;
    m_fileXceiverThread->terminate();
    m_fileXceiverThread->wait(2000);
}

void FileTab::initSettings()
{
    m_settings = MySettings::defaultSettings();
    loadPreference();

    connect(ui->sendModeButton, &QRadioButton::clicked, this, &FileTab::saveFilePreference);
    connect(ui->receiveModeButton, &QRadioButton::clicked, this, &FileTab::saveFilePreference);
    connect(ui->protoBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FileTab::saveFilePreference);

    connect(ui->RawTx_throttleNoneButton, &QRadioButton::clicked, this, &FileTab::saveFilePreference);
    connect(ui->RawTx_throttleByteButton, &QRadioButton::clicked, this, &FileTab::saveFilePreference);
    connect(ui->RawTx_throttleMsButton, &QRadioButton::clicked, this, &FileTab::saveFilePreference);
    connect(ui->RawTx_throttleByteBox, &QComboBox::currentTextChanged, this, &FileTab::saveFilePreference);
    connect(ui->RawTx_throttleMsBox, &QComboBox::currentTextChanged, this, &FileTab::saveFilePreference);
    connect(ui->RawTx_throttleWaitMsBox, &QComboBox::currentTextChanged, this, &FileTab::saveFilePreference);

    connect(ui->RawRx_autostopNoneButton, &QRadioButton::clicked, this, &FileTab::saveFilePreference);
    connect(ui->RawRx_autostopByteButton, &QRadioButton::clicked, this, &FileTab::saveFilePreference);
    connect(ui->RawRx_autostopByteBox, &QComboBox::currentTextChanged, this, &FileTab::saveFilePreference);

    connect(ui->filePathEdit, &QLineEdit::editingFinished, this, &FileTab::saveFilePreference);
}

FileXceiver *FileTab::fileXceiver()
{
    return m_fileXceiver;
}

void FileTab::on_fileBrowseButton_clicked()
{
    QString fileName;
    if(ui->sendModeButton->isChecked())
        fileName = QFileDialog::getOpenFileName(this);
    else
        fileName = QFileDialog::getSaveFileName(this);
    if(fileName.isEmpty())
        return;
    onFilePathSet(fileName);
}

void FileTab::onFilePathSet(const QString& path)
{
    m_currInstance->ui->filePathEdit->setText(path);
    m_currInstance->updateFileSize();
}

void FileTab::on_RawTx_throttleGrp_buttonClicked(QAbstractButton* button)
{
    Q_UNUSED(button)
    ui->RawTx_throttleByteBox->setEnabled(ui->RawTx_throttleByteButton->isChecked());
    ui->RawTx_throttleMsBox->setEnabled(ui->RawTx_throttleMsButton->isChecked());
    ui->RawTx_throttleWaitMsBox->setEnabled(!ui->RawTx_throttleNoneButton->isChecked());
}

void FileTab::on_RawRx_autostopGrp_buttonClicked(QAbstractButton* button)
{
    Q_UNUSED(button)
    ui->RawRx_autostopByteBox->setEnabled(ui->RawRx_autostopByteButton->isChecked());
}

void FileTab::on_checksumButton_clicked()
{
    ui->checksumLabel->setText(tr("Calculating..."));
    QMetaObject::invokeMethod(m_checksumCalc, "reset", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_checksumCalc, "loadFile", Qt::QueuedConnection, Q_ARG(QString, ui->filePathEdit->text()));
}

void FileTab::onChecksumUpdated(quint64 checksum)
{
    ui->checksumLabel->setText(QString("%1").arg(checksum, 8, 16, QLatin1Char('0')));
}

void FileTab::onChecksumError(AsyncCRC::CRCFileError error)
{
    if(error == AsyncCRC::OpenFileError)
        ui->checksumLabel->setText(tr("Failed to open file."));
    else if(error == AsyncCRC::ReadFileError)
        ui->checksumLabel->setText(tr("Failed to read file."));
}

void FileTab::dragEnterEvent(QDragEnterEvent *event)
{
    QList<QUrl> urlList = event->mimeData()->urls();
    if(urlList.size() == 1 && !Util::getValidLocalFilename(urlList).isEmpty())
        event->acceptProposedAction();
}

void FileTab::dropEvent(QDropEvent *event)
{
    QString filename = Util::getValidLocalFilename(event->mimeData()->urls());
    if(!filename.isEmpty())
        onFilePathSet(filename);
}

void FileTab::showUpTabHelper(int id)
{
    emit showUpTab(id);
}

#ifdef Q_OS_ANDROID
void FileTab::onSharedFileReceived(JNIEnv *env, jobject thiz, jstring text)
{
    // The QtNative.getUriWithValidPermission() will only check the persisted permissions in Qt 5.15.2.
    // But when an app shared a file by Intent, the permissions are usually transient.
    // This bug is fixed in the commit ec497d5e6587ac247a326fb9a0a11c37bb197786 of qtbase,
    // which has not been released yet.
    // To make this work, you need to make a patch from this commit or waiting for the associated release version

    // set the file path
    Q_UNUSED(thiz)
    const char* str = env->GetStringUTFChars(text, nullptr);
    QString fileName(str);
    env->ReleaseStringUTFChars(text, str);
    m_currInstance->onFilePathSet(fileName);

    m_currInstance->ui->receiveModeButton->setChecked(false);
    m_currInstance->ui->sendModeButton->setChecked(true);
    m_currInstance->onModeProtocolChanged();
    m_currInstance->showUpTabHelper(4);
}

#endif

void FileTab::on_clearButton_clicked()
{
    ui->statusEdit->clear();
    ui->progressBar->reset();
}


void FileTab::on_startStopButton_clicked()
{
    // "Start" button
    if(!m_working)
    {
        // precheck
        if(ui->receiveModeButton->isChecked() && QFileInfo::exists(ui->filePathEdit->text()))
        {
            if(QMessageBox::warning(this, tr("Receive"), tr("File already exists\nContinue?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
                return;
        }


        ui->progressBar->reset();
        m_handledSize = 0;
        QMetaObject::invokeMethod(m_fileXceiver, "setProtocol", Qt::QueuedConnection, Q_ARG(FileXceiver::Protocol, currentProtocol()));
        updateFileSize();

        if(ui->sendModeButton->isChecked())
        {
            if(currentProtocol() == FileXceiver::RawProtocol)
            {
                qsizetype waitTime = (ui->RawTx_throttleNoneButton->isChecked() ? -1 : ui->RawTx_throttleWaitMsBox->currentText().toLongLong());
                qsizetype batchByteNum = ui->RawTx_throttleByteBox->currentText().toLongLong();
                FileXceiver::ThrottleArgument arg;
                arg.waitTime = waitTime;
                arg.batchByteNum = batchByteNum;
                QMetaObject::invokeMethod(m_fileXceiver, "setThrottleArgument", Qt::QueuedConnection, Q_ARG(FileXceiver::ThrottleArgument, arg));
            }
            QMetaObject::invokeMethod(m_fileXceiver, "startTransmit", Qt::QueuedConnection, Q_ARG(QString, ui->filePathEdit->text()));
        }
        else
        {
            if(currentProtocol() == FileXceiver::RawProtocol)
            {
                qsizetype num = ui->RawRx_autostopNoneButton->isChecked() ? -1 : ui->RawRx_autostopByteBox->currentText().toLongLong();
                QMetaObject::invokeMethod(m_fileXceiver, "setAutostop", Qt::QueuedConnection, Q_ARG(qsizetype, num));
                QMetaObject::invokeMethod(m_fileXceiver, "startReceive", Qt::QueuedConnection, Q_ARG(QString, ui->filePathEdit->text()));
            }
        }
    }
    // "Stop" button
    else
    {
        stop();
        showMessage(tr("Stopped"));
    }
}

void FileTab::onDataTransmitted(qsizetype num)
{
    m_handledSize += num;
    ui->progressBar->setValue((double)m_handledSize / m_fileSize * 100.0);
    ui->sizeLabel->setText(QLocale(QLocale::English).toString(m_handledSize) + "/" + QLocale(QLocale::English).toString(m_fileSize) + " Bytes");
}

void FileTab::onDataReceived(qsizetype num)
{
    m_handledSize += num;
    if(currentProtocol() == FileXceiver::RawProtocol && ui->RawRx_autostopByteButton->isChecked())
    {
        ui->progressBar->setValue((double)m_handledSize / m_fileSize * 100.0);
        ui->sizeLabel->setText(QLocale(QLocale::English).toString(m_handledSize) + "/" + QLocale(QLocale::English).toString(m_fileSize) + " Bytes");
    }
    else
        ui->sizeLabel->setText(QLocale(QLocale::English).toString(m_handledSize) + " Bytes");
}

bool FileTab::receiving()
{
    return (ui->receiveModeButton->isChecked() && m_working);
}

void FileTab::showMessage(const QString& msg)
{
    ui->statusEdit->appendPlainText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " " + msg);
}

void FileTab::onFinished()
{
    if(m_working)
    {
        stop();
        showMessage(tr("Finished"));
    }
}

void FileTab::onStartResultArrived(bool result)
{
    m_working = result;
    if(result)
    {
        if(ui->receiveModeButton->isChecked() && currentProtocol() == FileXceiver::RawProtocol && ui->RawRx_autostopNoneButton->isChecked())
            ui->progressBar->setMaximum(0);
        setParameterWidgetEnabled(false);
        ui->startStopButton->setText(tr("Stop"));
        showMessage(tr("Started"));
    }
    else
    {
        showMessage(tr("Failed to start."));
    }
}


void FileTab::stop()
{
    m_working = false;
    QMetaObject::invokeMethod(m_fileXceiver, "stop", Qt::QueuedConnection);
    ui->startStopButton->setText(tr("Start"));
    setParameterWidgetEnabled(true);
    ui->progressBar->setMaximum(100); // for Raw receive
}

void FileTab::updateFileSize()
{
    m_fileSize = -1;
    if(ui->receiveModeButton->isChecked())
    {
        if(currentProtocol() == FileXceiver::RawProtocol)
        {
            if(ui->RawRx_autostopByteButton->isChecked())
                m_fileSize = ui->RawRx_autostopByteBox->currentText().toLongLong();
            else
                m_fileSize = -1;
        }
    }
    else
    {
        m_fileSize = QFileInfo(ui->filePathEdit->text()).size();
    }

    if(m_fileSize == -1)
        ui->sizeLabel->setText("");
    else
        ui->sizeLabel->setText(QLocale(QLocale::English).toString(m_handledSize) + "/" + QLocale(QLocale::English).toString(m_fileSize) + " Bytes");
}

void FileTab::setParameterWidgetEnabled(bool state)
{
    ui->sendReceiveWidget->setEnabled(state);
    ui->protoParamWidget->setEnabled(state);
    ui->filePathEdit->setEnabled(state);
    ui->fileBrowseButton->setEnabled(state);
    ui->checksumButton->setEnabled(state);
}

FileXceiver::Protocol FileTab::currentProtocol()
{
    return ui->protoBox->currentData().value<FileXceiver::Protocol>();
}

void FileTab::on_tipsButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->tipsPage);
}


void FileTab::on_tipsBackButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->mainPage);
}

void FileTab::onModeProtocolChanged()
{
    QWidget* widget = ui->protoParamWidget->currentWidget();
    if(ui->sendModeButton->isChecked())
    {
        if(currentProtocol() == FileXceiver::RawProtocol)
        {
            widget = ui->rawTxParamWidget;
        }
    }
    else
    {
        if(currentProtocol() == FileXceiver::RawProtocol)
        {
            widget = ui->rawRxParamWidget;
        }
    }
    ui->protoParamWidget->setCurrentWidget(widget);
}

void FileTab::loadPreference()
{
    // default preferences are defined in this function
    m_settings->beginGroup("SerialTest_File");

    ui->sendModeButton->setChecked(m_settings->value("SendMode", true).toBool());
    ui->receiveModeButton->setChecked(!(m_settings->value("SendMode", true).toBool()));
    ui->protoBox->setCurrentIndex(m_settings->value("Protocol", 0).toInt());


    ui->RawTx_throttleNoneButton->setChecked(m_settings->value("RawTx_throttleMode", 0).toInt() == 0);
    ui->RawTx_throttleByteButton->setChecked(m_settings->value("RawTx_throttleMode", 0).toInt() == 1);
    ui->RawTx_throttleMsButton->setChecked(m_settings->value("RawTx_throttleMode", 0).toInt() == 2);
    ui->RawTx_throttleByteBox->setCurrentText(m_settings->value("RawTx_throttleByteNum", "1048576").toString());
    ui->RawTx_throttleMsBox->setCurrentText(m_settings->value("RawTx_throttleTimeMs", "0").toString());
    ui->RawTx_throttleWaitMsBox->setCurrentText(m_settings->value("RawTx_throttleWaitMs", "20").toString());

    ui->RawRx_autostopNoneButton->setChecked(!(m_settings->value("RawTx_autostopEnabled", false).toBool()));
    ui->RawRx_autostopByteButton->setChecked(m_settings->value("RawTx_autostopEnabled", false).toBool());
    ui->RawRx_autostopByteBox->setCurrentText(m_settings->value("RawTx_autostopByteNum", "1048576").toString());

    ui->filePathEdit->setText(m_settings->value("FilePath", "").toString());
    m_settings->endGroup();

    onModeProtocolChanged();
    on_RawTx_throttleGrp_buttonClicked(nullptr);
    on_RawRx_autostopGrp_buttonClicked(nullptr);
}

void FileTab::saveFilePreference()
{
    if(m_settings->group() != "")
        return;
    m_settings->beginGroup("SerialTest_File");
    m_settings->setValue("SendMode", ui->sendModeButton->isChecked());
    m_settings->setValue("Protocol", ui->protoBox->currentIndex());

    int mode;
    mode = ui->RawTx_throttleNoneButton->isChecked() ? 0 : (ui->RawTx_throttleByteButton->isChecked() ? 1 : 2);
    m_settings->setValue("RawTx_throttleMode", mode);
    m_settings->setValue("RawTx_throttleByteNum", ui->RawTx_throttleByteBox->currentText());
    m_settings->setValue("RawTx_throttleTimeMs", ui->RawTx_throttleMsBox->currentText());
    m_settings->setValue("RawTx_throttleWaitMs", ui->RawTx_throttleWaitMsBox->currentText());

    m_settings->setValue("RawTx_autostopEnabled", ui->RawRx_autostopByteButton->isChecked());
    m_settings->setValue("RawTx_autostopByteNum", ui->RawRx_autostopByteBox->currentText());

    m_settings->setValue("FilePath", ui->filePathEdit->text());
    m_settings->endGroup();
}
