#pragma once

#include <QWidget>
#include <QVector>
#include <QPair>

/// Compact scrolling bar chart of SSM seeing values.
/// Bars are green when below threshold, red when above.
/// Yellow dashed line shows the trigger threshold.
class SeePlot : public QWidget
{
public:
    explicit SeePlot(QWidget *parent = nullptr);

    void addPoint(double timestamp, double seeing);
    void setThreshold(double arcsec);
    void setYMax(double arcsec);
    void setMaxPoints(int n);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPair<double, double>> m_data;   // (timestamp, seeing)
    double m_threshold = 2.0;
    double m_yMax      = 5.0;
    int    m_maxPoints = 300;
};
