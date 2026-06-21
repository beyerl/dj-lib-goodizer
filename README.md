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

### Web build (WebAssembly / Chrome)

The Qt UI also compiles to WebAssembly so it runs in a browser — used as an
automatable target for end-to-end tests and runtime-bug diagnosis. Built in CI
(`.github/workflows/wasm.yml`) with Qt for WebAssembly + Emscripten:

```sh
"$QT_WASM_ROOT/bin/qt-cmake" -S . -B build-wasm -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DDJ_BUILD_APP=ON -DDJ_BUILD_TESTS=OFF
cmake --build build-wasm --parallel
# serve build-wasm/app/ and open index.html in Chrome
```

In the browser there is no native folder picker or filesystem, so *Import
Folder…* is replaced by *Load Demo Library* (synthetic in-memory tracks). The
desktop build is unchanged. A small JS bridge (`app/WasmBridge.cpp`) mirrors UI
state to `window.__djState` and accepts commands via `window.__djCmd`, which the
Playwright tests under `tests/e2e/` drive.

## Status

- **v0.11.0** — Playwright end-to-end tests (`tests/e2e/`) covering the major
  browser flows — app load, *Load Demo Library*, track selection/detail,
  dashboard summary, audit tab, profile switching, and About — run in CI against
  the WebAssembly build via the `window.__djState`/`window.__djCmd` bridge.
- **v0.10.x** — WebAssembly build target: the Qt app compiles to WASM and runs
  in Chrome (CI job `wasm`), with a browser-only *Load Demo Library* path and a
  `window.__djState`/`window.__djCmd` test bridge.
- **v0.9.0** — Milestone 9: packaging — CMake install rules, CI artifacts on
  tags (windeployqt portable zip / Linux tarball), and production Inno Setup +
  AppImage scripts under `packaging/`.
- **v0.8.0** — Milestone 8: opt-in perceptual ops — linked-stereo compressor
  and mid/side width + L/R balance correction (both PerceptualAltering), with a
  mono-safety cross-check that predicts post-widening correlation (FR-WID-7).
- **v0.7.0** — Milestone 7: Qt UI — database-backed Library View with
  three-state flag colouring, Import Folder → background scan/analyze/persist,
  Track Detail metrics panel, profile switching, Dashboard + Audit tabs.
  (Waveform/goniometer/preview-playback visualizations deferred.)
- **v0.6.0** — Milestone 6: processing engine — standardizing chain (silence
  trim → gain-only normalize → resample → encode), dry-run planning, true-peak
  gain limiting, and an audit-log repository.
- **v0.5.x** — Milestone 5: job scheduler — dependency-free worker pool with
  cooperative pause/resume/cancel (`JobControl`), dynamic `parallelFor`, and a
  headless `AnalysisBatch` runner with progress + ETA.
- **v0.4.0** — Milestone 4: SQLite data store — vendored amalgamation,
  WAL + migrations, repositories for tracks/analysis-results/profiles, indexed
  metric columns, default-profile seeding.
- **v0.3.0** — Milestone 3: analysis engine — single-decode fan-out pipeline
  feeding silence, loudness/dynamics (BS.1770 K-weighting + gating, crest, DR),
  mono-correlation/fold-down, and stereo-width/balance analyzers.
- **v0.2.0** — Milestone 2: audio I/O — dependency-free WAV decode/encode,
  windowed-sinc resampler, TPDF dither; FFmpeg broad-format decode backend
  behind `DJ_WITH_AUDIO_BACKENDS` (verified by a dedicated Linux CI job).
- **v0.1.x** — Milestone 1: buildable skeleton, domain model, Qt shell, CI.
