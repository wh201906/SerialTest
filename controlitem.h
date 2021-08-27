#ifndef CONTROLITEM_H
#define CONTROLITEM_H

#include <QWidget>

namespace Ui
{
class ControlItem;
}

class ControlItem : public QWidget
{
    Q_OBJECT

public:
    enum Type
    {
        Command,
        Slider,
        CheckBox,
        SpinBox,
    };

    explicit ControlItem(Type type, QWidget *parent = nullptr);
    ~ControlItem();

    bool load(QString& data);
    QString save();
private slots:
    void on_slider_valueChanged(int value);

    void on_spinBoxUpButton_clicked();

    void on_spinBoxDownButton_clicked();

    void on_minEdit_editingFinished();

    void on_maxEdit_editingFinished();

    void on_stepEdit_editingFinished();

    void on_confButton_clicked();

    void on_prefixBox_stateChanged(int arg1);

    void on_suffixBox_stateChanged(int arg1);

    void on_deleteButton_clicked();

    void on_prefixTypeBox_currentIndexChanged(int index);

    void on_suffixTypeBox_currentIndexChanged(int index);

    void on_autoBox_stateChanged(int arg1);

    void on_sendButton_clicked();

    void on_sliderEdit_editingFinished();

    void on_hexBox_stateChanged(int arg1);

private:
    Ui::ControlItem *ui;
    Type type;

    void initUI();
    const QString dataSplitter = "||,,||";
signals:
    void send(QByteArray&);

};

#endif // CONTROLITEM_H
