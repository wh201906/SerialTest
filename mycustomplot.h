#ifndef MYCUSTOMPLOT_H
#define MYCUSTOMPLOT_H

#include "qcustomplot.h"

class MyCustomPlot : public QCustomPlot
{
    Q_OBJECT
    void HandlePinchGesture(QPinchGesture *pinchGesture);
public:
    MyCustomPlot(QWidget *parent = nullptr);
protected:
    bool event(QEvent *event);
};

#endif // MYCUSTOMPLOT_H
