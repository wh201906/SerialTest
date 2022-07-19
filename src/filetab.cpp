#include "filetab.h"
#include "ui_filetab.h"

#include <QFileDialog>
#include <QDebug>

FileTab::FileTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileTab)
{
    ui->setupUi(this);

    m_checksumThread = new QThread();
    m_checksumCalc = new AsyncCRC();
    m_checksumCalc->setNotify(true);
    m_checksumCalc->setParam(32, 0x04C11DB7ULL, 0xFFFFFFFFULL, true, true, 0xFFFFFFFFULL); // zlib CRC-32
    m_checksumCalc->moveToThread(m_checksumThread);
    m_checksumThread->start();
    connect(m_checksumCalc, &AsyncCRC::result, this, &FileTab::onChecksumUpdated);

#ifdef Q_OS_ANDROID
    m_currInstance = this;

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
    on_Raw_throttleGrp_buttonClicked(nullptr);
}

FileTab::~FileTab()
{
    delete ui;
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
    ui->filePathEdit->setText(fileName);
    // in onSharedFileReceived()
    QFileInfo info(fileName);
    ui->sizeLabel->setText(QLocale(QLocale::English).toString(info.size()) + " Bytes");
}

void FileTab::on_Raw_throttleGrp_buttonClicked(QAbstractButton* button)
{
    Q_UNUSED(button)
    ui->Raw_throttleByteBox->setEnabled(ui->Raw_throttleByteButton->isChecked());
    ui->Raw_throttleMsBox->setEnabled(ui->Raw_throttleMsButton->isChecked());
}

void FileTab::on_checksumButton_clicked()
{
    QFile file(ui->filePathEdit->text());
    if(!file.open(QFile::ReadOnly))
    {
        ui->checksumLabel->setText(tr("Failed to open file."));
        return;
    }
    ui->checksumLabel->setText(tr("Calculating..."));
    QMetaObject::invokeMethod(m_checksumCalc, "reset", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_checksumCalc, "addData", Qt::QueuedConnection, Q_ARG(QByteArray, file.readAll()));
}

void FileTab::onChecksumUpdated(quint64 checksum)
{
    ui->checksumLabel->setText(QString("%1").arg(checksum, 8, 16, QLatin1Char('0')));
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
    m_currInstance->ui->filePathEdit->setText(fileName);

    // in on_fileBrowseButton_clicked()
    QFileInfo info(fileName);
    m_currInstance->ui->sizeLabel->setText(QLocale(QLocale::English).toString(info.size()) + " Bytes");

    m_currInstance->showUpTabHelper(4);
}

FileTab* FileTab::m_currInstance = nullptr;

#endif
