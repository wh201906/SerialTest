#include "plottab.h"
#include "ui_plottab.h"

#include "legenditemdialog.h"

PlotTab::PlotTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlotTab)
{
    ui->setupUi(this);

    doubleRegex = new QRegularExpression("-?\\d*\\.?\\d+"); // for +xxxxx and xxxxx. , just get xxxxx
    doubleRegex->optimize();
    on_plot_advancedBox_stateChanged(Qt::Unchecked); // hide
}

PlotTab::~PlotTab()
{
    delete ui;
}

void PlotTab::initSettings()
{
    settings = MySettings::defaultSettings();
    loadPreference();

    connect(ui->plot_enaBox, &QCheckBox::clicked, this, &PlotTab::savePlotPreference);
    connect(ui->plot_latestBox, &QCheckBox::clicked, this, &PlotTab::savePlotPreference);
    connect(ui->plot_XTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotTab::savePlotPreference);
    // savePlotPreference() must be called after plot_dataNumBox is changed, so the signal is not connected there.
    connect(ui->plot_legendCheckBox, &QCheckBox::clicked, this, &PlotTab::savePlotPreference);
    connect(ui->plot_tracerCheckBox, &QCheckBox::clicked, this, &PlotTab::savePlotPreference);
    connect(ui->plot_frameSpTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotTab::savePlotPreference);
    connect(ui->plot_frameSpEdit, &QLineEdit::editingFinished, this, &PlotTab::savePlotPreference);
    connect(ui->plot_dataSpTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotTab::savePlotPreference);
    connect(ui->plot_dataSpEdit, &QLineEdit::editingFinished, this, &PlotTab::savePlotPreference);
    connect(ui->plot_clearFlagTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotTab::savePlotPreference);
    connect(ui->plot_clearFlagEdit, &QLineEdit::editingFinished, this, &PlotTab::savePlotPreference);
    connect(ui->plot_scatterBox, &QCheckBox::clicked, this, &PlotTab::savePlotPreference);

}

void PlotTab::setReplotInterval(int msec)
{
    m_dataProcessTimer->setInterval(msec);
}

void PlotTab::initQCP()
{
    // init
    plotBuf = new QString();
    plotTracer = new QCPItemTracer(ui->qcpWidget);
    plotText = new QCPItemText(ui->qcpWidget);
    m_dataProcessTimer = new QTimer();
    plotDefaultTicker = ui->qcpWidget->xAxis->ticker();
    plotTime = QTime::currentTime();
    plotCounter = 0;
    plotXAxisWidth = ui->qcpWidget->xAxis->range().size();
    plotTimeTicker->setTimeFormat("%h:%m:%s.%z");
    plotTimeTicker->setTickCount(5);
    connect(m_dataProcessTimer, &QTimer::timeout, this, &PlotTab::processData);
    m_dataProcessTimer->setInterval(20);
    m_dataProcessTimer->start();


    // appearance
    ui->qcpWidget->axisRect()->setupFullAxesBox(true);
    ui->qcpWidget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectLegend | QCP::iSelectPlottables);
    ui->qcpWidget->legend->setSelectableParts(QCPLegend::spItems);
    plotTracer->setStyle(QCPItemTracer::tsCrosshair);
    plotTracer->setBrush(Qt::red);
    plotTracer->setInterpolating(false);
    plotTracer->setVisible(false);
    if(ui->qcpWidget->graphCount() > plotSelectedId)
        plotTracer->setGraph(ui->qcpWidget->graph(plotSelectedId));
    plotText->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
    plotText->setTextAlignment(Qt::AlignLeft);
    plotText->position->setType(QCPItemPosition::ptAxisRectRatio);
    plotText->position->setCoords(0.01, 0.01);
    plotText->setPen(QPen(Qt::black));
    plotText->setPadding(QMargins(2, 2, 2, 2));
    plotText->setVisible(false);
    plotSelectedName = "";
    ui->qcpWidget->replot();

    // connect
    connect(ui->qcpWidget, &QCustomPlot::selectionChangedByUser, this, &PlotTab::onQCPSelectionChanged);
    connect(ui->qcpWidget->xAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged), this, &PlotTab::onXAxisChangedByUser);
    connect(ui->qcpWidget, &QCustomPlot::legendDoubleClick, this, &PlotTab::onQCPLegendDoubleClick);
    connect(ui->qcpWidget, &QCustomPlot::legendClick, this, &PlotTab::onQCPLegendClick);
    connect(ui->qcpWidget, &QCustomPlot::axisDoubleClick, this, &PlotTab::onQCPAxisDoubleClick);
    connect(ui->qcpWidget, &QCustomPlot::mousePress, this, &PlotTab::onQCPMousePress);
    connect(ui->qcpWidget, &QCustomPlot::mouseRelease, this, &PlotTab::onQCPMouseRelease);
}

void PlotTab::onQCPLegendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent* event)
{
    Q_UNUSED(legend)
    if((event->button() & Qt::LeftButton) == 0) // double click with left button
        return;
    if(item == nullptr)  // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
        return;

    // hide/show a graph
    QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
    QCPAbstractPlottable* pl = plItem->plottable();
    if(pl->visible())
    {
        pl->setName("*" + pl->name());
        pl->setVisible(false);
    }
    else
    {
        QString oldName = pl->name();
        if(oldName[0] == '*')
            pl->setName(oldName.right(oldName.size() - 1));
        pl->setVisible(true);
    }
    ui->qcpWidget->replot();
}

void PlotTab::onQCPLegendClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent* event)
{
    // Rename a graph by right clicking on its legend item(or long press on Android)
    Q_UNUSED(legend)
    if((event->button() & Qt::RightButton) == 0) // right click
        return;
    if(item == nullptr)
        return;
    setGraphProperty(item);
}

void PlotTab::onQCPAxisDoubleClick(QCPAxis *axis)
{
    if(axis == ui->qcpWidget->xAxis)
        on_plot_fitXButton_clicked();
    else if(axis == ui->qcpWidget->yAxis)
        on_plot_fitYButton_clicked();
}

void PlotTab::onQCPMousePress(QMouseEvent *event)
{
    QCPAbstractLegendItem* item = getLegendItemByPos(event->pos());
    if(item == nullptr)
        return;
    longPressCounter[item] = event->timestamp();
}

void PlotTab::onQCPMouseRelease(QMouseEvent *event)
{
    QCPAbstractLegendItem* item = getLegendItemByPos(event->pos());
    if(item == nullptr)
        return;
    ulong pressTimestamp = longPressCounter[item]; // default value is 0
    ulong releaseTimestamp = event->timestamp();
    // timeout=700 is used in QTapAndHoldGesture, so this threshold should be fine
    if(pressTimestamp != 0 && releaseTimestamp > pressTimestamp && releaseTimestamp - pressTimestamp > 700)
    {
        qDebug() << "long pressed!";
        setGraphProperty(item);
    }
    longPressCounter[item] = 0; // avoid multi click
}

void PlotTab::onQCPMouseMoved(QMouseEvent *event)
{
    if(ui->plot_tracerCheckBox->isChecked())
    {
        double x = ui->qcpWidget->xAxis->pixelToCoord(event->pos().x());
        updateTracer(x);
    }
}

void PlotTab::on_plot_dataNumBox_valueChanged(int arg1)
{
    changeGraphNum(arg1);
    longPressCounter.clear();
    savePlotPreference();
}

void PlotTab::changeGraphNum(int newNum)
{
    if(ui->plot_XTypeBox->currentIndex() == 1) // use first data as X
        newNum--;
    int delta = newNum - ui->qcpWidget->graphCount();
    QCPGraph* currGraph;
    if(delta > 0)
    {
        for(int i = 0; i < delta; i++)
        {
            QRandomGenerator* randGen = QRandomGenerator::global();
            currGraph = ui->qcpWidget->addGraph();
            currGraph->setPen(QColor(randGen->bounded(10, 235), randGen->bounded(10, 235), randGen->bounded(10, 235)));
            currGraph->setSelectable(QCP::stWhole);
        }
    }
    else if(delta < 0)
    {
        delta = -delta;
        for(int i = 0; i < delta; i++)
            ui->qcpWidget->removeGraph(ui->qcpWidget->graphCount() - 1);
    }
}

void PlotTab::on_plot_clearButton_clicked()
{
    int num;
    plotCounter = 0;
    plotTime = QTime::currentTime();
    num = ui->qcpWidget->graphCount();
    for(int i = 0; i < num; i++)
        ui->qcpWidget->graph(i)->data()->clear(); // use data()->clear() rather than data().clear()
    plotBuf->clear();
    ui->qcpWidget->replot();
}

void PlotTab::on_plot_legendCheckBox_stateChanged(int arg1)
{
    ui->qcpWidget->legend->setVisible(arg1 == Qt::Checked);
    ui->qcpWidget->replot();
}

void PlotTab::on_plot_advancedBox_stateChanged(int arg1)
{
    ui->plot_advancedWidget->setVisible(arg1 == Qt::Checked);
}

void PlotTab::updateTracer(double x)
{
    plotTracer->setGraphKey(x);
    plotTracer->updatePosition();

    double xValue = plotTracer->position->key();
    double yValue = plotTracer->position->value();
    plotText->setText((plotSelectedName + "\n%2, %3").arg(xValue).arg(yValue));
    ui->qcpWidget->replot();
}

void PlotTab::on_plot_tracerCheckBox_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked)
    {
        connect(ui->qcpWidget, &QCustomPlot::mouseMove, this, &PlotTab::onQCPMouseMoved);
        plotTracer->setVisible(true);
        plotText->setVisible(true);
    }
    else
    {
        disconnect(ui->qcpWidget, &QCustomPlot::mouseMove, this, &PlotTab::onQCPMouseMoved);
        plotTracer->setVisible(false);
        plotText->setVisible(false);
    }
    ui->qcpWidget->replot();
}

void PlotTab::on_plot_fitXButton_clicked()
{
    ui->qcpWidget->xAxis->rescale(true);
    ui->qcpWidget->replot();
}

void PlotTab::on_plot_fitYButton_clicked()
{
    ui->qcpWidget->yAxis->rescale(true);
    ui->qcpWidget->replot();
}

void PlotTab::onQCPSelectionChanged()
{
    // Copied from official interaction demo
    // A legendItem and a plottable cannot be both selected by user.
    // synchronize selection of graphs with selection of corresponding legend items:
    QCPGraph *graph;
    for(int i = 0; i < ui->qcpWidget->graphCount(); ++i)
    {
        graph = ui->qcpWidget->graph(i);
        QCPPlottableLegendItem *item = ui->qcpWidget->legend->itemWithPlottable(graph);
        if(item->selected() || graph->selected())
        {
            plotSelectedId = i;
            item->setSelected(true);
            graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
        }
    }
    graph = ui->qcpWidget->graph(plotSelectedId);
    plotTracer->setGraph(graph);
    plotSelectedName = ui->qcpWidget->legend->itemWithPlottable(graph)->plottable()->name();
}

void PlotTab::on_plot_frameSpTypeBox_currentIndexChanged(int index)
{
    ui->plot_frameSpEdit->setVisible(index != 2 && index != 3);
    if(index == 0)
    {
        if(ui->plot_frameSpEdit->text().isEmpty()) // empty separator is not allowed
            ui->plot_frameSpEdit->setText(plotFrameSeparator);
        else
            plotFrameSeparator = ui->plot_frameSpEdit->text();
    }
    else if(index == 1) // empty separator is not allowed
    {
        QByteArray newSp = QByteArray::fromHex(ui->plot_frameSpEdit->text().toLatin1());
        if(ui->plot_frameSpEdit->text() == "") // empty separator is not allowed
            ui->plot_frameSpEdit->setText(newSp.toHex(' '));
        else
            plotFrameSeparator = newSp;
    }
    else if(index == 2)
        plotFrameSeparator = "\r\n";
    else if(index == 3)
        plotFrameSeparator = "\n";
}

void PlotTab::on_plot_dataSpTypeBox_currentIndexChanged(int index)
{
    ui->plot_dataSpEdit->setVisible(index != 2 && index != 3);
    if(index == 0)
    {
        if(ui->plot_dataSpEdit->text().isEmpty()) // empty separator is not allowed
            ui->plot_dataSpEdit->setText(plotDataSeparator);
        else
            plotDataSeparator = ui->plot_dataSpEdit->text();
    }
    else if(index == 1)
    {
        QByteArray newSp = QByteArray::fromHex(ui->plot_dataSpEdit->text().toLatin1());
        if(ui->plot_dataSpEdit->text() == "") // empty separator is not allowed
            ui->plot_dataSpEdit->setText(newSp.toHex(' '));
        else
            plotDataSeparator = newSp;
    }
    else if(index == 2)
        plotDataSeparator = "\r\n";
    else if(index == 3)
        plotDataSeparator = "\n";
}


void PlotTab::on_plot_clearFlagTypeBox_currentIndexChanged(int index)
{
    ui->plot_clearFlagEdit->setVisible(index != 0);
    if(index == 0)
        plotClearFlag.clear();
    if(index == 1)
        plotClearFlag = ui->plot_clearFlagEdit->text();
    else if(index == 2)
        plotClearFlag = QByteArray::fromHex(ui->plot_clearFlagEdit->text().toLatin1());
}


void PlotTab::on_plot_clearFlagEdit_editingFinished()
{
    on_plot_clearFlagTypeBox_currentIndexChanged(ui->plot_clearFlagTypeBox->currentIndex());
}

void PlotTab::on_plot_frameSpEdit_editingFinished()
{
    on_plot_frameSpTypeBox_currentIndexChanged(ui->plot_frameSpTypeBox->currentIndex());
}


void PlotTab::on_plot_dataSpEdit_editingFinished()
{
    on_plot_dataSpTypeBox_currentIndexChanged(ui->plot_dataSpTypeBox->currentIndex());
}

void PlotTab::on_plot_XTypeBox_currentIndexChanged(int index)
{
    if(index == 1 && ui->plot_dataNumBox->value() == 1) // use first data as X axis
    {
        ui->plot_dataNumBox->setValue(2);
        ui->plot_dataNumBox->setMinimum(2);
        // no need to call on_plot_dataNumBox_valueChanged();
    }
    else
    {
        ui->plot_dataNumBox->setMinimum(1);
    }
    if(index == 2)
    {
        ui->qcpWidget->xAxis->setTicker(plotTimeTicker);
    }
    else
    {
        ui->qcpWidget->xAxis->setTicker(plotDefaultTicker);
    }
}

void PlotTab::on_plot_scatterBox_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked)
    {
        for(int i = 0; i < ui->qcpWidget->graphCount(); i++)
            ui->qcpWidget->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle));
    }
    else
    {
        for(int i = 0; i < ui->qcpWidget->graphCount(); i++)
            ui->qcpWidget->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));
    }
}

void PlotTab::onXAxisChangedByUser(const QCPRange &newRange)
{
    plotXAxisWidth = newRange.size();
}

void PlotTab::saveGraphProperty()
{
    if(settings->group() != "")
        return;
    QStringList nameList, colorList;
    settings->beginGroup("SerialTest_Plot");

    // don't use ui->plot_dataNumBox->value() there, the actural num of graphs might be value() or value()-1
    for(int i = 0; i < ui->qcpWidget->graphCount(); i++)
    {
        QCPAbstractPlottable* g = ui->qcpWidget->graph(i);
        QString gName = g->name();
        if(!g->visible() && gName[0] == '*')
            nameList << gName.right(gName.size() - 1);
        else
            nameList << gName;
        colorList << g->pen().color().name();
    }
    settings->setValue("GraphName", nameList);
    settings->setValue("GraphColor", colorList);
    settings->endGroup();
}

void PlotTab::savePlotPreference()
{
    if(settings->group() != "")
        return;
    settings->beginGroup("SerialTest_Plot");
    settings->setValue("Enabled", ui->plot_enaBox->isChecked());
    settings->setValue("Latest", ui->plot_latestBox->isChecked());
    settings->setValue("XType", ui->plot_XTypeBox->currentIndex());
    settings->setValue("DataNum", ui->plot_dataNumBox->value());
    settings->setValue("Legend", ui->plot_legendCheckBox->isChecked());
    settings->setValue("Tracer", ui->plot_tracerCheckBox->isChecked());
    settings->setValue("FrameSp_Type", ui->plot_frameSpTypeBox->currentIndex());
    settings->setValue("FrameSp_Context", ui->plot_frameSpEdit->text());
    settings->setValue("DataSp_Type", ui->plot_dataSpTypeBox->currentIndex());
    settings->setValue("DataSp_Context", ui->plot_dataSpEdit->text());
    settings->setValue("ClearF_Type", ui->plot_clearFlagTypeBox->currentIndex());
    settings->setValue("ClearF_Context", ui->plot_clearFlagEdit->text());
    settings->setValue("Scatter", ui->plot_scatterBox->isChecked());
    settings->endGroup();
}

void PlotTab::loadPreference()
{
    // default preferences are defined in this function
    const QString defaultFrameSp = "|";
    const QString defaultDataSp = ",";
    const QStringList darkThemeList = {"qdss_dark"};
    bool isDarkTheme = false;
    QStringList nameList, colorList;
    int nameNum, colorNum;

    settings->beginGroup("SerialTest_Plot");
    // empty separator is not allowed
    if(settings->value("FrameSp_Context").toString().isEmpty())
        settings->setValue("FrameSp_Context", defaultFrameSp);
    if(settings->value("DataSp_Context").toString().isEmpty())
        settings->setValue("DataSp_Context", defaultDataSp);


    ui->plot_enaBox->setChecked(settings->value("Enabled", false).toBool());
    ui->plot_latestBox->setChecked(settings->value("Latest", false).toBool());
    ui->plot_XTypeBox->setCurrentIndex(settings->value("XType", 0).toInt());
    ui->plot_dataNumBox->setValue(settings->value("DataNum", 1).toInt());
    ui->plot_legendCheckBox->setChecked(settings->value("Legend", false).toBool());
    ui->plot_tracerCheckBox->setChecked(settings->value("Tracer", false).toBool());
    ui->plot_frameSpTypeBox->setCurrentIndex(settings->value("FrameSp_Type", 3).toInt());
    ui->plot_frameSpEdit->setText(settings->value("FrameSp_Context", defaultFrameSp).toString());
    ui->plot_dataSpTypeBox->setCurrentIndex(settings->value("DataSp_Type", 0).toInt());
    ui->plot_dataSpEdit->setText(settings->value("DataSp_Context", defaultDataSp).toString());
    ui->plot_clearFlagTypeBox->setCurrentIndex(settings->value("ClearF_Type", 1).toInt());
    ui->plot_clearFlagEdit->setText(settings->value("ClearF_Context", "cls").toString());
    ui->plot_scatterBox->setChecked(settings->value("Scatter", false).toBool());
    colorList = settings->value("GraphColor", QStringList()).toStringList();
    nameList = settings->value("GraphName", QStringList()).toStringList();
    settings->endGroup();
    settings->beginGroup("SerialTest");
    isDarkTheme = darkThemeList.contains(settings->value("Theme_Name").toString());
    settings->endGroup();
    changeGraphNum(ui->plot_dataNumBox->value());
    on_plot_frameSpTypeBox_currentIndexChanged(ui->plot_frameSpTypeBox->currentIndex());
    on_plot_dataSpTypeBox_currentIndexChanged(ui->plot_dataSpTypeBox->currentIndex());
    on_plot_clearFlagTypeBox_currentIndexChanged(ui->plot_clearFlagTypeBox->currentIndex());
    // don't use ui->plot_dataNumBox->value() there, the actural num of graphs might be value() or value()-1
    nameNum = ui->qcpWidget->graphCount();
    colorNum = nameNum < colorList.size() ? nameNum : colorList.size();
    nameNum = nameNum < nameList.size() ? nameNum : nameList.size();
    for(int i = 0; i < colorNum; i++)
        ui->qcpWidget->graph(i)->setPen(QColor(colorList[i]));
    for(int i = 0; i < nameNum; i++)
        ui->qcpWidget->graph(i)->setName(nameList[i]);
    if(isDarkTheme)
        ui->qcpWidget->setDarkStyle();
}

bool PlotTab::enabled()
{
    return ui->plot_enaBox->isChecked();
}

void PlotTab::newData(const QByteArray& data)
{
    plotBuf->append(decoder->toUnicode(data));
}

void PlotTab::processData()
{
    double currKey = 0;
    bool hasData = false;
    int i;
    QStringList dataList;
    if(plotBuf->isEmpty())
        return;

    while((i = plotBuf->indexOf(plotFrameSeparator)) != -1)
    {
        hasData = true;
        dataList = ((QString)(plotBuf->left(i))).split(plotDataSeparator);
        // qDebug() << dataList;
        plotBuf->remove(0, i + plotFrameSeparator.length());
        plotCounter++;
        if(!plotClearFlag.isEmpty() && dataList[0] == plotClearFlag)
        {
            on_plot_clearButton_clicked();
        }
        else if(ui->plot_XTypeBox->currentIndex() == 0)
        {
            currKey = plotCounter;
            for(i = 0; i < ui->plot_dataNumBox->value() && i < dataList.length(); i++)
                ui->qcpWidget->graph(i)->addData(currKey, toDouble(dataList[i]));
        }
        else if(ui->plot_XTypeBox->currentIndex() == 1)
        {
            currKey = toDouble(dataList[0]);
            for(i = 1; i < ui->plot_dataNumBox->value() && i < dataList.length(); i++)
                ui->qcpWidget->graph(i - 1)->addData(currKey, toDouble(dataList[i]));
        }
        else if(ui->plot_XTypeBox->currentIndex() == 2)
        {
            currKey = plotTime.msecsTo(QTime::currentTime()) / 1000.0;
            for(i = 0; i < ui->plot_dataNumBox->value() && i < dataList.length(); i++)
                ui->qcpWidget->graph(i)->addData(currKey, toDouble(dataList[i]));
        }
        QApplication::processEvents();

    }
    if(!hasData)
    {
        if(plotBuf->size() > 1024 * 1024 * 256) // 256MB threshold
        {
            qDebug() << "plotBuf full!";
            plotBuf->clear();
        }
        return;
    }
    else if(ui->plot_latestBox->isChecked())
    {
        ui->qcpWidget->xAxis->blockSignals(true);
        ui->qcpWidget->xAxis->setRange(currKey, plotXAxisWidth, Qt::AlignRight);
        ui->qcpWidget->xAxis->blockSignals(false);
        if(ui->plot_tracerCheckBox->isChecked())
            updateTracer(currKey);
    }
    ui->qcpWidget->replot(QCustomPlot::rpQueuedReplot);
}

void PlotTab::setDecoder(QTextDecoder *decoder)
{
    if(this->decoder != nullptr)
        delete this->decoder;
    this->decoder = decoder;
}

QCPAbstractLegendItem* PlotTab::getLegendItemByPos(const QPointF &pos)
{
    int i;
    if(!ui->qcpWidget->legend->visible()) // a bug in QCustomPlot v2.10, visibility needs to be checked
        return nullptr;
    if(ui->qcpWidget->legend->selectTest(pos, false) == -1.0)
        return nullptr;
    for(i = 0; i < ui->qcpWidget->legend->itemCount(); i++)
    {
        QCPAbstractLegendItem* it = ui->qcpWidget->legend->item(i);
        if(it->selectTest(pos, false) != -1.0)
        {
            return it;
        }
    }
    return nullptr;
}

void PlotTab::setGraphProperty(QCPAbstractLegendItem *item)
{
    QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
    QCPAbstractPlottable* pl = plItem->plottable();
    QString oldName = pl->name();
    oldName = (!pl->visible() && oldName[0] == '*') ? oldName.right(oldName.size() - 1) : oldName;
    LegendItemDialog dialog(oldName, pl->pen().color());
    if(dialog.exec() == QDialog::Accepted)
    {
        pl->setName((pl->visible() ? "" : "*") + dialog.getName());
        pl->setPen(dialog.getColor());
        ui->qcpWidget->replot();
        saveGraphProperty();
    }
}

inline double PlotTab::toDouble(const QString& str)
{
    return doubleRegex->match(str).captured().toDouble();
}
