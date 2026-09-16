# presentomb vector

`presentomb vector` is a JUCE stereo vectorscope VST3 and standalone app for real-time phase, width, balance, and headroom monitoring.

It renders incoming stereo audio as a CRT-style XY oscilloscope with phosphor glow, persistence, visual clipping, and real-time-safe DSP built for music production.

## Features

- Stereo XY vectorscope display
- CRT-inspired phosphor trace and glow
- Visual persistence
- Visual clipping behavior for hot input
- Auto-gain option
- Adjustable gain, beam width, phosphor, persistence, and intensity
- VST3 plugin build
- Standalone desktop app build
- Audio Unit build on macOS
- Windows installer with optional standalone app component
- Real-time-safe audio thread with no heap allocation in `processBlock`

## Architecture

- `DSO2AudioProcessor` owns APVTS parameters, audio pass-through, metering, and scope capture.
- `ScopeBuffer` stores stereo input in a preallocated circular buffer using atomic write-position publishing.
- `DSO2AudioProcessorEditor` snapshots audio on the UI timer and renders the vectorscope natively.
- Installer packaging lives outside DSP/UI code, so release tooling does not affect plugin runtime.

## Build

Requirements:

- CMake 3.22+
- Visual Studio 2022 C++ toolchain
- JUCE 8.0.8, either available through `JUCE_DIR` or fetched by CMake

```powershell
cmake -S . -B build-gon -DCMAKE_BUILD_TYPE=Release
cmake --build build-gon --config Release --target PresentombDSO2_VST3
cmake --build build-gon --config Release --target PresentombDSO2_Standalone
```

If using a local JUCE checkout:

```powershell
cmake -S . -B build-gon -DJUCE_DIR=path\to\JUCE
```

## Installer

The Windows installer uses Inno Setup 6.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\package\build-installer.ps1
```

Output:

```text
dist\presentomb-vector-0.1.0-windows.exe
```

## macOS package

Run on macOS with Xcode command-line tools installed:

```bash
bash package/build-macos.sh
```

This builds universal Apple Silicon/Intel VST3, Audio Unit, and standalone app bundles. It creates a `.pkg` that installs them into standard system locations, then wraps it in `dist/presentomb-vector-0.1.0-macos.dmg`. Set `APPLE_SIGN_IDENTITY`, `APPLE_INSTALLER_IDENTITY`, and `APPLE_NOTARY_PROFILE` for signed, notarized distribution. Unsigned builds require explicit Gatekeeper approval.

Installer behavior:

- VST3 install is required.
- Standalone app install is optional.
- Desktop shortcut is optional when standalone app is selected.

## Repo Layout

```text
assets/      Font and bundled binary assets
installer/   Inno Setup installer script
package/     Release packaging helper scripts
release/     GitHub release staging notes
src/         JUCE processor, editor, scope buffer, and state code
```

## Release

Build installer, then attach this file to a GitHub release:

```text
dist\presentomb-vector-0.1.0-windows.exe
```

Recommended release title:

```text
presentomb vector 0.1.0
```
