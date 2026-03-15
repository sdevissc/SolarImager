#pragma once

#include "CameraInterface.h"
#include <QMutex>

// Pylon forward declarations
namespace Pylon { class CInstantCamera; class CGrabResultPtr; }

/// Basler camera implementation using the Pylon SDK.
class BaslerCamera : public CameraInterface
{
    Q_OBJECT

public:
    explicit BaslerCamera(QObject *parent = nullptr);
    ~BaslerCamera() override;

    // ── CameraInterface ───────────────────────────────────────────────────
    QString modelName()  const override { return m_modelName; }
    QString brand()      const override { return "Basler"; }

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
        return {"Mono8", "Mono12", "BayerRG8"};
    }

    void   startGrabbing() override;
    void   stopGrabbing()  override;
    QImage grabFrame()     override;

private:
    void    readCaps();
    QImage  pylonResultToQImage(Pylon::CGrabResultPtr &result);

    Pylon::CInstantCamera *m_camera    = nullptr;
    bool                   m_open      = false;
    bool                   m_simulated = false;
    bool                   m_grabbing  = false;
    Caps                   m_caps;
    QString                m_modelName;
    mutable QMutex         m_mutex;
    int                    m_simFrame  = 0;
};
