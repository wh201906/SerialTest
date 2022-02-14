#ifndef CTRLTAB_H
#define CTRLTAB_H

#include <QWidget>
#include <QTextCodec>
#include <QRegularExpression>

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

public slots:
    void setDataCodec(QTextCodec *codec);
private slots:
    void onCtrlItemDestroyed();
    void on_ctrl_clearButton_clicked();
    void on_ctrl_importButton_clicked();
    void on_ctrl_exportButton_clicked();
    void addCtrlItem();
private:
    Ui::CtrlTab *ui;

    int ctrlItemCount = 0;
    QTextCodec* dataCodec = nullptr;
    QRegularExpression* commentRegExp = nullptr;
signals:
    void send(const QByteArray& data);
    void newDataCodec(QTextCodec *codec);
};

#endif // CTRLTAB_H
