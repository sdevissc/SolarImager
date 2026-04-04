#pragma once

#include "AppSettings.h"
#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QLabel;

/// Settings dialog — edit all user-configurable parameters and manage profiles.
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const AppSettings &current, QWidget *parent = nullptr);

    /// Returns the settings as edited by the user (call after exec() == Accepted)
    AppSettings settings() const;

private slots:
    void onSaveProfile();
    void onLoadProfile();
    void onDeleteProfile();
    void onProfileSelected();

private:
    void buildUi();
    void populateFromSettings(const AppSettings &s);
    void refreshProfileList();

    // ── Telescope ──────────────────────────────────────────────────────────
    QDoubleSpinBox *m_spinDiameter    = nullptr;
    QDoubleSpinBox *m_spinFocalLength = nullptr;

    // ── Camera ─────────────────────────────────────────────────────────────
    QDoubleSpinBox *m_spinPixelSize   = nullptr;
    QDoubleSpinBox *m_spinWavelength  = nullptr;
    QComboBox      *m_comboFormat     = nullptr;
    QComboBox      *m_comboBitDepth   = nullptr;

    // ── Display ────────────────────────────────────────────────────────────
    QComboBox      *m_comboPalette    = nullptr;
    QComboBox      *m_comboZoom       = nullptr;

    // ── Recording ──────────────────────────────────────────────────────────
    QLineEdit      *m_txtRecDir       = nullptr;
    QSpinBox       *m_spinDuration    = nullptr;
    QComboBox      *m_comboDurMode    = nullptr;

    // ── SSM ────────────────────────────────────────────────────────────────
    QDoubleSpinBox *m_spinThreshold   = nullptr;
    QSpinBox       *m_spinConsec      = nullptr;
    QComboBox      *m_comboBaud       = nullptr;

    // ── Profiles ───────────────────────────────────────────────────────────
    QListWidget    *m_profileList     = nullptr;
    QLineEdit      *m_txtProfileName  = nullptr;
    QPushButton    *m_btnSave         = nullptr;
    QPushButton    *m_btnLoad         = nullptr;
    QPushButton    *m_btnDelete       = nullptr;

    AppSettings     m_settings;
};
