#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>

/// Pure abstract interface for all supported cameras.
/// MainWindow and FrameGrabber only ever hold a CameraInterface* —
/// they never know which brand is underneath.
///
/// Concrete implementations:
///   BaslerCamera    — Basler Pylon SDK
///   ZwoCamera       — ZWO ASI SDK   (ASICamera2.h)
///   PlayerOneCamera — Player One SDK (PlayerOneCamera.h)
class CameraInterface : public QObject
{
    Q_OBJECT

public:
    /// Camera capabilities read on open().
    struct Caps {
        double exposureMin  =    100.0;
        double exposureMax  = 1'000'000.0;
        double gainMin      =   0.0;
        double gainMax      =  24.0;
        int    widthMax     = 1920;
        int    heightMax    = 1200;
    };

    explicit CameraInterface(QObject *parent = nullptr)
        : QObject(parent) {}

    ~CameraInterface() override = default;

    // ── Identity ──────────────────────────────────────────────────────────
    /// Human-readable brand + model string, e.g. "Basler acA1920-155um"
    virtual QString modelName()  const = 0;
    /// Short brand tag used in UI selectors, e.g. "Basler", "ZWO", "PlayerOne"
    virtual QString brand()      const = 0;

    // ── Lifecycle ─────────────────────────────────────────────────────────
    /// Open the first available camera of this brand.
    /// Returns true on success. isSimulated() may be true even on success.
    virtual bool open()  = 0;
    virtual void close() = 0;

    virtual bool isOpen()      const = 0;
    virtual bool isSimulated() const = 0;
    virtual Caps caps()        const = 0;

    // ── Parameters ────────────────────────────────────────────────────────
    virtual void setExposure(double us)                    = 0;
    virtual void setAutoExposure(bool on)                  = 0;
    virtual void setGain(double db)                        = 0;
    virtual void setOffset(int x, int y)                   = 0;
    virtual void setRoi(int x, int y, int w, int h)        = 0;
    virtual void clearRoi()                                = 0;
    virtual void setPixelFormat(const QString &fmt)        = 0;

    /// Returns the list of pixel format strings this camera supports.
    /// Used to populate the UI combo after connect.
    virtual QStringList supportedFormats() const = 0;

    // ── Acquisition ───────────────────────────────────────────────────────
    virtual void   startGrabbing() = 0;
    virtual void   stopGrabbing()  = 0;
    /// Grab one frame. Returns QImage (Grayscale8 or RGB888).
    /// Returns null QImage on timeout or error.
    virtual QImage grabFrame()     = 0;

signals:
    void errorOccurred(const QString &msg);
};
