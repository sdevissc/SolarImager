#pragma once

#include <QWidget>
#include <QVector>
#include <QPair>

/// Compact scrolling line graph of SSM seeing values.
/// Black line on light gray background.
/// Orange dashed line shows the trigger threshold.
class SeePlot : public QWidget
{
public:
    explicit SeePlot(QWidget *parent = nullptr);

    void addPoint(double timestamp, double seeing);
    void setThreshold(double arcsec);
    void setYMax(double arcsec);
    void setMaxPoints(int n);
    /// Set the displayed time window in seconds (0 = show all data)
    void setTimeRange(int seconds);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPair<double, double>> m_data;   // (timestamp, seeing)
    double m_threshold    = 2.0;
    double m_yMax         = 5.0;
    int    m_maxPoints    = 3000;  // store up to 50 min at 1 Hz
    int    m_timeRangeSecs = 0;    // 0 = show all
};
