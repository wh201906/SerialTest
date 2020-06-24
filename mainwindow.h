#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QList>
#include <QPushButton>

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

private:
    Ui::MainWindow *ui;
    QSerialPort* port;
    QSerialPortInfo* info;
};
#endif // MAINWINDOW_H
