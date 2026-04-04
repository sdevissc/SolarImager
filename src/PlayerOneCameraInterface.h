#pragma once

#include "CameraInterface.h"
#include <QMutex>

// Player One SDK forward declarations — SDK header stays private to
// PlayerOneCameraLib OBJECT library (compiled in isolation like BaslerCameraLib).

/// Player One camera implementation using the Player One Astronomy SDK
/// (PlayerOneCamera.h / libPlayerOneCamera).
/// Mirrors the ZwoCamera / BaslerCamera pattern exactly.
class PlayerOneCamera : public CameraInterface
{
    Q_OBJECT

public:
    explicit PlayerOneCamera(QObject *parent = nullptr);
    ~PlayerOneCamera() override;

    // ── CameraInterface ───────────────────────────────────────────────────
    QString modelName()  const override { return m_modelName; }
    QString brand()      const override { return "Player One"; }

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
    /// Convert a Player One imgFormat integer to a QImage format + bytes/pixel.
    void imgFormatParams(int poImgFmt, QImage::Format &qFmt, int &bpp) const;

    int             m_cameraId  = -1;
    bool            m_open      = false;
    bool            m_simulated = false;
    bool            m_grabbing  = false;
    Caps            m_caps;
    QString         m_modelName;
    mutable QMutex  m_mutex;
    mutable QMutex  m_grabMutex;  ///< held during POAGetImageData, prevents close() racing
    int             m_simFrame  = 0;

    // Current ROI / format state (PO SDK requires full params on every ROI call)
    int m_roiX    = 0;
    int m_roiY    = 0;
    int m_roiW    = 1920;
    int m_roiH    = 1080;
    int m_bin     = 1;
    int m_imgFmt  = 0;   // POAImgFormat: 0 = RAW8
};
