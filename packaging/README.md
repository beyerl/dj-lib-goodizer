# Packaging

Cross-platform distribution per the architecture plan (§6.2, M9).

## CI artifacts

The `build` workflow produces downloadable artifacts on **tag pushes** (`v*`):

- **Windows** — a portable build via `windeployqt` (Qt runtime bundled next to
  the executable), zipped as `dj-lib-goodizer-windows-x64.zip`.
- **Linux** — an installed tree tarball `dj-lib-goodizer-linux-x64.tar.gz`.

## Production installers (run locally / in a release pipeline)

These need extra tooling not provisioned in the lightweight CI build:

- **Windows installer** — `packaging/windows/installer.iss` builds a setup
  executable with [Inno Setup](https://jrsoftware.org/isinfo.php):
  ```
  windeployqt --release dist\dj-lib-goodizer\dj-lib-goodizer.exe
  iscc packaging\windows\installer.iss
  ```
- **Linux AppImage** — `packaging/linux/build-appimage.sh` bundles Qt with
  [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) + its Qt plugin:
  ```
  ./packaging/linux/build-appimage.sh build
  ```

FFmpeg (LGPL) is bundled with the application where the audio backends are
enabled, so playback/decoding works without a system-installed codec (§6.2).
