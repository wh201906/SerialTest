#ifndef ADAPTIVESTACKEDWIDGET_H
#define ADAPTIVESTACKEDWIDGET_H

#include <QStackedWidget>
#include <QMap>

class AdaptiveStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    explicit AdaptiveStackedWidget(QWidget *parent = nullptr);

    int addWidget(QWidget *widget);
    int insertWidget(int index, QWidget *widget);
    void removeWidget(QWidget *widget);
public slots:
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget *widget);

private:
    QMap<QWidget*, QSizePolicy::Policy> m_horizontalPolicy, m_verticalPolicy;

    void setCurrentWidgetPolicy();
    void setNewWidgetPolicy(QWidget *widget);
};

#endif // ADAPTIVESTACKEDWIDGET_H
