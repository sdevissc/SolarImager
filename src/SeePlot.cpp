#include "SeePlot.h"

#include <QPainter>
#include <QPen>
#include <QDateTime>
#include <algorithm>
#include <cmath>

SeePlot::SeePlot(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setStyleSheet("background:#ebebeb;");
}

void SeePlot::addPoint(double timestamp, double seeing)
{
    if (m_data.size() >= m_maxPoints)
        m_data.removeFirst();
    m_data.append({timestamp, seeing});
    update();
}

void SeePlot::setThreshold(double arcsec)
{
    m_threshold = arcsec;
    update();
}

void SeePlot::setYMax(double arcsec)
{
    m_yMax = std::max(arcsec, 0.5);
    update();
}

void SeePlot::setMaxPoints(int n)
{
    m_maxPoints = std::max(n, 10);
    while (m_data.size() > m_maxPoints)
        m_data.removeFirst();
    update();
}

void SeePlot::setTimeRange(int seconds)
{
    m_timeRangeSecs = seconds;
    update();
}

void SeePlot::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(235, 235, 235));

    const int W = width(), H = height();
    const int PL = 36, PR = 6, PT = 8, PB = 22;
    const int pw = W - PL - PR;
    const int ph = H - PT - PB;

    if (pw <= 0 || ph <= 0) return;

    if (m_data.isEmpty()) {
        p.setPen(QColor(120, 120, 120));
        p.drawText(rect(), Qt::AlignCenter, "Waiting for SSM data…");
        return;
    }

    // Apply time range filter
    const double now = m_data.last().first;
    QVector<QPair<double,double>> visible;
    if (m_timeRangeSecs > 0) {
        const double cutoff = now - m_timeRangeSecs;
        for (const auto &pt : m_data)
            if (pt.first >= cutoff)
                visible.append(pt);
    } else {
        visible = m_data;
    }
    if (visible.isEmpty()) visible = m_data.mid(m_data.size() - 1);

    // Y → pixel
    auto yPx = [&](double val) -> int {
        double clamped = std::min(val, m_yMax);
        return PT + ph - static_cast<int>(clamped / m_yMax * ph);
    };

    // X → pixel
    const double tFirst = visible.first().first;
    const double tLast  = visible.last().first;
    const double tSpan  = std::max(tLast - tFirst, 1.0);
    auto xPx = [&](double ts) -> int {
        return PL + static_cast<int>((ts - tFirst) / tSpan * pw);
    };

    // Grid lines
    p.setPen(QPen(QColor(190, 190, 190), 1, Qt::DashLine));
    for (int i = 0; i <= 5; ++i)
        p.drawLine(PL, yPx(m_yMax * i / 5.0), PL + pw, yPx(m_yMax * i / 5.0));

    // Threshold line
    p.setPen(QPen(QColor(220, 140, 0), 1, Qt::DashLine));
    p.drawLine(PL, yPx(m_threshold), PL + pw, yPx(m_threshold));

    // Color-coded line graph: green below threshold, red above
    // Draw segment by segment so color changes at threshold crossings
    const int n = visible.size();
    if (n >= 2) {
        for (int i = 1; i < n; ++i) {
            double v0 = visible[i-1].second;
            double v1 = visible[i].second;
            int x0 = xPx(visible[i-1].first);
            int y0 = yPx(v0);
            int x1 = xPx(visible[i].first);
            int y1 = yPx(v1);

            // If segment crosses threshold, split it
            bool a0above = v0 > m_threshold;
            bool a1above = v1 > m_threshold;

            if (a0above == a1above) {
                // No crossing — draw full segment in one color
                p.setPen(QPen(a0above ? QColor(200, 50, 50) : QColor(50, 160, 80), 1));
                p.drawLine(x0, y0, x1, y1);
            } else {
                // Crossing — find intersection point
                double tCross = (m_threshold - v0) / (v1 - v0);
                int xMid = x0 + static_cast<int>(tCross * (x1 - x0));
                int yMid = yPx(m_threshold);

                p.setPen(QPen(a0above ? QColor(200, 50, 50) : QColor(50, 160, 80), 1));
                p.drawLine(x0, y0, xMid, yMid);
                p.setPen(QPen(a1above ? QColor(200, 50, 50) : QColor(50, 160, 80), 1));
                p.drawLine(xMid, yMid, x1, y1);
            }
        }
    } else if (n == 1) {
        bool above = visible[0].second > m_threshold;
        p.setPen(QPen(above ? QColor(200, 50, 50) : QColor(50, 160, 80), 3));
        p.drawPoint(PL + pw / 2, yPx(visible[0].second));
    }

    // Axes
    p.setPen(QPen(QColor(80, 80, 80), 1));
    p.drawLine(PL, PT, PL, PT + ph);
    p.drawLine(PL, PT + ph, PL + pw, PT + ph);

    // Y labels
    QFont f = p.font(); f.setPointSize(7); p.setFont(f);
    p.setPen(QColor(60, 60, 60));
    for (int i = 0; i <= 5; ++i) {
        double yv = m_yMax * i / 5.0;
        p.drawText(2, yPx(yv) + 4, QString("%1\"").arg(yv, 0, 'f', 1));
    }

    // X time labels
    if (n >= 2) {
        auto fmtTime = [](double ts) {
            return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts))
                       .toString("HH:mm:ss");
        };
        p.drawText(PL,           H - 4, fmtTime(visible.first().first));
        p.drawText(PL + pw - 50, H - 4, fmtTime(visible.last().first));
    }
}
