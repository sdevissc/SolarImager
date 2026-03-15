#include "ZwoCameraInterface.h"

#include <QDebug>
#include <cstring>

// ZWO SDK header — kept in src/sdk/ with original filename
#include "sdk/ASICamera2.h"
// ── ASI image-type constants used locally ─────────────────────────────────────
// ASI_IMG_RAW8 = 0, ASI_IMG_RGB24 = 1, ASI_IMG_RAW16 = 2, ASI_IMG_Y8 = 3

// ── Constructor / Destructor ──────────────────────────────────────────────────

ZwoCamera::ZwoCamera(QObject *parent)
    : CameraInterface(parent)
{}

ZwoCamera::~ZwoCamera()
{
    close();
}

// ── open / close ──────────────────────────────────────────────────────────────

bool ZwoCamera::open()
{
    QMutexLocker lock(&m_mutex);
    if (m_open) return true;

    int numCams = ASIGetNumOfConnectedCameras();
    if (numCams <= 0) {
        qWarning() << "[ZwoCamera] No camera found – simulation mode";
        m_simulated = true;
        m_modelName = "Simulation";
        m_caps      = Caps{};
        m_open      = true;
        return true;
    }

    // Use the first available camera
    ASI_CAMERA_INFO info;
    if (ASIGetCameraProperty(&info, 0) != ASI_SUCCESS) {
        qWarning() << "[ZwoCamera] ASIGetCameraProperty failed – simulation mode";
        m_simulated = true;
        m_modelName = "Simulation (error)";
        m_caps      = Caps{};
        m_open      = true;
        return false;
    }

    m_cameraId  = info.CameraID;
    m_modelName = QString::fromLatin1(info.Name);

    if (ASIOpenCamera(m_cameraId) != ASI_SUCCESS ||
        ASIInitCamera(m_cameraId) != ASI_SUCCESS)
    {
        emit errorOccurred("ASIOpenCamera / ASIInitCamera failed");
        m_simulated = true;
        m_modelName = "Simulation (open error)";
        m_caps      = Caps{};
        m_open      = true;
        return false;
    }

    readCaps();

    // Apply default ROI (full sensor, RAW8, bin1)
    m_roiW    = info.MaxWidth;
    m_roiH    = info.MaxHeight;
    m_roiX    = 0;
    m_roiY    = 0;
    m_bin     = 1;
    m_imgType = 0;  // ASI_IMG_RAW8
    ASISetROIFormat(m_cameraId, m_roiW, m_roiH, m_bin, (ASI_IMG_TYPE)m_imgType);
    ASISetStartPos(m_cameraId, m_roiX, m_roiY);

    m_simulated = false;
    m_open      = true;
    qInfo() << "[ZwoCamera] Opened:" << m_modelName;
    return true;
}

void ZwoCamera::close()
{
    QMutexLocker lock(&m_mutex);
    if (!m_open) return;

    stopGrabbing();

    if (!m_simulated && m_cameraId >= 0) {
        // Wait for any in-progress ASIGetVideoData to finish before closing
        QMutexLocker grabLock(&m_grabMutex);
        ASICloseCamera(m_cameraId);
        m_cameraId = -1;
    }
    m_open = false;
    qInfo() << "[ZwoCamera] Closed";
}

// ── readCaps ──────────────────────────────────────────────────────────────────

void ZwoCamera::readCaps()
{
    if (m_cameraId < 0) return;

    ASI_CAMERA_INFO info;
    if (ASIGetCameraProperty(&info, 0) != ASI_SUCCESS) return;

    m_caps.widthMax  = static_cast<int>(info.MaxWidth);
    m_caps.heightMax = static_cast<int>(info.MaxHeight);

    // Query exposure range via control caps
    int numControls = 0;
    ASIGetNumOfControls(m_cameraId, &numControls);
    for (int i = 0; i < numControls; ++i) {
        ASI_CONTROL_CAPS cc;
        if (ASIGetControlCaps(m_cameraId, i, &cc) != ASI_SUCCESS) continue;
        switch (cc.ControlType) {
        case ASI_EXPOSURE:
            m_caps.exposureMin = static_cast<double>(cc.MinValue);   // µs
            m_caps.exposureMax = static_cast<double>(cc.MaxValue);
            break;
        case ASI_GAIN:
            m_caps.gainMin = static_cast<double>(cc.MinValue);
            m_caps.gainMax = static_cast<double>(cc.MaxValue);
            break;
        default:
            break;
        }
    }
}

// ── Parameter setters ─────────────────────────────────────────────────────────

void ZwoCamera::setExposure(double us)
{
    if (m_simulated || m_cameraId < 0) return;
    long val = static_cast<long>(us);
    ASI_ERROR_CODE err = ASISetControlValue(m_cameraId, ASI_EXPOSURE, val, ASI_FALSE);
    if (err != ASI_SUCCESS)
        qWarning() << "[ZwoCamera] setExposure error:" << err;
}

void ZwoCamera::setAutoExposure(bool on)
{
    if (m_simulated || m_cameraId < 0) return;
    // Read current value so we don't reset it, just toggle the auto flag
    long val = 0; ASI_BOOL isAuto = ASI_FALSE;
    ASIGetControlValue(m_cameraId, ASI_EXPOSURE, &val, &isAuto);
    ASISetControlValue(m_cameraId, ASI_EXPOSURE, val, on ? ASI_TRUE : ASI_FALSE);
}

void ZwoCamera::setGain(double db)
{
    if (m_simulated || m_cameraId < 0) return;
    // ASI gain is a unitless integer; callers pass a "dB-like" value but we
    // store the range as raw ASI units, so pass directly as long.
    long val = static_cast<long>(db);
    ASI_ERROR_CODE err = ASISetControlValue(m_cameraId, ASI_GAIN, val, ASI_FALSE);
    if (err != ASI_SUCCESS)
        qWarning() << "[ZwoCamera] setGain error:" << err;
}

void ZwoCamera::setOffset(int x, int y)
{
    if (m_simulated || m_cameraId < 0) return;
    QMutexLocker lock(&m_mutex);
    m_roiX = x;
    m_roiY = y;
    if (m_grabbing) ASIStopVideoCapture(m_cameraId);
    ASISetStartPos(m_cameraId, m_roiX, m_roiY);
    if (m_grabbing) ASIStartVideoCapture(m_cameraId);
}

void ZwoCamera::setRoi(int x, int y, int w, int h)
{
    if (m_simulated || m_cameraId < 0) return;
    QMutexLocker lock(&m_mutex);
    // ASI requires width/height to be multiples of 8
    w = (w / 8) * 8;
    h = (h / 8) * 8;
    m_roiX = x; m_roiY = y; m_roiW = w; m_roiH = h;
    if (m_grabbing) ASIStopVideoCapture(m_cameraId);
    ASISetROIFormat(m_cameraId, m_roiW, m_roiH, m_bin, (ASI_IMG_TYPE)m_imgType);
    ASISetStartPos(m_cameraId, m_roiX, m_roiY);
    if (m_grabbing) ASIStartVideoCapture(m_cameraId);
}

void ZwoCamera::clearRoi()
{
    setRoi(0, 0, m_caps.widthMax, m_caps.heightMax);
}

void ZwoCamera::setPixelFormat(const QString &fmt)
{
    if (m_simulated || m_cameraId < 0) return;
    QMutexLocker lock(&m_mutex);

    int newType = m_imgType;
    if      (fmt == "RAW8"  || fmt == "Mono8")   newType = 0; // ASI_IMG_RAW8
    else if (fmt == "RGB24")                      newType = 1; // ASI_IMG_RGB24
    else if (fmt == "RAW16" || fmt == "Mono16")   newType = 2; // ASI_IMG_RAW16
    else if (fmt == "Y8")                         newType = 3; // ASI_IMG_Y8
    else {
        qWarning() << "[ZwoCamera] Unknown pixel format:" << fmt;
        return;
    }
    if (newType == m_imgType) return;
    m_imgType = newType;

    if (m_grabbing) ASIStopVideoCapture(m_cameraId);
    ASISetROIFormat(m_cameraId, m_roiW, m_roiH, m_bin, (ASI_IMG_TYPE)m_imgType);
    if (m_grabbing) ASIStartVideoCapture(m_cameraId);
}

// ── Grabbing ──────────────────────────────────────────────────────────────────

void ZwoCamera::startGrabbing()
{
    if (m_grabbing) return;
    if (!m_simulated && m_cameraId >= 0) {
        ASI_ERROR_CODE err = ASIStartVideoCapture(m_cameraId);
        if (err != ASI_SUCCESS) {
            emit errorOccurred(QString("ASIStartVideoCapture failed: %1").arg(err));
            return;
        }
    }
    m_grabbing = true;
}

void ZwoCamera::stopGrabbing()
{
    if (!m_grabbing) return;
    if (!m_simulated && m_cameraId >= 0)
        ASIStopVideoCapture(m_cameraId);
    m_grabbing = false;
}

// ── grabFrame ─────────────────────────────────────────────────────────────────

QImage ZwoCamera::grabFrame()
{
    if (!m_open) return {};

    if (m_simulated) {
        const int W = 640, H = 480;
        QImage img(W, H, QImage::Format_Grayscale8);
        for (int y = 0; y < H; ++y) {
            uchar *row = img.scanLine(y);
            for (int x = 0; x < W; ++x)
                row[x] = static_cast<uchar>((x + y + m_simFrame) & 0xFF);
        }
        ++m_simFrame;
        return img;
    }

    // Buffer size: RAW8/Y8 = W*H, RGB24 = W*H*3, RAW16 = W*H*2
    int bytesPerPixel = 1;
    QImage::Format qFmt = QImage::Format_Grayscale8;
    switch (m_imgType) {
    case 1: bytesPerPixel = 3; qFmt = QImage::Format_RGB888;      break; // RGB24
    case 2: bytesPerPixel = 2; qFmt = QImage::Format_Grayscale16; break; // RAW16
    default: break;
    }

    // Hold m_grabMutex for the entire SDK call duration so close() can
    // safely wait for this to finish before calling ASICloseCamera
    QMutexLocker grabLock(&m_grabMutex);

    // Poll with short timeouts so we can react to stopGrabbing() quickly
    const int bufSize = m_roiW * m_roiH * bytesPerPixel;
    QByteArray buf(bufSize, 0);

    ASI_ERROR_CODE err = ASI_ERROR_TIMEOUT;
    while (m_open && m_grabbing) {
        grabLock.unlock();
        err = ASIGetVideoData(
            m_cameraId,
            reinterpret_cast<unsigned char*>(buf.data()),
            bufSize,
            100);
        grabLock.relock();
        if (err != ASI_ERROR_TIMEOUT) break;
    }

    if (!m_open || !m_grabbing) return {};

    if (err != ASI_SUCCESS) {
        // Only report genuine errors during active capture
        // Timeout and end-of-stream codes are normal during shutdown
        if (m_grabbing && err != ASI_ERROR_TIMEOUT)
            emit errorOccurred(QString("ASIGetVideoData error: %1").arg(err));
        return {};
    }

    // Direct copy — works for RAW8, RAW16 (Grayscale16), RGB24
    QImage img(m_roiW, m_roiH, qFmt);
    const int rowBytes = m_roiW * bytesPerPixel;
    for (int y = 0; y < m_roiH; ++y)
        std::memcpy(img.scanLine(y),
                    buf.constData() + y * rowBytes,
                    rowBytes);
    return img;
}
