#include "filetab.h"
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
    m_checksumCalc = new AsyncCRC(this);
    m_fileXceiverThread = new QThread(this);
    m_fileXceiver = new FileXceiver(this);

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
    on_Raw_throttleGrp_buttonClicked(nullptr);
    setAcceptDrops(true);

    // not implemented yet
    ui->Raw_throttleMsButton->setHidden(true);
    ui->Raw_throttleMsBox->setHidden(true);
}

FileTab::~FileTab()
{
    QMetaObject::invokeMethod(m_fileXceiver, "stop", Qt::QueuedConnection);
    delete ui;
    m_checksumThread->terminate();
    m_checksumThread->wait(3000);
    delete m_fileXceiver;
    m_fileXceiverThread->terminate();
    m_fileXceiverThread->wait(3000);
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

void FileTab::on_Raw_throttleGrp_buttonClicked(QAbstractButton* button)
{
    Q_UNUSED(button)
    ui->Raw_throttleByteBox->setEnabled(ui->Raw_throttleByteButton->isChecked());
    ui->Raw_throttleMsBox->setEnabled(ui->Raw_throttleMsButton->isChecked());
    ui->Raw_throttleWaitMsBox->setEnabled(!ui->Raw_throttleNoneButton->isChecked());
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

QString FileTab::getValidFilename(const QList<QUrl> urlList)
{
    for(auto url : urlList)
    {
        if(url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile())
            return url.toLocalFile();
    }
    return QString();
}

void FileTab::dragEnterEvent(QDragEnterEvent *event)
{
    if(!getValidFilename(event->mimeData()->urls()).isEmpty())
        event->acceptProposedAction();
}

void FileTab::dropEvent(QDropEvent *event)
{
    QString filename = getValidFilename(event->mimeData()->urls());
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
    // This function is useless now.
    // The QtNative.getUriWithValidPermission() will only check the persisted permissions in Qt 5.15.2.
    // But when an app shared a file by Intent, the permissions are usually transient.
    // This bug is fixed in the commit ec497d5e6587ac247a326fb9a0a11c37bb197786 of qtbase,
    // which might be released in Qt 5.15.6.
    // So I'm waiting for the opensource release of Qt 5.15.6.

    // set the file path
    Q_UNUSED(thiz)
    const char* str = env->GetStringUTFChars(text, nullptr);
    QString fileName(str);
    env->ReleaseStringUTFChars(text, str);
    m_currInstance->onFilePathSet(fileName);

    m_currInstance->ui->receiveModeButton->setChecked(false);
    m_currInstance->ui->sendModeButton->setChecked(true);
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
                qsizetype waitTime = (ui->Raw_throttleNoneButton->isChecked() ? -1 : ui->Raw_throttleWaitMsBox->value());
                qsizetype batchByteNum = ui->Raw_throttleByteBox->value();
                FileXceiver::ThrottleArgument arg;
                arg.waitTime = waitTime;
                arg.batchByteNum = batchByteNum;
                QMetaObject::invokeMethod(m_fileXceiver, "setThrottleArgument", Qt::QueuedConnection, Q_ARG(FileXceiver::ThrottleArgument, arg));
            }
            QMetaObject::invokeMethod(m_fileXceiver, "startTransmit", Qt::QueuedConnection, Q_ARG(QString, ui->filePathEdit->text()));
        }
        else
        {
            QMetaObject::invokeMethod(m_fileXceiver, "startReceive", Qt::QueuedConnection, Q_ARG(QString, ui->filePathEdit->text()));
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
        if(ui->receiveModeButton->isChecked() && currentProtocol() == FileXceiver::RawProtocol)
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
    if(ui->receiveModeButton->isChecked() && currentProtocol() == FileXceiver::RawProtocol)
        ui->sizeLabel->setText("");
    else
    {
        m_fileSize = QFileInfo(ui->filePathEdit->text()).size();
        ui->sizeLabel->setText(QLocale(QLocale::English).toString(m_fileSize) + " Bytes");
    }
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

