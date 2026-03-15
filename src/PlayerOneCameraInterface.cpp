#include "PlayerOneCameraInterface.h"

#include <QDebug>
#include <cstring>

// Player One SDK header — kept in src/sdk/ with original filename
#include "sdk/PlayerOneCamera.h"
// ── POA image-format constants (from PlayerOneCamera.h) ───────────────────────
// POA_RAW8  = 0
// POA_RGB24 = 1
// POA_RAW16 = 2
// POA_MONO8 = 3   (debayered grayscale on colour cameras)

// ── Constructor / Destructor ──────────────────────────────────────────────────

PlayerOneCamera::PlayerOneCamera(QObject *parent)
    : CameraInterface(parent)
{}

PlayerOneCamera::~PlayerOneCamera()
{
    close();
}

// ── open / close ──────────────────────────────────────────────────────────────

bool PlayerOneCamera::open()
{
    QMutexLocker lock(&m_mutex);
    if (m_open) return true;

    int numCams = POAGetCameraCount();
    if (numCams <= 0) {
        qWarning() << "[PlayerOneCamera] No camera found – simulation mode";
        m_simulated = true;
        m_modelName = "Simulation";
        m_caps      = Caps{};
        m_open      = true;
        return true;
    }

    // Use the first available camera
    POACameraProperties prop;
    if (POAGetCameraProperties(0, &prop) != POA_OK) {
        qWarning() << "[PlayerOneCamera] POAGetCameraProperties failed – simulation";
        m_simulated = true;
        m_modelName = "Simulation (error)";
        m_caps      = Caps{};
        m_open      = true;
        return false;
    }

    m_cameraId  = prop.cameraID;
    m_modelName = QString::fromLatin1(prop.cameraModelName);

    if (POAOpenCamera(m_cameraId) != POA_OK ||
        POAInitCamera(m_cameraId) != POA_OK)
    {
        emit errorOccurred("POAOpenCamera / POAInitCamera failed");
        m_simulated = true;
        m_modelName = "Simulation (open error)";
        m_caps      = Caps{};
        m_open      = true;
        return false;
    }

    readCaps();

    // Default ROI: full sensor, RAW8, bin1
    m_roiW   = static_cast<int>(prop.maxWidth);
    m_roiH   = static_cast<int>(prop.maxHeight);
    m_roiX   = 0;
    m_roiY   = 0;
    m_bin    = 1;
    m_imgFmt = 0;  // POA_RAW8

    POASetImageFormat(m_cameraId, (POAImgFormat)m_imgFmt);
    POASetImageBin(m_cameraId, m_bin);
    POASetImageSize(m_cameraId, m_roiW, m_roiH);
    POASetImageStartPos(m_cameraId, m_roiX, m_roiY);

    m_simulated = false;
    m_open      = true;
    qInfo() << "[PlayerOneCamera] Opened:" << m_modelName;
    return true;
}

void PlayerOneCamera::close()
{
    QMutexLocker lock(&m_mutex);
    if (!m_open) return;

    stopGrabbing();

    if (!m_simulated && m_cameraId >= 0) {
        POACloseCamera(m_cameraId);
        m_cameraId = -1;
    }
    m_open = false;
    qInfo() << "[PlayerOneCamera] Closed";
}

// ── readCaps ──────────────────────────────────────────────────────────────────

void PlayerOneCamera::readCaps()
{
    if (m_cameraId < 0) return;

    POACameraProperties prop;
    if (POAGetCameraProperties(0, &prop) != POA_OK) return;

    m_caps.widthMax  = static_cast<int>(prop.maxWidth);
    m_caps.heightMax = static_cast<int>(prop.maxHeight);

    int numControls = 0;
    POAGetConfigsCount(m_cameraId, &numControls);
    for (int i = 0; i < numControls; ++i) {
        POAConfigAttributes attr;
        if (POAGetConfigAttributes(m_cameraId, i, &attr) != POA_OK) continue;
        switch (attr.configID) {
        case POA_EXPOSURE:
            m_caps.exposureMin = static_cast<double>(attr.minValue.intValue); // µs
            m_caps.exposureMax = static_cast<double>(attr.maxValue.intValue);
            break;
        case POA_GAIN:
            m_caps.gainMin = static_cast<double>(attr.minValue.intValue);
            m_caps.gainMax = static_cast<double>(attr.maxValue.intValue);
            break;
        default:
            break;
        }
    }
}

// ── Parameter setters ─────────────────────────────────────────────────────────

void PlayerOneCamera::setExposure(double us)
{
    if (m_simulated || m_cameraId < 0) return;
    POAConfigValue val;
    val.intValue = static_cast<long>(us);
    POAErrors err = POASetConfig(m_cameraId, POA_EXPOSURE, val, POA_FALSE);
    if (err != POA_OK)
        qWarning() << "[PlayerOneCamera] setExposure error:" << err;
}

void PlayerOneCamera::setAutoExposure(bool on)
{
    if (m_simulated || m_cameraId < 0) return;
    POAConfigValue val;
    POABool isAuto = POA_FALSE;
    POAGetConfig(m_cameraId, POA_EXPOSURE, &val, &isAuto);
    POASetConfig(m_cameraId, POA_EXPOSURE, val, on ? POA_TRUE : POA_FALSE);
}

void PlayerOneCamera::setGain(double db)
{
    if (m_simulated || m_cameraId < 0) return;
    POAConfigValue val;
    val.intValue = static_cast<long>(db);
    POAErrors err = POASetConfig(m_cameraId, POA_GAIN, val, POA_FALSE);
    if (err != POA_OK)
        qWarning() << "[PlayerOneCamera] setGain error:" << err;
}

void PlayerOneCamera::setOffset(int x, int y)
{
    if (m_simulated || m_cameraId < 0) return;
    QMutexLocker lock(&m_mutex);
    m_roiX = x;
    m_roiY = y;
    if (m_grabbing) POAStopExposure(m_cameraId);
    POASetImageStartPos(m_cameraId, m_roiX, m_roiY);
    if (m_grabbing) POAStartExposure(m_cameraId, POA_FALSE /*video*/);
}

void PlayerOneCamera::setRoi(int x, int y, int w, int h)
{
    if (m_simulated || m_cameraId < 0) return;
    QMutexLocker lock(&m_mutex);
    // PO SDK requires dimensions to be multiples of 8
    w = (w / 8) * 8;
    h = (h / 8) * 8;
    m_roiX = x; m_roiY = y; m_roiW = w; m_roiH = h;
    if (m_grabbing) POAStopExposure(m_cameraId);
    POASetImageSize(m_cameraId, m_roiW, m_roiH);
    POASetImageStartPos(m_cameraId, m_roiX, m_roiY);
    if (m_grabbing) POAStartExposure(m_cameraId, POA_FALSE);
}

void PlayerOneCamera::clearRoi()
{
    setRoi(0, 0, m_caps.widthMax, m_caps.heightMax);
}

void PlayerOneCamera::setPixelFormat(const QString &fmt)
{
    if (m_simulated || m_cameraId < 0) return;
    QMutexLocker lock(&m_mutex);

    int newFmt = m_imgFmt;
    if      (fmt == "RAW8"  || fmt == "Mono8")   newFmt = 0; // POA_RAW8
    else if (fmt == "RGB24")                      newFmt = 1; // POA_RGB24
    else if (fmt == "RAW16" || fmt == "Mono16")   newFmt = 2; // POA_RAW16
    else if (fmt == "Mono8D")                     newFmt = 3; // POA_MONO8
    else {
        qWarning() << "[PlayerOneCamera] Unknown pixel format:" << fmt;
        return;
    }
    if (newFmt == m_imgFmt) return;
    m_imgFmt = newFmt;

    if (m_grabbing) POAStopExposure(m_cameraId);
    POASetImageFormat(m_cameraId, (POAImgFormat)m_imgFmt);
    if (m_grabbing) POAStartExposure(m_cameraId, POA_FALSE);
}

// ── imgFormatParams ───────────────────────────────────────────────────────────

void PlayerOneCamera::imgFormatParams(int poImgFmt,
                                      QImage::Format &qFmt, int &bpp) const
{
    switch (poImgFmt) {
    case 1:  qFmt = QImage::Format_RGB888;    bpp = 3; break; // POA_RGB24
    case 2:  qFmt = QImage::Format_Grayscale8; bpp = 2; break; // POA_RAW16 → 8
    default: qFmt = QImage::Format_Grayscale8; bpp = 1; break; // RAW8 / MONO8
    }
}

// ── Grabbing ──────────────────────────────────────────────────────────────────

void PlayerOneCamera::startGrabbing()
{
    if (m_grabbing) return;
    if (!m_simulated && m_cameraId >= 0) {
        // POA_FALSE = video (continuous) mode
        POAErrors err = POAStartExposure(m_cameraId, POA_FALSE);
        if (err != POA_OK) {
            emit errorOccurred(QString("POAStartExposure failed: %1").arg(err));
            return;
        }
    }
    m_grabbing = true;
}

void PlayerOneCamera::stopGrabbing()
{
    if (!m_grabbing) return;
    if (!m_simulated && m_cameraId >= 0)
        POAStopExposure(m_cameraId);
    m_grabbing = false;
}

// ── grabFrame ─────────────────────────────────────────────────────────────────

QImage PlayerOneCamera::grabFrame()
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

    QImage::Format qFmt; int bpp;
    imgFormatParams(m_imgFmt, qFmt, bpp);
    const int bufSize = m_roiW * m_roiH * bpp;
    QByteArray buf(bufSize, 0);

    // Poll for a ready frame with a 2-second timeout
    POAErrors err = POAGetImageData(
        m_cameraId,
        reinterpret_cast<unsigned char*>(buf.data()),
        bufSize,
        2000);

    if (err != POA_OK) {
        if (err != POA_ERROR_TIMEOUT)
            emit errorOccurred(QString("POAGetImageData error: %1").arg(err));
        return {};
    }

    if (m_imgFmt == 2) {
        // RAW16 → 8-bit: shift top 8 bits
        QImage img(m_roiW, m_roiH, QImage::Format_Grayscale8);
        const auto *src = reinterpret_cast<const uint16_t*>(buf.constData());
        for (int y = 0; y < m_roiH; ++y) {
            uchar *dst = img.scanLine(y);
            for (int x = 0; x < m_roiW; ++x)
                dst[x] = static_cast<uchar>(src[y * m_roiW + x] >> 8);
        }
        return img;
    }

    // RAW8 / MONO8 / RGB24: direct copy
    QImage img(m_roiW, m_roiH, qFmt);
    const int rowBytes = m_roiW * bpp;
    for (int y = 0; y < m_roiH; ++y)
        std::memcpy(img.scanLine(y),
                    buf.constData() + y * rowBytes,
                    rowBytes);
    return img;
}
