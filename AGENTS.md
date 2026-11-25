# AGENTS.md

Guidance for AI coding agents working in **aember-monitor**.

## What this repo is

A full-screen status display for aember devices, packed as an LXC
container. It only displays: the data comes from the sibling
`../aember-stats` (JSON lines on `/run/aember-stats/stats.sock`). Siblings:
`../aember` (the C++23 init system that runs both containers),
`../aember-resources` (kernel, LXC, the SDKs; Qt is in the SDKs since
v0.1.13), `../aember-build-helper` (packs containers; a submodule here).

## Layout

```
include/aember-monitor/model.h           Sample: the JSON line, parsed leniently
include/aember-monitor/dashboard.h       Paint(): one frame, pure (QPainter)
include/aember-monitor/monitor-window.h  QRasterWindow + QLocalSocket client
src/main.cpp                             platform (linuxfb/offscreen), font dir, flags
tests/test_monitor.cpp                   Qt Test: parser, drawing, socket client
fuzz/                                    libFuzzer target (host clang, Ubuntu's Qt)
assets/fonts/                            DejaVu Sans (the only font) + its license
aember-container.json                    container manifest (aember-build-helper)
aember-cli/run.py                        `aember` CLI: sdk, build, pack, install, test, all
                                         (aember-build-helper's project-cli/, told NAME etc.)
```

## Rules

- **Qt stays minimal**: Core, Gui and Network from the SDK's qtbase (built
  with `gui linuxfb no-opengl png zlib`). No widgets, QML, OpenGL, fontconfig
  or input; nothing outside qtbase. A feature that needs more is a change to
  aember-resources' `kas/recipes/sdk.yml` first.
- **No moc in the app**: connections use lambdas, no `Q_OBJECT` outside the
  tests (the test class needs it; AUTOMOC is on only there).
- `Paint()` is pure (sample + size → pixels) and scales with the frame
  height; tests render it into QImages at 320x240, 800x480 and 1920x1080.
  Keep drawing out of the window class.
- The JSON format is aember-stats' (`version` 1). Parse leniently (missing
  fields keep defaults), reject other versions, never crash on bad input.
  Format changes go into both repos together.
- The socket directory comes from the LXC pre-start hook + bind mount in
  `aember-container.json`, the same as aember-stats' (read-only here).
- The CLI is shared with aember-stats, aember-auth and aember-fleet: it
  lives in the aember-build-helper submodule (`project-cli/`, stdlib
  Python, never pip-installed); `aember-cli/run.py` holds the differences
  (`NAME`, `RESOURCES_VERSION`, `TESTS = "sdk"`). Change the CLI there.
- WSL crashes on full-core builds: the CLI uses 2/3 of the CPUs.

## Builds and tests

- Cross builds source the SDK's environment-setup and use its
  `OEToolchainConfig.cmake` (meta-qt6 adds Qt's host tools there).
- `aember-cli/run.py test`: the Qt tests built with the x64
  SDK and run through the SDK's loader (`QT_QPA_PLATFORM=offscreen`, the
  sysroot's plugins).
- Before calling a change done: `run.py test`, a cross build + pack, and the
  container in aember with aember-stats (QEMU boot; `--screenshot` shows the
  frame).
- C++23, `-Werror`, clang-format; Python black, 88 columns.
- Mutation testing: `aember-cli/run.py mutate [-n N] [-f FILE]` (the shared
  mutator from aember-build-helper: one operator flipped at a time in
  `src/*.cpp` except main.cpp, the unit tests rebuilt and run; report in
  `build/mutation/`). Weekly in CI (`.github/workflows/mutation.yml`).
  Score 2026-11-25: 40.4% -> 42.1% (57 mutants). Most survivors are
  painting code (colours, fonts, layout) that no unit test looks at: the
  screenshots and the QEMU demo are its check.
- Fuzzing: `fuzz/run.sh [SECONDS]` (CI: 60 s, the `fuzz` job) feeds any
  line to `ParseSample()` and draws a frame of what parsed (`fuzz_sample`).
  It builds with clang 18 against Ubuntu's Qt 6 (`qt6-base-dev`), not the
  SDK. `fuzz/lsan.supp` hides fontconfig's font cache, which isn't ours. A
  crash gets a fix, a `Regression:` test and its input in
  `fuzz/corpus/sample/`.
- A test that pins down a fixed bug starts with a comment
  `Regression: <the bug>` (`grep -rn "Regression:"` lists them). Every bug
  fix gets one.

## Working agreements

- Branch off `main`, merge through a PR; commit, push and merge only when
  asked. The GitHub repository is created by the user.
