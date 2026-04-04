#!/bin/bash
# Install Solar Imaging Console system-wide
# Run as: sudo ./install.sh  (from the build directory)

set -e

INSTALL_DIR="/opt/solar-imager"
DESKTOP_DIR="/usr/share/applications"
ICON_DIR="/usr/share/icons/hicolor/scalable/apps"
REPO_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "Installing Solar Imaging Console..."

# Create install directory and copy executable
mkdir -p "$INSTALL_DIR"
cp SolarImager "$INSTALL_DIR/"
chmod +x "$INSTALL_DIR/SolarImager"

# Copy icon
mkdir -p "$ICON_DIR"
cp "$REPO_DIR/solar-imager.svg" "$INSTALL_DIR/"
cp "$REPO_DIR/solar-imager.svg" "$ICON_DIR/solar-imager.svg"

# Install .desktop file
cp "$REPO_DIR/solar-imager.desktop" "$DESKTOP_DIR/"
chmod 644 "$DESKTOP_DIR/solar-imager.desktop"

# Refresh icon cache
update-icon-caches "$ICON_DIR" 2>/dev/null || true

echo "Done. Solar Imaging Console is now available in your app launcher."
echo "You can also run it directly with: $INSTALL_DIR/SolarImager"
