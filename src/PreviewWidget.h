#pragma once

#include <QLabel>
#include <QRect>
#include <QPoint>

/// Drop-in replacement for the preview QLabel that adds rubber-band
/// ROI drag-selection. The user clicks and drags to draw a rectangle;
/// on release, roiSelected() is emitted with the ROI in **image pixels**
/// (accounting for the current scale factor).
class PreviewWidget : public QLabel
{
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = nullptr);

    /// Call this whenever a new pixmap is set so the widget knows the
    /// scale factor between displayed pixels and sensor pixels.
    void setImageSize(int w, int h);

    /// Clear the ROI overlay (called after clearRoi())
    void clearRoi();

signals:
    /// Emitted on mouse release with ROI in sensor pixel coordinates.
    void roiSelected(int x, int y, int w, int h);

protected:
    void mousePressEvent(QMouseEvent *e)   override;
    void mouseMoveEvent(QMouseEvent *e)    override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e)        override;

private:
    QPoint widgetToImage(const QPoint &p) const;

    bool   m_dragging   = false;
    QPoint m_dragStart;   ///< widget coordinates
    QPoint m_dragCurrent; ///< widget coordinates
    QRect  m_roiOverlay;  ///< last committed ROI in widget coords (for drawing)

    int    m_imageW = 0;
    int    m_imageH = 0;
};
