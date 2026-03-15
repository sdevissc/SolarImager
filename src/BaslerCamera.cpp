#include "BaslerCamera.h"

#include <QDebug>
#include <cmath>
#include <cstring>

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>

using namespace Pylon;
using namespace GenApi;

// ── Constructor / Destructor ──────────────────────────────────────────────────

BaslerCamera::BaslerCamera(QObject *parent)
    : CameraInterface(parent)
{
    PylonInitialize();
}

BaslerCamera::~BaslerCamera()
{
    close();
    PylonTerminate();
}

// ── open / close ──────────────────────────────────────────────────────────────

bool BaslerCamera::open()
{
    QMutexLocker lock(&m_mutex);
    if (m_open) return true;

    try {
        CTlFactory &factory = CTlFactory::GetInstance();
        DeviceInfoList_t devices;
        if (factory.EnumerateDevices(devices) == 0) {
            qWarning() << "[BaslerCamera] No camera found – simulation mode";
            m_simulated = true;
            m_modelName = "Simulation";
            m_caps      = Caps{};
            m_open      = true;
            return true;
        }

        m_camera = new CInstantCamera(factory.CreateFirstDevice());
        m_camera->Open();

        m_modelName = QString::fromStdString(
            m_camera->GetDeviceInfo().GetModelName().c_str());

        readCaps();

        m_simulated = false;
        m_open      = true;
        qInfo() << "[BaslerCamera] Opened:" << m_modelName;
        return true;

    } catch (const GenericException &e) {
        emit errorOccurred(QString("Camera open failed: %1")
                           .arg(QString(e.GetDescription())));
        m_simulated = true;
        m_modelName = "Simulation (error)";
        m_caps      = Caps{};
        m_open      = true;
        return false;
    }
}

void BaslerCamera::close()
{
    QMutexLocker lock(&m_mutex);
    if (!m_open) return;

    stopGrabbing();

    if (m_camera && !m_simulated) {
        try {
            m_camera->Close();
        } catch (...) {}
        delete m_camera;
        m_camera = nullptr;
    }
    m_open = false;
    qInfo() << "[BaslerCamera] Closed";
}

// ── readCaps ──────────────────────────────────────────────────────────────────

void BaslerCamera::readCaps()
{
    if (!m_camera) return;
    try {
        auto &nm = m_camera->GetNodeMap();

        auto getFloatMin = [&](const char *name, double def) -> double {
            GenApi::CFloatPtr p(nm.GetNode(name));
            return (p.IsValid()) ? p->GetMin() : def;
        };
        auto getFloatMax = [&](const char *name, double def) -> double {
            GenApi::CFloatPtr p(nm.GetNode(name));
            return (p.IsValid()) ? p->GetMax() : def;
        };
        auto getIntMax = [&](const char *name, int def) -> int {
            GenApi::CIntegerPtr p(nm.GetNode(name));
            return (p.IsValid()) ? static_cast<int>(p->GetMax()) : def;
        };

        m_caps.exposureMin = getFloatMin("ExposureTime", 100.0);
        m_caps.exposureMax = getFloatMax("ExposureTime", 1'000'000.0);
        m_caps.gainMin     = getFloatMin("Gain", 0.0);
        m_caps.gainMax     = getFloatMax("Gain", 24.0);
        m_caps.widthMax    = getIntMax("Width",  1920);
        m_caps.heightMax   = getIntMax("Height", 1200);

    } catch (const GenericException &e) {
        qWarning() << "[BaslerCamera] readCaps error:" << e.GetDescription();
    }
}

// ── Parameter setters ─────────────────────────────────────────────────────────

// GenApi helpers — work with all Pylon versions
static void setFloat(GenApi::INodeMap &nm, const char *name, double val)
{
    GenApi::CFloatPtr p(nm.GetNode(name));
    if (p.IsValid() && GenApi::IsWritable(p)) p->SetValue(val);
}

static void setInt(GenApi::INodeMap &nm, const char *name, int64_t val)
{
    GenApi::CIntegerPtr p(nm.GetNode(name));
    if (p.IsValid() && GenApi::IsWritable(p)) p->SetValue(val);
}

void BaslerCamera::setExposure(double us)
{
    if (m_simulated || !m_camera) return;
    try {
        setFloat(m_camera->GetNodeMap(), "ExposureTime", us);
    } catch (const GenericException &e) {
        qWarning() << "[BaslerCamera] setExposure:" << e.GetDescription();
    }
}

void BaslerCamera::setAutoExposure(bool on)
{
    if (m_simulated || !m_camera) return;
    try {
        GenApi::CEnumerationPtr p(m_camera->GetNodeMap().GetNode("ExposureAuto"));
        if (p.IsValid() && GenApi::IsWritable(p))
            p->FromString(on ? "Continuous" : "Off");
    } catch (const GenericException &e) {
        qWarning() << "[BaslerCamera] setAutoExposure:" << e.GetDescription();
    }
}

void BaslerCamera::setGain(double db)
{
    if (m_simulated || !m_camera) return;
    try {
        setFloat(m_camera->GetNodeMap(), "Gain", db);
    } catch (const GenericException &e) {
        qWarning() << "[BaslerCamera] setGain:" << e.GetDescription();
    }
}

void BaslerCamera::setOffset(int x, int y)
{
    if (m_simulated || !m_camera) return;
    try {
        if (m_grabbing) m_camera->StopGrabbing();
        auto &nm = m_camera->GetNodeMap();
        setInt(nm, "OffsetX", x);
        setInt(nm, "OffsetY", y);
        if (m_grabbing) m_camera->StartGrabbing(GrabStrategy_OneByOne);
    } catch (const GenericException &e) {
        qWarning() << "[BaslerCamera] setOffset:" << e.GetDescription();
    }
}

void BaslerCamera::setRoi(int x, int y, int w, int h)
{
    if (m_simulated || !m_camera) return;
    try {
        bool wasGrabbing = m_grabbing;
        if (wasGrabbing) m_camera->StopGrabbing();
        auto &nm = m_camera->GetNodeMap();
        // Order: reset offsets → set size → set offsets (Basler requirement)
        setInt(nm, "OffsetX", 0);
        setInt(nm, "OffsetY", 0);
        setInt(nm, "Width",   w);
        setInt(nm, "Height",  h);
        setInt(nm, "OffsetX", x);
        setInt(nm, "OffsetY", y);
        if (wasGrabbing) m_camera->StartGrabbing(GrabStrategy_OneByOne);
    } catch (const GenericException &e) {
        qWarning() << "[BaslerCamera] setRoi:" << e.GetDescription();
    }
}

void BaslerCamera::clearRoi()
{
    if (m_simulated || !m_camera) return;
    try {
        bool wasGrabbing = m_grabbing;
        if (wasGrabbing) m_camera->StopGrabbing();
        auto &nm = m_camera->GetNodeMap();
        setInt(nm, "OffsetX", 0);
        setInt(nm, "OffsetY", 0);
        setInt(nm, "Width",   m_caps.widthMax);
        setInt(nm, "Height",  m_caps.heightMax);
        if (wasGrabbing) m_camera->StartGrabbing(GrabStrategy_OneByOne);
    } catch (const GenericException &e) {
        qWarning() << "[BaslerCamera] clearRoi:" << e.GetDescription();
    }
}

void BaslerCamera::setPixelFormat(const QString &fmt)
{
    if (m_simulated || !m_camera) return;
    try {
        bool wasGrabbing = m_grabbing;
        if (wasGrabbing) m_camera->StopGrabbing();
        CEnumParameter pixFmt(m_camera->GetNodeMap(), "PixelFormat");
        pixFmt.SetValue(fmt.toStdString().c_str());
        if (wasGrabbing) m_camera->StartGrabbing(GrabStrategy_OneByOne);
    } catch (const GenericException &e) {
        qWarning() << "[BaslerCamera] setPixelFormat:" << e.GetDescription();
    }
}

// ── Grabbing ──────────────────────────────────────────────────────────────────

void BaslerCamera::startGrabbing()
{
    if (m_grabbing) return;
    if (!m_simulated && m_camera) {
        try {
            m_camera->StartGrabbing(GrabStrategy_OneByOne);
        } catch (const GenericException &e) {
            emit errorOccurred(QString("StartGrabbing failed: %1")
                               .arg(QString(e.GetDescription())));
            return;
        }
    }
    m_grabbing = true;
}

void BaslerCamera::stopGrabbing()
{
    if (!m_grabbing) return;
    if (!m_simulated && m_camera) {
        try {
            m_camera->StopGrabbing();
        } catch (...) {}
    }
    m_grabbing = false;
}

// ── grabFrame ─────────────────────────────────────────────────────────────────

QImage BaslerCamera::grabFrame()
{
    if (!m_open) return {};

    if (m_simulated) {
        // Generate a simple gradient test pattern
        const int W = 640, H = 480;
        QImage img(W, H, QImage::Format_Grayscale8);
        for (int y = 0; y < H; ++y) {
            uchar *row = img.scanLine(y);
            for (int x = 0; x < W; ++x) {
                row[x] = static_cast<uchar>(
                    (x + y + m_simFrame) & 0xFF);
            }
        }
        ++m_simFrame;
        return img;
    }

    try {
        CGrabResultPtr result;
        m_camera->RetrieveResult(5000, result, TimeoutHandling_ThrowException);

        if (!result->GrabSucceeded()) {
            emit errorOccurred(QString("Grab failed: %1")
                               .arg(QString(result->GetErrorDescription())));
            return {};
        }
        return pylonResultToQImage(result);

    } catch (const GenericException &e) {
        emit errorOccurred(QString("grabFrame exception: %1")
                           .arg(QString(e.GetDescription())));
        return {};
    }
}

// ── pylonResultToQImage ───────────────────────────────────────────────────────

QImage BaslerCamera::pylonResultToQImage(CGrabResultPtr &result)
{
    const int W = static_cast<int>(result->GetWidth());
    const int H = static_cast<int>(result->GetHeight());
    const void *buf = result->GetBuffer();

    EPixelType pixType = result->GetPixelType();

    if (pixType == PixelType_Mono8) {
        QImage img(W, H, QImage::Format_Grayscale8);
        for (int y = 0; y < H; ++y)
            std::memcpy(img.scanLine(y),
                        static_cast<const uchar*>(buf) + y * W, W);
        return img;
    }

    if (pixType == PixelType_Mono12 || pixType == PixelType_Mono16) {
        // Scale 12/16-bit → 8-bit for display
        QImage img(W, H, QImage::Format_Grayscale8);
        const auto *src = static_cast<const uint16_t*>(buf);
        for (int y = 0; y < H; ++y) {
            uchar *dst = img.scanLine(y);
            for (int x = 0; x < W; ++x)
                dst[x] = static_cast<uchar>(src[y * W + x] >> 4);
        }
        return img;
    }

    if (Pylon::IsColorImage(pixType)) {
        // Convert to RGB8 using Pylon's software converter
        CImageFormatConverter conv;
        conv.OutputPixelFormat = PixelType_RGB8packed;
        CPylonImage destImage;
        conv.Convert(destImage, result);
        QImage img(W, H, QImage::Format_RGB888);
        for (int y = 0; y < H; ++y)
            std::memcpy(img.scanLine(y),
                        static_cast<const uchar*>(destImage.GetBuffer()) + y * W * 3,
                        W * 3);
        return img;
    }

    qWarning() << "[BaslerCamera] Unsupported pixel type:" << pixType;
    return {};
}
