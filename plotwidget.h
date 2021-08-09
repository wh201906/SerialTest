#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>

class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);
    bool view = false;

protected:
    virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

signals:

};

#endif // PLOTWIDGET_H
