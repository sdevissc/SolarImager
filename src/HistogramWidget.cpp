#include "HistogramWidget.h"

#include <QPainter>
#include <QPen>
#include <cmath>
#include <cstdint>
#include <algorithm>

HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setStyleSheet("background:#1a1a1a;");
}

void HistogramWidget::setLogScale(bool on)
{
    m_log = on;
    update();
}

void HistogramWidget::updateFromImage(const QImage &img)
{
    if (img.isNull()) return;

    std::fill(std::begin(m_hist), std::end(m_hist), 0u);

    const int W = img.width();
    const int H = img.height();

    // Subsample every 4th row and 4th column (~16× fewer pixels, negligible visual difference)
    if (img.format() == QImage::Format_Grayscale8) {
        for (int y = 0; y < H; y += 4) {
            const uchar *row = img.constScanLine(y);
            for (int x = 0; x < W; x += 4)
                ++m_hist[row[x]];
        }
    } else if (img.format() == QImage::Format_Grayscale16) {
        // Downscale 16→8 for histogram display
        for (int y = 0; y < H; y += 4) {
            const uint16_t *row = reinterpret_cast<const uint16_t*>(img.constScanLine(y));
            for (int x = 0; x < W; x += 4)
                ++m_hist[row[x] >> 8];
        }
    } else {
        // RGB888 — use green channel as luminance proxy
        for (int y = 0; y < H; y += 4) {
            const uchar *row = img.constScanLine(y);
            for (int x = 0; x < W; x += 4)
                ++m_hist[row[x * 3 + 1]];
        }
    }

    m_hasData = true;
    update();
}

void HistogramWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(26, 26, 26));

    if (!m_hasData) {
        p.setPen(QColor(100, 100, 100));
        p.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }

    const int W = width(), H = height();
    const int PL=28, PR=6, PT=20, PB=18;
    const int pw = W - PL - PR;
    const int ph = H - PT - PB;

    // Build float histogram (optionally log-scaled)
    float fhist[256];
    if (m_log) {
        for (int i = 0; i < 256; ++i)
            fhist[i] = (m_hist[i] > 0) ? std::log1p(static_cast<float>(m_hist[i])) : 0.f;
    } else {
        for (int i = 0; i < 256; ++i)
            fhist[i] = static_cast<float>(m_hist[i]);
    }

    const float maxVal = *std::max_element(fhist, fhist + 256);
    if (maxVal <= 0.f) return;

    const float scale = ph / maxVal;
    const int   barW  = std::max(1, pw / 256);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(80, 160, 220));  // solid steel blue
    for (int i = 0; i < 256; ++i) {
        int bh = static_cast<int>(fhist[i] * scale);
        if (bh == 0) continue;
        int x = PL + i * pw / 256;
        p.drawRect(x, PT + ph - bh, barW, bh);
    }

    // Axes
    p.setPen(QPen(QColor(120, 120, 120), 1));
    p.drawLine(PL, PT, PL, PT + ph);
    p.drawLine(PL, PT + ph, PL + pw, PT + ph);

    // X labels
    p.setPen(QColor(160, 160, 160));
    QFont f = p.font(); f.setPointSize(7); p.setFont(f);
    for (int v : {0, 64, 128, 192, 255}) {
        int x = PL + v * pw / 255;
        p.drawText(x - 8, H - 2, QString::number(v));
    }

    // Saturated and zero pixel percentages
    uint64_t total = 0;
    for (int i = 0; i < 256; ++i) total += m_hist[i];
    if (total > 0) {
        QFont pf = p.font(); pf.setPointSize(8); pf.setBold(true); p.setFont(pf);

        // Zero pixels (bin 0) — cyan
        double zeroPct = 100.0 * m_hist[0] / total;
        p.setPen(QColor(0, 200, 220));
        p.drawText(PL + 2, PT - 3,
                   QString("blk: %1%").arg(zeroPct, 0, 'f', 1));

        // Saturated pixels (bin 255) — red/orange
        double satPct = 100.0 * m_hist[255] / total;
        QString satStr = QString("sat: %1%").arg(satPct, 0, 'f', 1);
        p.setPen(satPct > 0.1 ? QColor(255, 80, 80) : QColor(160, 160, 160));
        QFontMetrics fm(pf);
        int satW = fm.horizontalAdvance(satStr);
        p.drawText(PL + pw - satW - 2, PT - 3, satStr);
    }

    // Y label
    p.save();
    p.translate(10, PT + ph / 2);
    p.rotate(-90);
    p.drawText(-12, 4, m_log ? "log" : "lin");
    p.restore();
}
