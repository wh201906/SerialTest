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
        if(!interactions().testFlag(QCP::iRangeZoom))
            return QWidget::event(event);
        QPinchGesture* pinchGesture = static_cast<QPinchGesture *>(gesture);
        if(!HandlePinchGesture(pinchGesture))
            return QWidget::event(event);
        event->accept();
        return true;
    }
    return QWidget::event(event);
}

bool MyCustomPlot::HandlePinchGesture(QPinchGesture* pinchGesture)
{
    if(pinchGesture->state() != Qt::GestureFinished)
        return false;

    QPointF pos =  mapFromGlobal(pinchGesture->hotSpot().toPoint());
    double scaleFactor = pinchGesture->totalScaleFactor();
    // forward event to layerable under cursor:
    foreach(QCPLayerable *candidate, layerableListAt(pos, false))
    {
        QCPAxis* axis = qobject_cast<QCPAxis*>(candidate);
        if(axis && axis->axisRect()->rangeZoom().testFlag(axis->orientation()) && axis->axisRect()->rangeZoomAxes(axis->orientation()).contains(axis))
        {
            double factor = (axis->axisRect()->rangeZoomFactor(axis->orientation()) * 1.0 / scaleFactor);
            axis->scaleRange(factor, axis->pixelToCoord(axis->orientation() == Qt::Horizontal ? pos.x() : pos.y()));
            replot();
            break;
        }

        QCPAxisRect* rect = qobject_cast<QCPAxisRect*>(candidate);
        Qt::Orientations rangeZoom;
        if(rect && (rangeZoom = rect->rangeZoom()) != 0)
        {
            double factorH, factorV;
            factorH = rect->rangeZoomFactor(Qt::Horizontal) / scaleFactor;
            factorV = rect->rangeZoomFactor(Qt::Vertical) / scaleFactor;
            if(rangeZoom.testFlag(Qt::Horizontal))
            {
                foreach(QPointer<QCPAxis> axis, rect->rangeZoomAxes(Qt::Horizontal))
                {
                    if(!axis.isNull())
                        axis->scaleRange(factorH, axis->pixelToCoord(pos.x()));
                }
            }
            if(rangeZoom.testFlag(Qt::Vertical))
            {
                foreach(QPointer<QCPAxis> axis, rect->rangeZoomAxes(Qt::Vertical))
                {
                    if(!axis.isNull())
                        axis->scaleRange(factorV, axis->pixelToCoord(pos.y()));
                }
            }
            replot();
            break;
        }
    }
    return true;
}
