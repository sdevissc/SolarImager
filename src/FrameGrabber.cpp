#include "FrameGrabber.h"
#include "CameraInterface.h"

#include <QElapsedTimer>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QDataStream>

// ══════════════════════════════════════════════════════════════════════════════
// SER format writer
// Spec: http://www.grischa-hahn.homepage.t-online.de/astro/ser/SER%20Doc%20V3b.pdf
// Header: 178 bytes, little-endian except timestamps (big-endian)
// Pixel data: frames concatenated, no padding
// Trailer: optional per-frame UTC timestamps (8 bytes each, Windows FILETIME)
// ══════════════════════════════════════════════════════════════════════════════

#pragma pack(push, 1)
struct SerHeader {
    char     fileID[14]   = {'L','U','C','A','M','-','R','E','C','O','R','D','E','R'};
    int32_t  luID         = 0;
    int32_t  colorID      = 0;
    int32_t  littleEndian = 0;
    int32_t  imageWidth   = 0;
    int32_t  imageHeight  = 0;
    int32_t  pixelDepth   = 8;
    int32_t  frameCount   = 0;
    char     observer[40] = {};
    char     instrument[40] = {};
    char     telescope[40] = {};
    int64_t  dateTime     = 0;
    int64_t  dateTimeUTC  = 0;
};
#pragma pack(pop)
static_assert(sizeof(SerHeader) == 178, "SER header must be 178 bytes");

// Windows FILETIME: 100-ns intervals since 1601-01-01
static int64_t toWindowsFileTime(const QDateTime &dt)
{
    // Seconds from 1601-01-01 to Unix epoch (1970-01-01)
    static constexpr int64_t EPOCH_DIFF = 11644473600LL;
    return (dt.toSecsSinceEpoch() + EPOCH_DIFF) * 10000000LL;
}

class SerWriter
{
public:
    SerWriter() = default;
    ~SerWriter() { close(); }

    bool open(const QString &path, int w, int h, int bitDepth, bool color)
    {
        m_file.setFileName(path);
        if (!m_file.open(QIODevice::WriteOnly)) {
            qWarning() << "[SerWriter] Cannot open file:" << path
                       << "error:" << m_file.errorString();
            return false;
        }
        m_w        = w;
        m_h        = h;
        m_bitDepth = bitDepth;
        m_color    = color;
        m_count    = 0;
        m_startUtc = QDateTime::currentDateTimeUtc();

        // Write placeholder header — will be rewritten on close()
        // with the actual frame count
        SerHeader hdr;
        hdr.imageWidth   = w;
        hdr.imageHeight  = h;
        hdr.pixelDepth   = bitDepth;
        hdr.colorID      = color ? 100 : 0;   // 100=RGB, 0=MONO
        hdr.littleEndian = 1;
        hdr.dateTime     = toWindowsFileTime(m_startUtc);
        hdr.dateTimeUTC  = hdr.dateTime;
        strncpy(hdr.instrument, "SolarImager", sizeof(hdr.instrument)-1);

        m_file.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        return true;
    }

    bool writeFrame(const QImage &img)
    {
        if (!m_file.isOpen()) return false;

        // Timestamp for trailer (Windows FILETIME, UTC)
        int64_t ts = toWindowsFileTime(QDateTime::currentDateTimeUtc());
        m_timestamps.append(ts);

        if (img.format() == QImage::Format_Grayscale8) {
            for (int y = 0; y < img.height(); ++y)
                m_file.write(reinterpret_cast<const char*>(img.constScanLine(y)),
                             img.width());
        } else if (img.format() == QImage::Format_Grayscale16) {
            for (int y = 0; y < img.height(); ++y)
                m_file.write(reinterpret_cast<const char*>(img.constScanLine(y)),
                             img.width() * 2);
        } else if (img.format() == QImage::Format_RGB888) {
            for (int y = 0; y < img.height(); ++y)
                m_file.write(reinterpret_cast<const char*>(img.constScanLine(y)),
                             img.width() * 3);
        } else {
            // Convert to Grayscale8 as fallback
            QImage conv = img.convertToFormat(QImage::Format_Grayscale8);
            for (int y = 0; y < conv.height(); ++y)
                m_file.write(reinterpret_cast<const char*>(conv.constScanLine(y)),
                             conv.width());
        }
        ++m_count;
        return true;
    }

    void close()
    {
        if (!m_file.isOpen()) return;

        // Write timestamp trailer
        for (int64_t ts : m_timestamps)
            m_file.write(reinterpret_cast<const char*>(&ts), sizeof(ts));

        // Rewrite header with correct frame count
        m_file.seek(0);
        SerHeader hdr;
        hdr.imageWidth   = m_w;
        hdr.imageHeight  = m_h;
        hdr.pixelDepth   = m_bitDepth;
        hdr.colorID      = m_color ? 100 : 0;
        hdr.littleEndian = 1;
        hdr.frameCount   = m_count;
        hdr.dateTime     = toWindowsFileTime(m_startUtc);
        hdr.dateTimeUTC  = hdr.dateTime;
        strncpy(hdr.instrument, "SolarImager", sizeof(hdr.instrument)-1);
        m_file.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

        m_file.close();
        qInfo() << "[SerWriter] Wrote" << m_count << "frames to" << m_file.fileName();
    }

    int frameCount() const { return m_count; }
    bool isOpen()    const { return m_file.isOpen(); }

private:
    QFile           m_file;
    int             m_w        = 0;
    int             m_h        = 0;
    int             m_bitDepth = 8;
    bool            m_color    = false;
    int             m_count    = 0;
    QDateTime       m_startUtc;
    QVector<int64_t> m_timestamps;
};

// ══════════════════════════════════════════════════════════════════════════════
// FrameGrabber
// ══════════════════════════════════════════════════════════════════════════════

FrameGrabber::FrameGrabber(CameraInterface *camera, QObject *parent)
    : QThread(parent), m_camera(camera)
{}

void FrameGrabber::stop()
{
    QMutexLocker lock(&m_mutex);
    m_stop = true;
}

void FrameGrabber::startRecording(const QString &filepath, int maxFrames, int bitDepth)
{
    QMutexLocker lock(&m_mutex);
    m_recRequest = RecRequest{filepath, maxFrames, bitDepth};
}

void FrameGrabber::stopRecording()
{
    QMutexLocker lock(&m_mutex);
    m_recording = false;
}

bool FrameGrabber::isRecording() const
{
    QMutexLocker lock(&m_mutex);
    return m_recording;
}

void FrameGrabber::requestRoi(int x, int y, int w, int h)
{
    QMutexLocker lock(&m_mutex);
    m_pendingRoiCmd = RoiCmd::Set;
    m_roiX=x; m_roiY=y; m_roiW=w; m_roiH=h;
}

void FrameGrabber::requestClearRoi()
{
    QMutexLocker lock(&m_mutex);
    m_pendingRoiCmd = RoiCmd::Clear;
}

// ── run ───────────────────────────────────────────────────────────────────────

void FrameGrabber::run()
{
    if (m_camera) m_camera->startGrabbing();

    QElapsedTimer timer;
    timer.start();

    SerWriter ser;

    while (true) {
        {
            QMutexLocker lock(&m_mutex);
            if (m_stop) break;
        }

        processPendingRoi();

        QImage frame = m_camera->grabFrame();
        if (frame.isNull()) {
            QMutexLocker lock(&m_mutex);
            if (m_stop) break;
            lock.unlock();
            msleep(10);
            continue;
        }

        // ── Preview throttle ─────────────────────────────────────────────
        bool recActive = false;
        { QMutexLocker lock(&m_mutex); recActive = m_recording; }
        // Adaptive display fps: large frames (>2MP) are throttled more
        // to avoid saturating the UI thread with expensive LUT/scale ops.
        // Recording path is throttled further to leave CPU headroom for disk I/O.
        const long pixels = static_cast<long>(frame.width()) * frame.height();
        int maxDisplayFps;
        if (recActive) {
            maxDisplayFps = (pixels > 2'000'000) ? 3 : 5;
        } else {
            if      (pixels > 6'000'000) maxDisplayFps = 10;
            else if (pixels > 2'000'000) maxDisplayFps = 15;
            else                         maxDisplayFps = 25;
        }
        const qint64 intervalNs = 1'000'000'000LL / maxDisplayFps;
        qint64 nowNs = timer.nsecsElapsed();
        if (nowNs - m_lastPreviewNs >= intervalNs) {
            emit frameReady(frame);
            m_lastPreviewNs = nowNs;
        }

        // ── Recording ────────────────────────────────────────────────────
        {
            QMutexLocker lock(&m_mutex);

            // Start a new recording if requested
            if (m_recRequest.has_value()) {
                // Close any existing recording first
                if (ser.isOpen()) {
                    lock.unlock();
                    ser.close();
                    lock.relock();
                }
                const auto &req = m_recRequest.value();
                m_recMaxFrames  = req.maxFrames;
                m_recFrameCount = 0;
                m_recording     = true;
                // Copy values before reset() destroys the RecRequest
                QString recPath    = req.path;
                int     recBitDepth = req.bitDepth;
                m_recRequest.reset();

                // Open SER file — detect color from frame format
                bool color = (frame.format() == QImage::Format_RGB888);
                int  depth = (frame.format() == QImage::Format_Grayscale16) ? 16 : recBitDepth;
                lock.unlock();
                bool ok = ser.open(recPath, frame.width(), frame.height(),
                                   depth, color);
                lock.relock();
                if (!ok) {
                    m_recording = false;
                    qWarning() << "[FrameGrabber] Failed to open SER file:" << recPath;
                } else {
                    qInfo() << "[FrameGrabber] Recording started ->"
                            << recPath << "max:" << m_recMaxFrames;
                }
            }

            // Write frame if recording
            if (m_recording) {
                lock.unlock();
                ser.writeFrame(frame);
                lock.relock();

                ++m_recFrameCount;
                emit recordingStats(m_fpsAvg, m_mbpsAvg, m_recFrameCount);

                if (m_recFrameCount >= m_recMaxFrames || !m_recording) {
                    m_recording = false;
                    lock.unlock();
                    ser.close();
                    lock.relock();
                    emit recordingFinished();
                }
            }

            // stopRecording() was called externally
            if (!m_recording && ser.isOpen()) {
                lock.unlock();
                ser.close();
                lock.relock();
                emit recordingFinished();
            }
        }

        updateStats(frame);
    }

    // Close any open recording on exit
    if (ser.isOpen())
        ser.close();

    if (m_camera) m_camera->stopGrabbing();
}

// ── processPendingRoi ─────────────────────────────────────────────────────────

void FrameGrabber::processPendingRoi()
{
    RoiCmd cmd;
    int x, y, w, h;
    {
        QMutexLocker lock(&m_mutex);
        cmd = m_pendingRoiCmd;
        m_pendingRoiCmd = RoiCmd::None;
        x=m_roiX; y=m_roiY; w=m_roiW; h=m_roiH;
    }
    if (cmd == RoiCmd::Set && m_camera)
        m_camera->setRoi(x, y, w, h);
    else if (cmd == RoiCmd::Clear && m_camera)
        m_camera->clearRoi();
}

// ── updateStats ───────────────────────────────────────────────────────────────

void FrameGrabber::updateStats(const QImage &img)
{
    static QElapsedTimer t;
    static bool init = false;
    if (!init) { t.start(); init = true; }

    qint64 nowNs = t.nsecsElapsed();
    double dtSec = (nowNs - m_lastStatNs) / 1e9;
    if (m_lastStatNs == 0) { m_lastStatNs = nowNs; return; }

    double alpha = 0.1;
    double instFps  = (dtSec > 0) ? 1.0 / dtSec : 0.0;
    double instMbps = (img.sizeInBytes() / 1e6) / dtSec;

    if (m_fpsAvg < 0) {
        m_fpsAvg  = instFps;
        m_mbpsAvg = instMbps;
    } else {
        m_fpsAvg  = alpha * instFps  + (1.0 - alpha) * m_fpsAvg;
        m_mbpsAvg = alpha * instMbps + (1.0 - alpha) * m_mbpsAvg;
    }
    m_lastStatNs = nowNs;

    // Emit 4x/sec
    static qint64 lastEmitNs = 0;
    if (nowNs - lastEmitNs >= 250'000'000LL) {
        int done = 0;
        { QMutexLocker lock(&m_mutex); done = m_recFrameCount; }
        emit recordingStats(m_fpsAvg, m_mbpsAvg, done);
        lastEmitNs = nowNs;
    }
}
