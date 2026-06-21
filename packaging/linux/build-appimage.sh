#!/usr/bin/env bash
# Build a self-contained AppImage for DJ Library Goodizer using linuxdeploy +
# its Qt plugin. Usage: packaging/linux/build-appimage.sh <build-dir>
#
# Requires: linuxdeploy-x86_64.AppImage and linuxdeploy-plugin-qt-x86_64.AppImage
# on PATH (download from https://github.com/linuxdeploy/linuxdeploy/releases).
set -euo pipefail

BUILD_DIR="${1:-build}"
APPDIR="$(pwd)/AppDir"

rm -rf "$APPDIR"
cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

# Minimal desktop entry + icon are required by linuxdeploy.
install -Dm644 /dev/stdin "$APPDIR/usr/share/applications/dj-lib-goodizer.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=DJ Library Goodizer
Exec=dj-lib-goodizer
Icon=dj-lib-goodizer
Categories=AudioVideo;Audio;
EOF
# A placeholder icon; replace with real artwork before release.
: > "$APPDIR/usr/share/icons/hicolor/256x256/apps/dj-lib-goodizer.png" || true

export QML_SOURCES_PATHS=""
linuxdeploy-x86_64.AppImage \
  --appdir "$APPDIR" \
  --plugin qt \
  --output appimage

echo "AppImage written to $(pwd)"
