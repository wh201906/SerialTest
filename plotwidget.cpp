#include "plotwidget.h"
#include <QPainter>
#include <QDebug>

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void PlotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.setPen(255);
    if(view)
        painter.setViewport(900 - width(), 900 - height(), width(), height());
    painter.drawEllipse(0, 0, 900, 900);
    qDebug() << "painted";
}
