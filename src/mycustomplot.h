#ifndef MYCUSTOMPLOT_H
#define MYCUSTOMPLOT_H

#include "qcustomplot.h"

class MyCustomPlot : public QCustomPlot
{
    Q_OBJECT
public:
    explicit MyCustomPlot(QWidget *parent = nullptr);
    void setDarkStyle();
protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool handlePinchGesture(QPinchGesture *pinchGesture);


    QHash<QCPAxis*, double> m_axisScaleCenterList;
    bool m_iRangeDragEnabled;
};

#endif // MYCUSTOMPLOT_H
