#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-macos}"
CONFIG="${CONFIG:-Release}"
ARCHS="${ARCHS:-arm64;x86_64}"
VERSION="$(sed -nE 's/.*project\([^)]*VERSION[[:space:]]+([0-9.]+).*/\1/p' "$ROOT/CMakeLists.txt" | head -n 1)"

[[ "$(uname -s)" == "Darwin" ]] || { echo "macOS package must be built on macOS." >&2; exit 1; }
[[ -n "$VERSION" ]] || { echo "Could not read project version." >&2; exit 1; }

cmake -S "$ROOT" -B "$BUILD_DIR" -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
cmake --build "$BUILD_DIR" --config "$CONFIG" \
  --target PresentombDSO2_VST3 PresentombDSO2_AU PresentombDSO2_Standalone

ARTIFACTS="$BUILD_DIR/PresentombDSO2_artefacts/$CONFIG"
VST3="$ARTIFACTS/VST3/presentomb vector.vst3"
AU="$ARTIFACTS/AU/presentomb vector.component"
APP="$ARTIFACTS/Standalone/presentomb vector.app"
for path in "$VST3" "$AU" "$APP"; do [[ -e "$path" ]] || { echo "Missing artifact: $path" >&2; exit 1; }; done

PAYLOAD="$BUILD_DIR/package-root"
PKG="$BUILD_DIR/presentomb-vector-$VERSION-macos.pkg"
DMG="$ROOT/dist/presentomb-vector-$VERSION-macos.dmg"
rm -rf "$PAYLOAD"
mkdir -p "$PAYLOAD/Library/Audio/Plug-Ins/VST3" \
  "$PAYLOAD/Library/Audio/Plug-Ins/Components" "$PAYLOAD/Applications" "$ROOT/dist"
ditto "$VST3" "$PAYLOAD/Library/Audio/Plug-Ins/VST3/presentomb vector.vst3"
ditto "$AU" "$PAYLOAD/Library/Audio/Plug-Ins/Components/presentomb vector.component"
ditto "$APP" "$PAYLOAD/Applications/presentomb vector.app"

if [[ -n "${APPLE_SIGN_IDENTITY:-}" ]]; then
  codesign --force --deep --options runtime --timestamp --sign "$APPLE_SIGN_IDENTITY" "$PAYLOAD/Library/Audio/Plug-Ins/VST3/presentomb vector.vst3"
  codesign --force --deep --options runtime --timestamp --sign "$APPLE_SIGN_IDENTITY" "$PAYLOAD/Library/Audio/Plug-Ins/Components/presentomb vector.component"
  codesign --force --deep --options runtime --timestamp --sign "$APPLE_SIGN_IDENTITY" "$PAYLOAD/Applications/presentomb vector.app"
fi

PKG_ARGS=(--root "$PAYLOAD" --identifier com.presentomb.vectorvst.pkg --version "$VERSION" --install-location /)
if [[ -n "${APPLE_INSTALLER_IDENTITY:-}" ]]; then
  PKG_ARGS+=(--sign "$APPLE_INSTALLER_IDENTITY")
fi
pkgbuild "${PKG_ARGS[@]}" "$PKG"

DMG_STAGE="$BUILD_DIR/dmg-root"
rm -rf "$DMG_STAGE"
mkdir -p "$DMG_STAGE"
ditto "$PKG" "$DMG_STAGE/presentomb vector Installer.pkg"
rm -f "$DMG"
hdiutil create -volname "presentomb vector" -srcfolder "$DMG_STAGE" -ov -format UDZO "$DMG"

if [[ -n "${APPLE_NOTARY_PROFILE:-}" ]]; then
  xcrun notarytool submit "$DMG" --keychain-profile "$APPLE_NOTARY_PROFILE" --wait
  xcrun stapler staple "$DMG"
fi

echo "Created $DMG"
