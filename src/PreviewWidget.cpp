#include "PreviewWidget.h"

#include <QPainter>
#include <QPen>
#include <QMouseEvent>
#include <algorithm>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QLabel(parent)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void PreviewWidget::setImageSize(int w, int h)
{
    m_imageW = w;
    m_imageH = h;
}

void PreviewWidget::clearRoi()
{
    m_roiOverlay = QRect();
    update();
}

// ── coordinate mapping ────────────────────────────────────────────────────────
// The pixmap is centred in the label (Qt::AlignCenter).
// We need to find the top-left corner of the rendered pixmap first.

QPoint PreviewWidget::widgetToImage(const QPoint &p) const
{
    if (m_imageW <= 0 || m_imageH <= 0 || pixmap(Qt::ReturnByValue).isNull())
        return p;

    const QPixmap &pm = pixmap(Qt::ReturnByValue);
    const int pmW = pm.width();
    const int pmH = pm.height();

    // Pixmap is centred — compute its top-left in widget coords
    const int offX = (width()  - pmW) / 2;
    const int offY = (height() - pmH) / 2;

    // Scale from displayed pixels to sensor pixels
    const double scaleX = static_cast<double>(m_imageW) / pmW;
    const double scaleY = static_cast<double>(m_imageH) / pmH;

    int ix = static_cast<int>((p.x() - offX) * scaleX);
    int iy = static_cast<int>((p.y() - offY) * scaleY);

    ix = std::clamp(ix, 0, m_imageW - 1);
    iy = std::clamp(iy, 0, m_imageH - 1);

    return {ix, iy};
}

// ── mouse events ──────────────────────────────────────────────────────────────

void PreviewWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_imageW > 0) {
        m_dragging    = true;
        m_dragStart   = e->pos();
        m_dragCurrent = e->pos();
        update();
    }
    QLabel::mousePressEvent(e);
}

void PreviewWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging) {
        m_dragCurrent = e->pos();
        update();
    }
    QLabel::mouseMoveEvent(e);
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;

        // Build the rectangle in widget coords
        QRect widgetRect = QRect(m_dragStart, m_dragCurrent).normalized();

        // Enforce a minimum size (avoid accidental single-pixel clicks)
        if (widgetRect.width() < 8 || widgetRect.height() < 8) {
            update();
            return;
        }

        // Convert corners to image coords
        QPoint topLeft     = widgetToImage(widgetRect.topLeft());
        QPoint bottomRight = widgetToImage(widgetRect.bottomRight());

        int x = topLeft.x();
        int y = topLeft.y();
        int w = bottomRight.x() - x;
        int h = bottomRight.y() - y;

        // Snap to multiples of 8 (required by most camera SDKs)
        w = std::max(64, (w / 8) * 8);
        h = std::max(64, (h / 8) * 8);
        w = std::min(w, m_imageW - x);
        h = std::min(h, m_imageH - y);

        m_roiOverlay = widgetRect;
        update();

        emit roiSelected(x, y, w, h);
    }
    QLabel::mouseReleaseEvent(e);
}

void PreviewWidget::setScrollOffset(int x, int y)
{
    m_scrollX = x;
    m_scrollY = y;
}

// ── paint ─────────────────────────────────────────────────────────────────────

void PreviewWidget::paintEvent(QPaintEvent *e)
{
    QPainter p(this);

    // Draw pixmap at scroll offset (for zoom > 100%)
    const QPixmap pix = this->pixmap(Qt::ReturnByValue);
    if (!pix.isNull()) {
        p.drawPixmap(m_scrollX, m_scrollY, pix);
    } else {
        // No pixmap — let QLabel draw its text
        QLabel::paintEvent(e);
    }

    // Draw the rubber band while dragging
    if (m_dragging) {
        QRect r = QRect(m_dragStart, m_dragCurrent).normalized();
        p.setPen(QPen(QColor(255, 200, 0), 1, Qt::DashLine));
        p.setBrush(QColor(255, 200, 0, 30));
        p.drawRect(r);
    }
}
