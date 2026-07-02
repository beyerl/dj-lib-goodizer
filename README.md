# DJ Library Goodizer

Desktop tool that prepares heterogeneous DJ music libraries so tracks behave
consistently during playback and mixing. It analyzes and conditions audio
across five areas — format/sample-rate standardization, silence trimming,
dynamic-range consistency, mono compatibility, and stereo width/balance — and
writes standardized, non-destructive output with a full audit log.

Native C++20 / Qt 6 (Windows 10/11 + Linux x64). Audio I/O via FFmpeg.

## Layout

| Path | Contents |
|------|----------|
| `core/` | `djcore` — Qt-free domain library (audio, analysis, model, processing, db, jobs, profile). Builds and tests without Qt. |
| `cli/`  | `djcli` — Qt-free end-to-end harness: import → analyze → apply profile → write files → audit. |
| `app/`  | Qt Widgets GUI (thin layer over `djcore`). |
| `tests/`| GoogleTest unit + end-to-end tests. |

## Build

Requires CMake ≥ 3.21, Ninja, a C++20 compiler. Qt 6 is required only for the
GUI (`DJ_BUILD_APP`).

Core + CLI + tests (no Qt):

```
cmake -S . -B build -G Ninja -DDJ_BUILD_APP=OFF -DDJ_BUILD_CLI=ON -DDJ_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Full build including the Qt GUI: drop `-DDJ_BUILD_APP=OFF` (needs Qt 6 Widgets).

Broad-format audio (FFmpeg/soxr/TagLib) is enabled with
`-DDJ_WITH_AUDIO_BACKENDS=ON` (Milestone 4+).

## Options

| Option | Default | Purpose |
|--------|---------|---------|
| `DJ_BUILD_APP` | ON | Build the Qt GUI application |
| `DJ_BUILD_CLI` | ON | Build the headless `djcli` tool |
| `DJ_BUILD_TESTS` | ON | Build unit/e2e tests |
| `DJ_WITH_AUDIO_BACKENDS` | OFF | Build with FFmpeg/soxr/libebur128/TagLib |

## Status

In active development, milestone by milestone (one minor version per milestone).
See the specification and the plan for the roadmap.
