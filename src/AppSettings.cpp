#include "AppSettings.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDebug>

// ── JSON helpers ──────────────────────────────────────────────────────────────

QJsonObject AppSettings::toJson() const
{
    QJsonObject o;
    // Telescope
    o["telDiameter"]      = telDiameter;
    o["telFocalLength"]   = telFocalLength;
    // Camera
    o["camPixelSize"]     = camPixelSize;
    o["camWavelength"]    = camWavelength;
    o["camDefaultFormat"] = camDefaultFormat;
    o["camDefaultBitDepth"] = camDefaultBitDepth;
    // Display
    o["displayPalette"]   = displayPalette;
    o["displayZoom"]      = displayZoom;
    // Recording
    o["recDirectory"]     = recDirectory;
    o["recDuration"]      = recDuration;
    o["recDurationMode"]  = recDurationMode;
    // SSM
    o["ssmThreshold"]     = ssmThreshold;
    o["ssmConsec"]        = ssmConsec;
    o["ssmPort"]          = ssmPort;
    o["ssmBaud"]          = ssmBaud;
    // Profile
    o["profileName"]      = profileName;
    return o;
}

void AppSettings::fromJson(const QJsonObject &o)
{
    auto dbl = [&](const char *k, double def) {
        return o.contains(k) ? o[k].toDouble(def) : def; };
    auto str = [&](const char *k, const QString &def) {
        return o.contains(k) ? o[k].toString(def) : def; };
    auto i = [&](const char *k, int def) {
        return o.contains(k) ? o[k].toInt(def) : def; };

    telDiameter       = dbl("telDiameter",    200.0);
    telFocalLength    = dbl("telFocalLength",  3000.0);
    camPixelSize      = dbl("camPixelSize",    2.9);
    camWavelength     = dbl("camWavelength",   550.0);
    camDefaultFormat  = str("camDefaultFormat", "RAW8");
    camDefaultBitDepth = i("camDefaultBitDepth", 8);
    displayPalette    = str("displayPalette",  "Original");
    displayZoom       = str("displayZoom",     "Fit");
    recDirectory      = str("recDirectory",    "");
    recDuration       = i("recDuration",       100);
    recDurationMode   = str("recDurationMode", "Frames");
    ssmThreshold      = dbl("ssmThreshold",    2.0);
    ssmConsec         = i("ssmConsec",         3);
    ssmPort           = str("ssmPort",         "");
    ssmBaud           = str("ssmBaud",         "115200");
    profileName       = str("profileName",     "Default");
}

// ── File I/O ──────────────────────────────────────────────────────────────────

QString AppSettings::profileDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                  + "/SolarImager/profiles";
    QDir().mkpath(dir);
    return dir;
}

bool AppSettings::save(const QString &name) const
{
    AppSettings copy = *this;
    if (!name.isEmpty()) copy.profileName = name;

    const QString path = profileDir() + "/" + copy.profileName + ".json";
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "[AppSettings] Cannot write" << path;
        return false;
    }
    f.write(QJsonDocument(copy.toJson()).toJson(QJsonDocument::Indented));
    qInfo() << "[AppSettings] Saved profile:" << copy.profileName;
    return true;
}

AppSettings AppSettings::load(const QString &name)
{
    const QString path = profileDir() + "/" + name + ".json";
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[AppSettings] Cannot read" << path << "— using defaults";
        AppSettings s; s.profileName = name; return s;
    }
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (doc.isNull()) {
        qWarning() << "[AppSettings] JSON parse error:" << err.errorString();
        AppSettings s; s.profileName = name; return s;
    }
    AppSettings s;
    s.fromJson(doc.object());
    s.profileName = name;
    return s;
}

QStringList AppSettings::availableProfiles()
{
    QStringList names;
    for (const QString &f : QDir(profileDir()).entryList({"*.json"}, QDir::Files))
        names << f.left(f.length() - 5);  // strip .json
    return names;
}
