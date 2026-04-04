#pragma once

#include <QWidget>
#include <QVector>
#include <QPair>

/// Scrolling line graph of SSM seeing values.
/// Line is green when below threshold, red when above.
/// Color changes in real time when threshold is adjusted.
/// Orange dashed line shows the trigger threshold.
class SeePlot : public QWidget
{
public:
    explicit SeePlot(QWidget *parent = nullptr);

    void addPoint(double timestamp, double seeing);
    void setThreshold(double arcsec);
    void setYMax(double arcsec);
    void setMaxPoints(int n);
    void setTimeRange(int seconds);  // 0 = show all

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPair<double, double>> m_data;
    double m_threshold     = 2.0;
    double m_yMax          = 5.0;
    int    m_maxPoints     = 3000;
    int    m_timeRangeSecs = 0;
};
