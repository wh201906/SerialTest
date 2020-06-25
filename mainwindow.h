#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QList>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void refreshPortsInfo();
    void on_portTable_cellDoubleClicked(int row, int column);

    void on_advancedBox_clicked(bool checked);

    void on_openButton_clicked();

    void on_closeButton_clicked();

    void readData();
    void on_sendButton_clicked();


    void onErrorOccurred(QSerialPort::SerialPortError error);
private:
    Ui::MainWindow *ui;
    QSerialPort* port;
    QSerialPortInfo* info;
    void initUI();
    void stateUpdate();
    bool portState;
    QLabel* portLabel;
    QLabel* stateLabel;
    QLabel* baudRateLabel;
    QLabel* dataBitsLabel;
    QLabel* stopBitsLabel;
    QLabel* parityLabel;

};
#endif // MAINWINDOW_H
