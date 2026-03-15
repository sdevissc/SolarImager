#pragma once
#include <QApplication>
#include <QPalette>
#include <QColor>

namespace Theme {

/// Apply the Windows 10 light-gray Fusion palette to the application.
/// Call once after QApplication is constructed, before any window is shown.
inline void applyLightFusion(QApplication &app)
{
    app.setStyle("Fusion");

    QPalette pal;
    pal.setColor(QPalette::Window,          QColor(240, 240, 240));
    pal.setColor(QPalette::WindowText,      QColor(  0,   0,   0));
    pal.setColor(QPalette::Base,            QColor(255, 255, 255));
    pal.setColor(QPalette::AlternateBase,   QColor(233, 233, 233));
    pal.setColor(QPalette::ToolTipBase,     QColor(255, 255, 220));
    pal.setColor(QPalette::ToolTipText,     QColor(  0,   0,   0));
    pal.setColor(QPalette::Text,            QColor(  0,   0,   0));
    pal.setColor(QPalette::Button,          QColor(225, 225, 225));
    pal.setColor(QPalette::ButtonText,      QColor(  0,   0,   0));
    pal.setColor(QPalette::BrightText,      QColor(255,   0,   0));
    pal.setColor(QPalette::Highlight,       QColor(  0, 120, 215));
    pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    pal.setColor(QPalette::Mid,             QColor(200, 200, 200));
    pal.setColor(QPalette::Dark,            QColor(160, 160, 160));
    pal.setColor(QPalette::Shadow,          QColor(105, 105, 105));
    app.setPalette(pal);
}

/// Stylesheet for a checkable red/green toggle button.
/// Red = unchecked (inactive), green = checked (active).
inline const char* toggleButtonStyle()
{
    return
        "QPushButton {"
        "  background: #c0392b; color: white; font-weight: bold;"
        "  border-radius: 4px; padding: 2px 8px;"
        "}"
        "QPushButton:checked { background: #27ae60; }";
}

/// Stylesheet for a warning/orange action button.
inline const char* warnButtonStyle()
{
    return
        "QPushButton {"
        "  background: #c06000; color: white; font-weight: bold;"
        "  border-radius: 4px; padding: 4px;"
        "}";
}

} // namespace Theme
