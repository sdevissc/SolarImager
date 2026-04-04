#pragma once
#include <QString>
#include <QJsonObject>

/// All user-configurable settings, serialisable to/from JSON.
struct AppSettings
{
    // ── Telescope ─────────────────────────────────────────────────────────
    double  telDiameter    = 200.0;   // mm
    double  telFocalLength = 3000.0;  // mm

    // ── Camera ───────────────────────────────────────────────────────────
    double  camPixelSize   = 2.9;     // µm
    double  camWavelength  = 550.0;   // nm (for sampling calc)
    QString camDefaultFormat = "RAW8";
    int     camDefaultBitDepth = 8;

    // ── Display ───────────────────────────────────────────────────────────
    QString displayPalette = "Original";
    QString displayZoom    = "Fit";

    // ── Recording ────────────────────────────────────────────────────────
    QString recDirectory   = "";      // empty = home dir
    int     recDuration    = 100;
    QString recDurationMode = "Frames";

    // ── SSM ──────────────────────────────────────────────────────────────
    double  ssmThreshold   = 2.0;     // arcsec
    int     ssmConsec      = 3;
    QString ssmPort        = "";
    QString ssmBaud        = "115200";

    // ── Profile name ─────────────────────────────────────────────────────
    QString profileName    = "Default";

    // ── Serialisation ────────────────────────────────────────────────────
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);

    /// Save to ~/.config/SolarImager/profiles/<name>.json
    bool save(const QString &name = QString()) const;
    /// Load from ~/.config/SolarImager/profiles/<name>.json
    static AppSettings load(const QString &name);
    /// List available profile names
    static QStringList availableProfiles();
    /// Directory where profiles are stored
    static QString profileDir();
};
