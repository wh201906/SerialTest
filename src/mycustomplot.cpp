#include "mycustomplot.h"

#include <QGesture>
#include <QGestureEvent>
#include <QGestureRecognizer>

MyCustomPlot::MyCustomPlot(QWidget* parent) : QCustomPlot(parent)
{
    grabGesture(Qt::PinchGesture);
    installEventFilter(this);
}

bool MyCustomPlot::event(QEvent *event)
{
    if(event->type() == QEvent::Gesture)
    {
        QGestureEvent* gEvent = static_cast<QGestureEvent*>(event);
        QGesture* gesture = gEvent->gesture(Qt::PinchGesture);
        if(!gesture)
            return QCustomPlot::event(event);
        if(!interactions().testFlag(QCP::iRangeZoom))
            return QCustomPlot::event(event);
        QPinchGesture* pinchGesture = static_cast<QPinchGesture *>(gesture);
        if(!handlePinchGesture(pinchGesture))
            return QCustomPlot::event(event);
        event->accept();
        return true;
    }
    return QCustomPlot::event(event);
}

bool MyCustomPlot::eventFilter(QObject *watched, QEvent *event)
{
    // eventFilter doesn't work on the mouseEvent of layerables
    // so this eventFilter should be installed on the QCustomPlot object,
    // and treat "mMouseEventLayerable" as "watched"
    if(event->type() == QEvent::MouseMove)
    {
        QCPAxis* axis = qobject_cast<QCPAxis*>(mMouseEventLayerable);
        if(axis)
        {
            // if iRangeDrag is not set, return true(filter it out)
            return !interactions().testFlag(QCP::iRangeDrag);
        }
    }
    return QCustomPlot::eventFilter(watched, event);
}

bool MyCustomPlot::handlePinchGesture(QPinchGesture* pinchGesture)
{
    auto state = pinchGesture->state();
    // record the scale center
    if(state == Qt::GestureStarted)
    {
        // disable the drag interaction
        // The drag interaction will affect the scaleRange() there.
        // The mouseMoveEvent() of QCPAxisRect will test the iRangeDrag flag.
        // The mouseMoveEvent() of QCPAxisRect will not test the iRangeDrag flag, so I need some extra code in MyCustomPlot::eventFilter()
        m_iRangeDragEnabled = interactions().testFlag(QCP::iRangeDrag);
        setInteraction(QCP::iRangeDrag, false);

        QPointF pinchCenter = mapFromGlobal(pinchGesture->centerPoint().toPoint());
        foreach(QCPLayerable* candidate, layerableListAt(pinchCenter, false))
        {
            QCPAxis* axis = qobject_cast<QCPAxis*>(candidate);
            QCPAxisRect* rect = qobject_cast<QCPAxisRect*>(candidate);
            // pinch on axis, detect it first
            if(axis && axis->axisRect()->rangeZoom().testFlag(axis->orientation()) && axis->axisRect()->rangeZoomAxes(axis->orientation()).contains(axis))
            {
                qDebug() << "on axis";
                m_axisScaleCenterList.insert(axis, axis->pixelToCoord(axis->orientation() == Qt::Horizontal ? pinchCenter.x() : pinchCenter.y()));
                break; // necessary, since the QCPAxisRcet contains the QCPAxis
            }
            // pinch on rect
            else if(rect)
            {
                qDebug() << "on rect";
                Qt::Orientations rangeZoom = rect->rangeZoom();
                if(rangeZoom.testFlag(Qt::Horizontal))
                {
                    foreach(QCPAxis* it, rect->rangeZoomAxes(Qt::Horizontal))
                    {
                        m_axisScaleCenterList.insert(it, it->pixelToCoord(pinchCenter.x()));
                    }
                }
                if(rangeZoom.testFlag(Qt::Vertical))
                {
                    foreach(QCPAxis* it, rect->rangeZoomAxes(Qt::Vertical))
                    {
                        m_axisScaleCenterList.insert(it, it->pixelToCoord(pinchCenter.y()));
                    }
                }
                break;
            }
        }
    }
    // scale
    else if(state == Qt::GestureUpdated)
    {
        for(auto it = m_axisScaleCenterList.begin(); it != m_axisScaleCenterList.end(); ++it)
        {
            it.key()->scaleRange(1.0 / pinchGesture->scaleFactor(), it.value());
        }
        replot(rpQueuedReplot);
    }
    else if(state == Qt::GestureFinished)
    {
        setInteraction(QCP::iRangeDrag, m_iRangeDragEnabled);
        m_axisScaleCenterList.clear();
        replot();
    }
    return true;
}
