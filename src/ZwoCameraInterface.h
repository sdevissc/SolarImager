#pragma once

#include "CameraInterface.h"
#include <QMutex>

// The ZWO ASI SDK header (ASICamera2.h) is included only in ZwoCameraInterface.cpp,
// which is compiled in the isolated ZwoCameraLib OBJECT library.
// No SDK types are needed in this header.

/// ZWO ASI camera implementation using the ASI SDK (ASICamera2.h / libASICamera2).
/// Mirrors the BaslerCamera pattern:
///   – SDK headers are private to ZwoCameraLib OBJECT library
///   – Falls back to a simulation gradient if no camera is found
class ZwoCamera : public CameraInterface
{
    Q_OBJECT

public:
    explicit ZwoCamera(QObject *parent = nullptr);
    ~ZwoCamera() override;

    // ── CameraInterface ───────────────────────────────────────────────────
    QString modelName()  const override { return m_modelName; }
    QString brand()      const override { return "ZWO"; }

    bool open()  override;
    void close() override;

    bool isOpen()      const override { return m_open; }
    bool isSimulated() const override { return m_simulated; }
    Caps caps()        const override { return m_caps; }

    void setExposure(double us)             override;
    void setAutoExposure(bool on)           override;
    void setGain(double db)                 override;
    void setOffset(int x, int y)            override;
    void setRoi(int x, int y, int w, int h) override;
    void clearRoi()                         override;
    void setPixelFormat(const QString &fmt) override;
    QStringList supportedFormats() const override {
        return {"RAW8", "RAW16"};
    }

    void   startGrabbing() override;
    void   stopGrabbing()  override;
    QImage grabFrame()     override;

private:
    void readCaps();

    int             m_cameraId  = -1;   ///< ASI camera index (0-based)
    bool            m_open      = false;
    bool            m_simulated = false;
    bool            m_grabbing  = false;
    Caps            m_caps;
    QString         m_modelName;
    mutable QMutex  m_mutex;
    mutable QMutex  m_grabMutex;  ///< held during ASIGetVideoData, prevents close() racing
    int             m_simFrame  = 0;

    // Current ROI / format state — needed because ASI SetROIFormat
    // must always be called with all four parameters at once
    int     m_roiX   = 0;
    int     m_roiY   = 0;
    int     m_roiW   = 1920;
    int     m_roiH   = 1080;
    int     m_bin    = 1;
    int     m_imgType = 0;   // ASI_IMG_RAW8
};
