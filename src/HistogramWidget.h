#pragma once
#include <QWidget>
#include <QImage>
#include <cstdint>

/// Fast 256-bin intensity histogram.
/// Subsamples 1-in-16 pixels for performance.
/// Stays dark (black background) regardless of app theme — correct for data display.
class HistogramWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget *parent = nullptr);

    void setLogScale(bool on);

public slots:
    void updateFromImage(const QImage &img);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    uint32_t m_hist[256] = {};
    bool     m_hasData   = false;
    bool     m_log       = false;
};
