#include "mycustomplot.h"

#include <QGesture>
#include <QGestureEvent>
#include <QGestureRecognizer>

MyCustomPlot::MyCustomPlot(QWidget* parent) : QCustomPlot(parent)
{
    grabGesture(Qt::PinchGesture);
}

bool MyCustomPlot::event(QEvent *event)
{
    if(event->type() == QEvent::Gesture)
    {
        QGestureEvent* gEvent = static_cast<QGestureEvent*>(event);
        QGesture* gesture = gEvent->gesture(Qt::PinchGesture);

        if(!gesture)
            return QWidget::event(event);

        QPinchGesture* pinchGesture = static_cast<QPinchGesture *>(gesture);
        if(pinchGesture->state() != Qt::GestureFinished)
            return QWidget::event(event);

        qDebug() << pinchGesture->scaleFactor() << pinchGesture->totalScaleFactor() << pinchGesture->lastScaleFactor();
        QPointF pos = pinchGesture->centerPoint();
        // forward event to layerable under cursor:
        foreach(QCPLayerable *candidate, layerableListAt(pos, false))
        {
            qDebug() << candidate;
            event->accept(); // default impl of QCPLayerable's mouse events ignore the event, in that case propagate to next candidate in list
            QCPAxis* axis = qobject_cast<QCPAxis*>(candidate);
            if(axis)
            {
                const double factor = qPow(axis->axisRect()->rangeZoomFactor(axis->orientation()), -pinchGesture->totalScaleFactor());
                axis->scaleRange(factor, axis->pixelToCoord(axis->orientation() == Qt::Horizontal ? pos.x() : pos.y()));
                break;
            }
        }
        event->accept();
        return true;

    }
    return QWidget::event(event);
}

void MyCustomPlot::HandlePinchGesture(QPinchGesture* pinchGesture)
{
    qDebug() << pinchGesture;
}
