#ifndef SERIALPINOUT_H
#define SERIALPINOUT_H

#include <QWidget>
#include <QSerialPort>

#include "mysettings.h"

namespace Ui
{
class SerialPinout;
}

class SerialPinout : public QWidget
{
    Q_OBJECT

public:
    explicit SerialPinout(QWidget *parent = nullptr);
    ~SerialPinout();

    void initSettings();
public slots:
    void setPinout(QSerialPort::PinoutSignals signal);
    void setEnableState(bool state);
    bool getEnableState();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void on_enaBox_clicked(bool checked);
    void loadPreference();
    void savePreference();

private:
    Ui::SerialPinout *ui;
    const QString activeBGList[3] = {"background-color: rgb(0,220,0);", "background-color: rgb(220,0,0);", "background-color: rgb(0,0,220);"};
    int activeBGId = 0;
    const QString noBG = "";
    MySettings *m_settings;
    void onEnableStateChanged(bool state);
signals:
    void enableStateChanged(bool state);
};

#endif // SERIALPINOUT_H
