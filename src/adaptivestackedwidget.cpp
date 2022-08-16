#include "adaptivestackedwidget.h"

#include <QDebug>

AdaptiveStackedWidget::AdaptiveStackedWidget(QWidget *parent) : QStackedWidget(parent)
{

}

int AdaptiveStackedWidget::addWidget(QWidget *widget)
{
    int result = QStackedWidget::addWidget(widget);
    setNewWidgetPolicy(widget);
    setCurrentWidgetPolicy();
    return result;
}

int AdaptiveStackedWidget::insertWidget(int index, QWidget *widget)
{
    int result = QStackedWidget::insertWidget(index, widget);
    setNewWidgetPolicy(widget);
    setCurrentWidgetPolicy();
    return result;
}

void AdaptiveStackedWidget::removeWidget(QWidget *widget)
{
    QStackedWidget::removeWidget(widget);
    m_horizontalPolicy.remove(widget);
    m_horizontalPolicy.remove(widget);
}

void AdaptiveStackedWidget::setCurrentIndex(int index)
{
    currentWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QStackedWidget::setCurrentIndex(index);
    setCurrentWidgetPolicy();
}

void AdaptiveStackedWidget::setCurrentWidget(QWidget *widget)
{
    currentWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QStackedWidget::setCurrentWidget(widget);
    setCurrentWidgetPolicy();
}

void AdaptiveStackedWidget::setCurrentWidgetPolicy()
{
    QWidget* widget = currentWidget();
    if(!widget)
        return;
    widget->setSizePolicy(m_horizontalPolicy[widget], m_verticalPolicy[widget]);
    widget->adjustSize();
    adjustSize();
}

void AdaptiveStackedWidget::setNewWidgetPolicy(QWidget *widget)
{
    if(!widget)
        return;
    m_horizontalPolicy[widget] = widget->sizePolicy().horizontalPolicy();
    m_verticalPolicy[widget] = widget->sizePolicy().verticalPolicy();
    widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}
