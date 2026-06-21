# DJ Library Goodizer

Desktop DJ **library preparation** software — standardizes, analyzes, and
conditions a heterogeneous audio library so tracks behave consistently during
playback, mixing, and mono/broadcast fold-down. It is a *pre-performance* tool
that feeds clean, standardized audio into DJ software (rekordbox, Serato, Engine
DJ, Mixxx); it is **not** a real-time mixer.

See `DJ_Library_Prep_Software_Specification.docx` for the full product spec.

## Feature areas (v1)

| Area | Capability |
|------|-----------|
| A | Format & sample-rate standardization |
| B | Trim silence / consistent start-stop points |
| C | Dynamic-range consistency (loudness *and* dynamics) |
| D | Mono compatibility checking |
| E | Stereo width / balance consistency |

All analysis is non-destructive; corrective processing writes to a parallel
output library by default.

## Tech stack

- **C++20 / Qt 6** (Qt Widgets) — cross-platform native (Windows 10/11, Linux x64)
- **FFmpeg/libav** decode-encode, **libsoxr** resampling, **libebur128**
  loudness, **TagLib** tags *(Milestone 2+)*
- **SQLite** library/metrics/audit store
- **CMake + vcpkg**; **GoogleTest** for tests

## Architecture

`core/` is a Qt-free, headless-testable domain library; `app/` is the Qt UI on
top of it. See the architecture plan for the component breakdown and milestones.

```
core/   # PcmBuffer, FormatInfo, analysis/processing interfaces, model, profiles
app/    # Qt UI (Library View, Track Detail, Dashboard, Profile Editor, Batch, Audit)
tests/  # GoogleTest unit/integration tests + reference corpus
```

## Build

Requires CMake ≥ 3.21, a C++20 compiler, and Qt 6 (for the app).

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

To build only the headless core + tests (no Qt):

```sh
cmake -S . -B build -DDJ_BUILD_APP=OFF
cmake --build build --parallel
```

The audio backends (FFmpeg/libebur128/soxr/TagLib) are gated behind
`-DDJ_WITH_AUDIO_BACKENDS=ON` and land in Milestone 2.

## Status

**v0.1.0** — Milestone 1: buildable skeleton, domain model, Qt shell, CI.
