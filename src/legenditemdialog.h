#ifndef LEGENDITEMDIALOG_H
#define LEGENDITEMDIALOG_H

#include <QDialog>
#include <QColorDialog>

namespace Ui
{
class LegendItemDialog;
}

class LegendItemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LegendItemDialog(QWidget *parent = nullptr);
    explicit LegendItemDialog(const QString& name, const QColor& color, QWidget *parent = nullptr);
    ~LegendItemDialog();

    QColor getColor();
    QString getName();
public slots:
    void setName(const QString &name);
    void setColor(const QColor &color);
private slots:
    void on_colorButton_clicked();

    void on_nameEdit_editingFinished();

private:
    Ui::LegendItemDialog *ui;
    QColorDialog *colorDialog;

    QColor m_color;
    QString m_name;
};

#endif // LEGENDITEMDIALOG_H
