#ifndef SERIALPINOUT_H
#define SERIALPINOUT_H

#include <QWidget>
#include <QSerialPort>

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

public slots:
    void setPinout(QSerialPort::PinoutSignals signal);
    void setEnableState(bool state);
private slots:
    void on_enaBox_clicked(bool checked);

private:
    Ui::SerialPinout *ui;
    const QString greenBG = "background-color: rgb(0,220,0);";
    const QString noBG = "";
    void onEnableStateChanged(bool state);
signals:
    void enableStateChanged(bool state);
};

#endif // SERIALPINOUT_H
