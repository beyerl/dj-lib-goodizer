# Project Choices

A record of the decisions made during the planning and implementation of the
DJ Library Preparation Software.

## Choices selected in response to questions

These were chosen explicitly when asked to pick between options during planning:

| Question | Choice | Notes |
|----------|--------|-------|
| **Tech stack** | **C++/Qt (per spec)** | Native cross-platform stack exactly as the spec's reference stack prescribes — Qt Widgets UI, SQLite store. (Alternatives offered: Python POC, Rust, Web/Electron+TS.) |
| **Scope** | **Full v1 architecture** | Complete architecture for all five feature areas and all NFRs (50k tracks, batch scheduler, audit log, named profiles, Win/Linux parity). (Alternative offered: POC vertical slice.) |
| **Audio I/O** | **FFmpeg/libav** | Broadest format coverage (MP3, AAC, OGG, ALAC, FLAC, WAV, AIFF); LGPL build, bundled. (Alternatives offered: libsndfile + extras, decide-in-plan.) |

## Other explicit directions given

Decisions/instructions issued directly in messages (not multiple-choice):

- **Proceed past planning into implementation** — after producing the
  architecture plan, switch to implementation mode and start building.
- **Add a GitHub Actions build workflow** for the project.
- **Commit and push as version 0.1.0** once the initial work was done.
- **Implement all milestones one by one**, creating a commit and a **minor
  version** per milestone, and **patch versions** for fixes, **until the build
  is green**.
- **Use pushes to trigger the CI build pipeline** as the validation loop.
- **Use Playwright** to check the CI build status.
- **Save these choices** to a markdown file in the current folder (this file).
