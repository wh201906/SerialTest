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
        // The mouseMoveEvent() of QCPAxis will not test the iRangeDrag flag, so I need some extra code in MyCustomPlot::eventFilter()
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
                QPointF pinchDirection = pinchGesture->centerPoint() - pinchGesture->hotSpot();
                qreal pinchTan = (pinchDirection.x() == 0) ? 572958 : qAbs(pinchDirection.y() / pinchDirection.x());
                Qt::Orientations pinchOrientations;
                if(pinchTan < 2.74) // ~70 degree
                    pinchOrientations |= Qt::Horizontal;
                if(pinchTan > 0.364) // ~20 degree
                    pinchOrientations |= Qt::Vertical;
                Qt::Orientations rangeZoom = rect->rangeZoom();
                if(rangeZoom.testFlag(Qt::Horizontal) && pinchOrientations.testFlag(Qt::Horizontal))
                {
                    foreach(QCPAxis* it, rect->rangeZoomAxes(Qt::Horizontal))
                    {
                        m_axisScaleCenterList.insert(it, it->pixelToCoord(pinchCenter.x()));
                    }
                }
                if(rangeZoom.testFlag(Qt::Vertical) && pinchOrientations.testFlag(Qt::Vertical))
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

void MyCustomPlot::setDarkStyle(bool enabled)
{
    QPen tmpPen[3];
    QColor tmpColor;
    QBrush tmpBrush[2];

    QLinearGradient plotGradient;
    QLinearGradient axisRectGradient;
    if(enabled)
    {
        // from https://www.qcustomplot.com/index.php/demos/styleddemo
        plotGradient.setStart(0, 0);
        plotGradient.setFinalStop(0, 350);
        plotGradient.setColorAt(0, QColor(80, 80, 80));
        plotGradient.setColorAt(1, QColor(50, 50, 50));
        axisRectGradient.setStart(0, 0);
        axisRectGradient.setFinalStop(0, 350);
        axisRectGradient.setColorAt(0, QColor(80, 80, 80));
        axisRectGradient.setColorAt(1, QColor(30, 30, 30));

        tmpPen[0] = QPen(Qt::white, 1);
        tmpPen[1] = QPen(QColor(140, 140, 140), 1, Qt::DotLine);
        tmpPen[2] = Qt::NoPen;
        tmpColor = Qt::white;
        tmpBrush[0] = plotGradient;
        tmpBrush[1] = axisRectGradient;
    }
    else
    {
        // from qcustomplot.cpp(2.1.1)
        tmpPen[0] = QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap);
        tmpPen[1] = QPen(QColor(200, 200, 200), 0, Qt::DotLine);
        tmpPen[2] = QPen(QColor(200, 200, 200), 0, Qt::SolidLine);
        tmpColor = Qt::black;
        tmpBrush[0] = QBrush(Qt::white, Qt::SolidPattern);
        tmpBrush[1] = Qt::NoBrush;
    }
    xAxis->setBasePen(tmpPen[0]);
    yAxis->setBasePen(tmpPen[0]);
    xAxis->setTickPen(tmpPen[0]);
    yAxis->setTickPen(tmpPen[0]);
    xAxis->setSubTickPen(tmpPen[0]);
    yAxis->setSubTickPen(tmpPen[0]);
    xAxis->setTickLabelColor(tmpColor);
    yAxis->setTickLabelColor(tmpColor);
    xAxis->grid()->setPen(tmpPen[1]);
    yAxis->grid()->setPen(tmpPen[1]);
//    xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
//    yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
//    xAxis->grid()->setSubGridVisible(true);
//    yAxis->grid()->setSubGridVisible(true);
    xAxis->grid()->setZeroLinePen(tmpPen[2]);
    yAxis->grid()->setZeroLinePen(tmpPen[2]);
    setBackground(tmpBrush[0]);
    axisRect()->setBackground(tmpBrush[1]);

    xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
}
