presentomb vector

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
- Windows installer with optional standalone app component
- Real-time-safe audio thread with no heap allocation in `processBlock`

## Architecture

- `DSO2AudioProcessor` owns APVTS parameters, audio pass-through, metering, and scope capture.
- `ScopeBuffer` stores stereo input in a preallocated circular buffer using atomic write-position publishing.
- `DSO2AudioProcessorEditor` snapshots audio on the UI timer and renders the vectorscope natively.
- Installer packaging lives outside DSP/UI code, so release tooling does not affect plugin runtime.
