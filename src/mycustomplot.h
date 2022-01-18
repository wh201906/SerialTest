#ifndef MYCUSTOMPLOT_H
#define MYCUSTOMPLOT_H

#include "qcustomplot.h"

class MyCustomPlot : public QCustomPlot
{
    Q_OBJECT
public:
    MyCustomPlot(QWidget *parent = nullptr);
protected:
    bool event(QEvent *event);
private:
    bool handlePinchGesture(QPinchGesture *pinchGesture);
};

#endif // MYCUSTOMPLOT_H
