#ifndef SETTINGSTAB_H
#define SETTINGSTAB_H

#include <QWidget>

#include "mysettings.h"

namespace Ui
{
class SettingsTab;
}

class SettingsTab : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsTab(QWidget *parent = nullptr);
    ~SettingsTab();

    void initSettings();
private slots:
    void on_Opacity_Box_valueChanged(int arg1);

    void on_Font_setButton_clicked();

    void on_Conf_clearButton_clicked();

    void on_Conf_createInCWDButton_clicked();

    void on_Conf_createInConfDirButton_clicked();

    void on_DataFont_setButton_clicked();

    void on_Android_fullScreenBox_clicked();
    void savePreference();
    void loadPreference();
#ifdef Q_OS_ANDROID
    void on_Android_forceLandscapeBox_clicked();
#endif

    void on_Lang_nameBox_currentIndexChanged(int index);

    void on_Conf_setMaxHistoryButton_clicked();

    void on_Conf_clearHistoryButton_clicked();

    void on_Lang_setButton_clicked();

    void on_Conf_importButton_clicked();

    void on_Conf_exportButton_clicked();

    void on_Theme_setButton_clicked();

private:
    Ui::SettingsTab *ui;
    MySettings* m_settings;
    void createConfFile(const QString &path, bool overwrite = false);
signals:
    void opacityChanged(qreal value);
    void fontChanged(QFont font);
    void fullScreenStateChanged(bool isFullScreen);
};

#endif // SETTINGSTAB_H
