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
    setStyleSheet("background:#1c1c1c;");
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

void SeePlot::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), QColor(28, 28, 28));

    const int W = width(), H = height();
    const int PL = 36, PR = 6, PT = 8, PB = 22;
    const int pw = W - PL - PR;
    const int ph = H - PT - PB;

    if (pw <= 0 || ph <= 0) return;

    // Waiting message
    if (m_data.isEmpty()) {
        p.setPen(QColor(120, 120, 120));
        p.drawText(rect(), Qt::AlignCenter, "Waiting for SSM data…");
        return;
    }

    // Y → pixel helper
    auto yPx = [&](double val) -> int {
        double clamped = std::min(val, m_yMax);
        return PT + ph - static_cast<int>(clamped / m_yMax * ph);
    };

    // Grid lines
    p.setPen(QPen(QColor(45, 45, 45), 1, Qt::DashLine));
    for (int i = 0; i <= 5; ++i) {
        int yp = yPx(m_yMax * i / 5.0);
        p.drawLine(PL, yp, PL + pw, yp);
    }

    // Threshold line
    p.setPen(QPen(QColor(255, 200, 0), 1, Qt::DashLine));
    p.drawLine(PL, yPx(m_threshold), PL + pw, yPx(m_threshold));

    // Bars — two passes (good / bad) to minimise setBrush calls
    const int n = m_data.size();
    const int barW = std::max(1, pw / std::max(n, 1));

    for (int pass = 0; pass < 2; ++pass) {
        p.setPen(Qt::NoPen);
        p.setBrush(pass == 0 ? QColor(46, 180, 95) : QColor(210, 65, 50));
        for (int i = 0; i < n; ++i) {
            double val = m_data[i].second;
            bool good  = (val <= m_threshold);
            if ((pass == 0) != good) continue;
            int bh = std::max(1, static_cast<int>(
                std::min(val, m_yMax) / m_yMax * ph));
            int x  = PL + (n > 1 ? i * pw / (n - 1) : pw / 2);
            p.drawRect(x, PT + ph - bh, barW, bh);
        }
    }

    // Axes
    p.setPen(QPen(QColor(110, 110, 110), 1));
    p.drawLine(PL, PT, PL, PT + ph);
    p.drawLine(PL, PT + ph, PL + pw, PT + ph);

    // Y labels
    QFont f = p.font();
    f.setPointSize(7);
    p.setFont(f);
    p.setPen(QColor(150, 150, 150));
    for (int i = 0; i <= 5; ++i) {
        double yv = m_yMax * i / 5.0;
        p.drawText(2, yPx(yv) + 4, QString("%1\"").arg(yv, 0, 'f', 1));
    }

    // X time labels (start / end)
    if (n >= 2) {
        auto fmtTime = [](double ts) {
            return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts))
                       .toString("HH:mm:ss");
        };
        p.drawText(PL,      H - 4, fmtTime(m_data.first().first));
        p.drawText(PL + pw - 50, H - 4, fmtTime(m_data.last().first));
    }
}
