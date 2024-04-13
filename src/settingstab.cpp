#include "settingstab.h"
#include "ui_settingstab.h"
#include "devicetab.h"

#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QScroller>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#endif

SettingsTab::SettingsTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsTab)
{
    ui->setupUi(this);

#ifdef Q_OS_ANDROID
    ui->opacityWidget->hide();
    ui->Conf_createInCWDButton->hide();
    ui->Conf_createInConfDirButton->hide();
    ui->Conf_tipsEdit->hide();
#else
    ui->Android_fullScreenBox->hide();
    ui->Android_forceLandscapeBox->hide();
    ui->Android_dockBox->hide();
    ui->Android_HWSerialBox->hide();
    connect(ui->Opacity_slider, &QSlider::valueChanged, ui->Opacity_Box, &QSpinBox::setValue);
#endif

    ui->Lang_nameBox->addItem(tr("(System)"), "(sys)");
    ui->Lang_nameBox->addItem(tr("Simplified Chinese"), "zh_CN");
    ui->Lang_nameBox->addItem(tr("Traditional Chinese"), "zh_TW");
    ui->Lang_nameBox->addItem(tr("English"), "en");
    ui->Lang_nameBox->addItem(tr("Norwegian"), "nb_NO");
    ui->Lang_nameBox->addItem(tr("(External File)"), "(ext)");

    ui->Theme_nameBox->addItem(tr("(None)"), "(none)");
    ui->Theme_nameBox->addItem(tr("Dark"), "qdss_dark");
    ui->Theme_nameBox->addItem(tr("Light"), "qdss_light");

    // APP_VERSION is defined in the .pro file
    ui->versionLabel->setText(APP_VERSION);
}

SettingsTab::~SettingsTab()
{
    delete ui;
}

void SettingsTab::on_Opacity_Box_valueChanged(int arg1)
{
    ui->Opacity_slider->blockSignals(true);
    ui->Opacity_slider->setValue(arg1);
    emit opacityChanged(arg1 / 100.0);
    // settings
    ui->Opacity_slider->blockSignals(false);
}


void SettingsTab::on_Font_setButton_clicked()
{
    QFont font = ui->Font_nameBox->currentFont();
    font.setPointSize(ui->Font_sizeBox->value());
    QApplication::setFont(font, "QWidget");
    // QApplication::setFont(font) doesn't work fine on android

    m_settings->beginGroup("SerialTest");
    m_settings->setValue("Font_Name", ui->Font_nameBox->currentFont().family());
    m_settings->setValue("Font_Size", ui->Font_sizeBox->value());
    m_settings->endGroup();
}


void SettingsTab::on_Conf_clearButton_clicked()
{
    QMessageBox::StandardButton btn;
    btn = QMessageBox::warning(this, tr("Warning"), tr("All configurations and history will be deleted!\nAnd this app will be closed!\nContinue?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if(btn == QMessageBox::No)
        return;
    m_settings->clear();
    m_settings->sync();
    QApplication::closeAllWindows();
}

void SettingsTab::initSettings()
{
    m_settings = MySettings::defaultSettings();
    loadPreference();

    // xxx_setButton will handle the preference itself.
    connect(ui->Android_fullScreenBox, &QCheckBox::clicked, this, &SettingsTab::savePreference);
    connect(ui->Android_forceLandscapeBox, &QCheckBox::clicked, this, &SettingsTab::savePreference);
    connect(ui->Android_dockBox, &QCheckBox::clicked, this, &SettingsTab::savePreference);
    connect(ui->General_touchScrollBox, &QCheckBox::clicked, this, &SettingsTab::savePreference);
    // Android_HWSerialBox will handle the preference itself.
    connect(ui->Opacity_Box, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsTab::savePreference);
    connect(ui->Data_recordDataBox, &QCheckBox::clicked, this, &SettingsTab::savePreference);
    connect(ui->Data_mergeTimestampBox, &QCheckBox::clicked, this, &SettingsTab::savePreference);
    connect(ui->Data_mergeTimestampIntervalBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsTab::savePreference);
}


void SettingsTab::on_Conf_createInCWDButton_clicked()
{
    createConfFile("preference.ini");
}


void SettingsTab::on_Conf_createInConfDirButton_clicked()
{
    QDir path(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    path.mkpath(".");
    createConfFile(path.absoluteFilePath("preference.ini"));
}

void SettingsTab::createConfFile(const QString& path, bool overwrite)
{
    QString absolutePath = QDir(path).absolutePath();
    QFile file(path); // don't use absolutePath there(for Android)
    if(file.exists() && !overwrite)
    {
        QMessageBox::information(this, tr("Create"), tr("The file already exists at") + "\n" + absolutePath);
        return;
    }
    if(!file.open(QFile::WriteOnly))
    {
        QMessageBox::information(this, tr("Create"), tr("Cannot create file there."));
        return;
    }

    QFile config(m_settings->fileName());
    if(!config.open(QFile::ReadOnly))
    {
        QMessageBox::information(this, tr("Read"), tr("Cannot read config."));
        return;
    }

    while(!config.atEnd())
        file.write(config.read(128 * 1024));

    file.close();
    config.close();

    QMessageBox::information(this, tr("Create"), tr("Created at") + "\n" + absolutePath);
}

void SettingsTab::on_DataFont_setButton_clicked()
{
    QFont font = ui->DataFont_nameBox->currentFont();
    font.setPointSize(ui->DataFont_sizeBox->value());
    QApplication::setFont(font, "QPlainTextEdit");

    m_settings->beginGroup("SerialTest");
    m_settings->setValue("DataFont_Name", ui->DataFont_nameBox->currentFont().family());
    m_settings->setValue("DataFont_Size", ui->DataFont_sizeBox->value());
    m_settings->endGroup();
}


void SettingsTab::on_Android_fullScreenBox_clicked()
{
    emit fullScreenStateChanged(ui->Android_fullScreenBox->isChecked());
}


void SettingsTab::on_General_touchScrollBox_clicked()
{
    emit TouchScrollStateChanged(ui->General_touchScrollBox->isChecked());
}

void SettingsTab::savePreference()
{
    if(m_settings->group() != "")
        return;
    m_settings->beginGroup("SerialTest");
#ifdef Q_OS_ANDROID
    m_settings->setValue("Android_FullScreen", ui->Android_fullScreenBox->isChecked());
    m_settings->setValue("Android_ForceLandscape", ui->Android_forceLandscapeBox->isChecked());
    m_settings->setValue("Android_Dock", ui->Android_dockBox->isChecked());
#else
    m_settings->setValue("Opacity", ui->Opacity_Box->value());
#endif
    m_settings->setValue("TouchScroll", ui->General_touchScrollBox->isChecked());
    m_settings->endGroup();
    // Android_HWSerialBox will handle the preference itself.
    m_settings->beginGroup("SerialTest_Data");
    m_settings->setValue("RecordData", ui->Data_recordDataBox->isChecked());
    m_settings->setValue("MergeTimestamp", ui->Data_mergeTimestampBox->isChecked());
    m_settings->setValue("TimestampInterval", ui->Data_mergeTimestampIntervalBox->value());
    m_settings->endGroup();
}

void SettingsTab::loadPreference()
{
    m_settings->beginGroup("SerialTest");

    ui->Conf_maxHistoryBox->setValue(m_settings->value("History_MaxCount", 200).toInt());
    int langId = ui->Lang_nameBox->findData(m_settings->value("Lang_Name", "(sys)").toString());
    ui->Lang_nameBox->setCurrentIndex((langId == -1) ? 0 : langId);
    ui->Lang_filePathEdit->setText(m_settings->value("Lang_Path", "").toString());
    ui->Android_fullScreenBox->setChecked(m_settings->value("Android_FullScreen", false).toBool());
    ui->Android_forceLandscapeBox->setChecked(m_settings->value("Android_ForceLandscape", true).toBool());
    ui->Android_dockBox->setChecked(m_settings->value("Android_Dock", false).toBool());
    ui->General_touchScrollBox->setChecked(m_settings->value("TouchScroll", true).toBool());
    ui->Opacity_Box->setValue(m_settings->value("Opacity", 100).toInt());
    ui->General_simultaneousClearBox->setChecked(m_settings->value("ClearBothRxDataAndGraph", false).toBool());
    int themeId = ui->Theme_nameBox->findData(m_settings->value("Theme_Name", "(none)").toString());
    ui->Theme_nameBox->setCurrentIndex((themeId == -1) ? 0 : themeId);

    // QApplication::font() might return wrong result
    // If fonts are not specified in config file, don't touch them.
    QString tmpFontName;
    int tmpFontSize;
    bool fontValid = false, dataFontValid = false;
    tmpFontName = m_settings->value("Font_Name", "").toString();
    tmpFontSize = m_settings->value("Font_Size", -1).toInt();
    if(!tmpFontName.isEmpty() && tmpFontSize != -1 && tmpFontName == QFont(tmpFontName).family())
    {
        ui->Font_nameBox->setCurrentFont(QFont(tmpFontName));
        ui->Font_sizeBox->setValue(tmpFontSize);
        fontValid = true;
    }
    tmpFontName = m_settings->value("DataFont_Name", "").toString();
    tmpFontSize = m_settings->value("DataFont_Size", -1).toInt();
    if(!tmpFontName.isEmpty() && tmpFontSize != -1 && tmpFontName == QFont(tmpFontName).family())
    {
        ui->DataFont_nameBox->setCurrentFont(QFont(tmpFontName));
        ui->DataFont_sizeBox->setValue(tmpFontSize);
        dataFontValid = true;
    }

    m_settings->endGroup();

    m_settings->beginGroup("SerialTest_Connect");
#ifdef Q_OS_ANDROID
    ui->Android_HWSerialBox->setChecked(m_settings->value("Android_HWSerial", false).toBool());
#endif
    m_settings->endGroup();
    m_settings->beginGroup("SerialTest_Data");
    ui->Data_recordDataBox->setChecked(m_settings->value("RecordData", false).toBool());
    ui->Data_mergeTimestampBox->setChecked(m_settings->value("MergeTimestamp", true).toBool());
    ui->Data_mergeTimestampIntervalBox->setValue(m_settings->value("TimestampInterval", 10).toInt());
    m_settings->endGroup();

    // Language is applied in main.cpp, not there.
    on_Lang_nameBox_currentIndexChanged(ui->Lang_nameBox->currentIndex());
#ifdef Q_OS_ANDROID
    on_Android_fullScreenBox_clicked();
    on_Android_forceLandscapeBox_clicked();
    // Android_dockBox only affect the config file
#else
    on_Opacity_Box_valueChanged(ui->Opacity_Box->value());
#endif
    on_General_touchScrollBox_clicked();
    on_Theme_setButton_clicked();
    on_Data_recordDataBox_clicked();
    on_Data_mergeTimestampBox_clicked();
    on_Data_mergeTimestampIntervalBox_valueChanged(ui->Data_mergeTimestampIntervalBox->value());
    on_General_simultaneousClearBox_clicked();

    if(fontValid)
        on_Font_setButton_clicked();
    if(dataFontValid)
        on_DataFont_setButton_clicked();
    ui->Conf_currPathLabel->setText(m_settings->fileName());

}

#ifdef Q_OS_ANDROID

void SettingsTab::on_Android_forceLandscapeBox_clicked()
{
    jint mode;
    if(ui->Android_forceLandscapeBox->isChecked())
        mode = QAndroidJniObject::getStaticField<jint>("android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_USER_LANDSCAPE");
    else
        mode = QAndroidJniObject::getStaticField<jint>("android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_UNSPECIFIED");
    QtAndroid::androidActivity().callMethod<void>("setRequestedOrientation", "(I)V", mode);
}

void SettingsTab::on_Android_HWSerialBox_clicked()
{
    // the receiver of updateAvailableDeviceTypes() will read the settings
    // emit the signal after the setting is stored
    m_settings->beginGroup("SerialTest_Connect");
    m_settings->setValue("Android_HWSerial", ui->Android_HWSerialBox->isChecked());
    m_settings->endGroup();
    emit updateAvailableDeviceTypes();
}

#endif


void SettingsTab::on_Lang_nameBox_currentIndexChanged(int index)
{
    ui->Lang_filePathEdit->setHidden(index != ui->Lang_nameBox->count() - 1);
}


void SettingsTab::on_Conf_setMaxHistoryButton_clicked()
{
    m_settings->beginGroup("SerialTest");
    m_settings->setValue("History_MaxCount", ui->Conf_maxHistoryBox->value());
    m_settings->endGroup();
}


void SettingsTab::on_Conf_clearHistoryButton_clicked()
{
    QMessageBox::StandardButton btn;
    btn = QMessageBox::warning(this, tr("Warning"), tr("All history will be deleted!\nContinue?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if(btn == QMessageBox::No)
        return;
    for(const auto& name : DeviceTab::m_historyPrefix)
        m_settings->remove(name);
}


void SettingsTab::on_Lang_setButton_clicked()
{
    m_settings->beginGroup("SerialTest");
    m_settings->setValue("Lang_Name", ui->Lang_nameBox->currentData().toString());
    m_settings->setValue("Lang_Path", ui->Lang_filePathEdit->text());
    m_settings->endGroup();
}


void SettingsTab::on_Conf_importButton_clicked()
{
    QMessageBox::StandardButton btn;
    btn = QMessageBox::warning(this, tr("Warning"), tr("This app will be closed after import!\nContinue?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if(btn == QMessageBox::No)
        return;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Import config from file"), QString(), tr("Config files") + " (*.ini *.conf);;" + tr("All files") + " (*.*)");
    if(fileName.isEmpty())
        return;
    QSettings newSettings(fileName, QSettings::IniFormat);
    newSettings.setIniCodec("UTF-8");

    if(!newSettings.childGroups().contains("SerialTest"))
    {
        QMessageBox::warning(this, tr("Error"), tr("Unsupported file format."));
        return;
    }
    m_settings->clear();
    for(const auto& key : newSettings.allKeys())
        m_settings->setValue(key, newSettings.value(key));
    m_settings->sync();

    QMessageBox::information(this, tr("Info"), tr("Imported."));
    QApplication::closeAllWindows();
}

void SettingsTab::on_Conf_exportButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export config to file"), QString(), tr("Config files") + " (*.ini *.conf);;" + tr("All files") + " (*.*)");
    if(fileName.isEmpty())
        return;
    createConfFile(fileName, true);
}


void SettingsTab::on_Theme_setButton_clicked()
{
    QString themeName = ui->Theme_nameBox->currentData().toString();
    m_settings->beginGroup("SerialTest");
    m_settings->setValue("Theme_Name", themeName);
    m_settings->endGroup();
    emit themeChanged(themeName);
}


void SettingsTab::on_Data_recordDataBox_clicked()
{
    emit recordDataChanged(ui->Data_recordDataBox->isChecked());
}


void SettingsTab::on_Data_mergeTimestampBox_clicked()
{
    emit mergeTimestampChanged(ui->Data_mergeTimestampBox->isChecked());
}


void SettingsTab::on_Data_mergeTimestampIntervalBox_valueChanged(int arg1)
{
    emit timestampIntervalChanged(arg1);
}


void SettingsTab::on_General_simultaneousClearBox_clicked()
{
    bool clearBoth = ui->General_simultaneousClearBox->isChecked();
    m_settings->beginGroup("SerialTest");
    m_settings->setValue("ClearBothRxDataAndGraph", clearBoth);
    m_settings->endGroup();
    emit clearBehaviorChanged(clearBoth);
}


void SettingsTab::setTouchScroll(bool enabled)
{
    if(enabled)
    {
        QScroller::grabGesture(ui->scrollArea);
    }
    else
    {
        QScroller::ungrabGesture(ui->scrollArea);
    }
}
