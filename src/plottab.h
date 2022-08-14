#ifndef PLOTTAB_H
#define PLOTTAB_H

#include <QWidget>

#include "mysettings.h"
#include "mycustomplot.h"

namespace Ui
{
class PlotTab;
}

class PlotTab : public QWidget
{
    Q_OBJECT

public:
    explicit PlotTab(QWidget *parent = nullptr);
    ~PlotTab();

    void initQCP();
    void initSettings();
    void setReplotInterval(int msec);
    bool enabled();
public slots:
    void newData(const QByteArray &data);
    void setDecoder(QTextDecoder* decoder);
signals:

private slots:
    void onQCPLegendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent* event);
    void onQCPLegendClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event);
    void onQCPAxisDoubleClick(QCPAxis *axis);
    void onQCPMousePress(QMouseEvent *event);
    void onQCPMouseRelease(QMouseEvent *event);
    void onQCPMouseMoved(QMouseEvent *event);
    void onQCPSelectionChanged();
    void onXAxisChangedByUser(const QCPRange &newRange);
    void on_plot_tracerCheckBox_stateChanged(int arg1);
    void on_plot_fitXButton_clicked();
    void on_plot_fitYButton_clicked();
    void on_plot_dataNumBox_valueChanged(int arg1);
    void on_plot_clearButton_clicked();
    void on_plot_legendCheckBox_stateChanged(int arg1);
    void on_plot_advancedBox_stateChanged(int arg1);
    void on_plot_frameSpTypeBox_currentIndexChanged(int index);
    void on_plot_dataSpTypeBox_currentIndexChanged(int index);
    void on_plot_scatterBox_stateChanged(int arg1);
    void on_plot_frameSpEdit_editingFinished();
    void on_plot_dataSpEdit_editingFinished();
    void on_plot_clearFlagTypeBox_currentIndexChanged(int index);
    void on_plot_clearFlagEdit_editingFinished();
    void on_plot_XTypeBox_currentIndexChanged(int index);
    void savePlotPreference();
    void loadPreference();
    void processData();
private:
    Ui::PlotTab *ui;

    QString* plotBuf;
    quint64 plotCounter;
    QCPItemTracer* plotTracer;
    QCPItemText* plotText;
    int plotSelectedId = 0;
    QString plotSelectedName;
    QString plotFrameSeparator;
    QString plotDataSeparator;
    QString plotClearFlag;
    double plotXAxisWidth;
    QSharedPointer<QCPAxisTickerTime> plotTimeTicker = QSharedPointer<QCPAxisTickerTime>(new QCPAxisTickerTime);
    QSharedPointer<QCPAxisTicker> plotDefaultTicker;
    QTime plotTime;

    QMap<QCPAbstractLegendItem*, ulong> longPressCounter;

    QTextDecoder* decoder = nullptr;
    MySettings *settings;
    QRegularExpression* doubleRegex;

    QTimer* m_dataProcessTimer;

    void updateTracer(double x);
    QCPAbstractLegendItem *getLegendItemByPos(const QPointF &pos);
    void setGraphProperty(QCPAbstractLegendItem *item);
    inline double toDouble(const QString& str); // find valid value then convert
    void saveGraphProperty();
    void changeGraphNum(int newNum);
};

#endif // PLOTTAB_H
