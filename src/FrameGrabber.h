#pragma once

#include <QThread>
#include <QImage>
#include <QMutex>
#include <optional>

class CameraInterface;

/// Runs the camera grab loop in a background thread.
/// Works with any CameraInterface implementation (Basler, ZWO, PlayerOne…).
/// Emits frameReady() at most PREVIEW_FPS times/sec for the UI.
/// Recording writes every frame regardless of the preview throttle.
class FrameGrabber : public QThread
{
    Q_OBJECT

public:
    static constexpr int PREVIEW_FPS           = 15;  ///< Display fps when idle
    static constexpr int PREVIEW_FPS_RECORDING =  5;  ///< Display fps when recording (save CPU for I/O)

    explicit FrameGrabber(CameraInterface *camera, QObject *parent = nullptr);

    void stop();
    void startRecording(const QString &filepath, int maxFrames, int bitDepth);
    void stopRecording();
    bool isRecording() const;
    void requestRoi(int x, int y, int w, int h);
    void requestClearRoi();

signals:
    void frameReady(QImage img);
    void recordingStats(double fps, double mbps, int framesDone);
    void recordingFinished();

protected:
    void run() override;

private:
    CameraInterface *m_camera;
    bool             m_stop = false;

    qint64  m_lastPreviewNs = 0;

    struct RecRequest { QString path; int maxFrames=0; int bitDepth=8; };
    mutable QMutex              m_mutex;
    std::optional<RecRequest>   m_recRequest;
    bool   m_recording      = false;
    int    m_recMaxFrames   = 0;
    int    m_recFrameCount  = 0;

    qint64 m_lastStatNs     = 0;
    double m_fpsAvg         = -1.0;
    double m_mbpsAvg        = 0.0;

    enum class RoiCmd { None, Set, Clear };
    RoiCmd m_pendingRoiCmd  = RoiCmd::None;
    int m_roiX=0, m_roiY=0, m_roiW=0, m_roiH=0;

    void processPendingRoi();
    void updateStats(const QImage &img);
};
