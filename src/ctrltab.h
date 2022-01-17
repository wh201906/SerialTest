#ifndef CTRLTAB_H
#define CTRLTAB_H

#include <QWidget>

namespace Ui
{
class CtrlTab;
}

class CtrlTab : public QWidget
{
    Q_OBJECT

public:
    explicit CtrlTab(QWidget *parent = nullptr);
    ~CtrlTab();

    void setDataCodec(QTextCodec* codec);

private slots:
    void on_ctrl_addCMDButton_clicked();
    void on_ctrl_addSliderButton_clicked();
    void on_ctrl_addCheckBoxButton_clicked();
    void on_ctrl_addSpinBoxButton_clicked();
    void onCtrlItemDestroyed();
    void on_ctrl_clearButton_clicked();
    void on_ctrl_importButton_clicked();
    void on_ctrl_exportButton_clicked();
private:
    Ui::CtrlTab *ui;

    int ctrlItemCount = 0;
    QTextCodec* dataCodec = nullptr;
signals:
    void send(const QByteArray& data);
};

#endif // CTRLTAB_H
