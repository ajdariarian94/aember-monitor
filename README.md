# aember-monitor

A small full-screen status display for [aember](https://github.com/ajdariarian94/aember)
devices, in its own LXC container. It shows what
[aember-stats](https://github.com/ajdariarian94/aember-stats) collects: CPU,
memory, network rates, disk, temperatures, and aember itself (online state,
IP, PID 1, every service and container with its state).

It replaces aember's old monitor container (a 162 MB prebuilt Qt 6 for x64
only). This one is built with the aember-resources SDKs for **every aember
target** (x64, aarch64, arm32, riscv64 and their musl variants) against a
minimal Qt 6 (Core, Gui, Network; raster painting, no OpenGL, no widgets).

```
 aember                                     ● online   10.0.2.15   up 1d 2h
 ┌ CPU ──────────────┐ ┌ MEMORY ───────────┐ ┌ NETWORK ──────────────────┐
 │ 42%               │ │ 25%               │ │ eth0      ↓1.5 KB/s ↑512 B/s│
 │ ████████░░░░░░░░  │ │ ████░░░░░░░░░░░░  │ │                           │
 │ 4 cores · load .52│ │ 375 MB free of 500│ │                           │
 └───────────────────┘ └───────────────────┘ └───────────────────────────┘
 ┌ SERVICES ─────────────────────────────┐ ┌ SYSTEM ────────────────────┐
 │ ● aember-netd     ● aember-stats ▣    │ │ PID 1      3.8 MB · 7 thr  │
 │ ● aember-containerd                   │ │ Disk           900 KB free │
 └───────────────────────────────────────┘ └────────────────────────────┘
```

## How it works

- aember-stats writes one JSON line per second to
  `/run/aember-stats/stats.sock`. Both containers bind that directory from
  the device; the monitor's bind is read-only. The monitor reconnects by
  itself, so the two containers start in any order.
- The display: Qt's `linuxfb` platform on `/dev/fb0` (bound in when the
  device has one). Without a framebuffer it draws offscreen, which is how the
  QEMU tests and `--screenshot` work. `QT_QPA_PLATFORM` overrides both.
- One font (DejaVu Sans) ships in the container; there is no fontconfig.
  glibc targets also get the sysroot's locale archive for `C.UTF-8`
  (Qt wants a UTF-8 locale; musl has it built in).

```
aember-monitor [--socket PATH] [--screenshot FILE.png [--quit]]
```

`--screenshot` saves the first frame with data as a PNG (and `--quit` exits
afterwards).

## Build

In the dev container (`.devcontainer/`, aember's dev image):

```bash
git submodule update --init                     # tools/aember-build-helper
aember-cli/run.py sdk   -a aarch64-poky-linux   # aember-resources SDK (v0.1.13+, with Qt)
aember-cli/run.py build -a aarch64-poky-linux   # → build/stage/<arch>/
aember-cli/run.py pack  -a aarch64-poky-linux   # → build/aember-out/<arch>/aember-monitor/
aember-cli/run.py install -a aarch64-poky-linux ../aember   # into aember's overlay
aember-cli/run.py test                          # unit tests (x64 SDK)
```

`AEMBER_RESOURCES_DIR` reuses SDKs installed elsewhere (e.g. aember's
`build/aember-resources`), and `AEMBER_BUILD_JOBS` caps the build jobs.
The unit tests are built with the x64 SDK and run on the host through the
SDK's loader with Qt's offscreen platform. They cover the parser, the
drawing at three screen sizes, and the socket client including reconnects.

## Fuzzing

`fuzz/fuzz_sample` feeds any line to the parser and draws a frame of what
it parsed (libFuzzer, ASan + UBSan). On Ubuntu 24.04, with `clang-18 llvm-18
libclang-rt-18-dev cmake ninja-build qt6-base-dev` installed: `fuzz/run.sh`
(60 s, as CI does); crashing inputs land in `build/fuzz/crashes/sample/`.

## License

See [LICENSE](LICENSE). DejaVu Sans: see `assets/fonts/LICENSE-DejaVu.txt`.
