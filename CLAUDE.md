# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Status

Pre-implementation stage. Only a project specification (README.md) exists. No source code has been written yet.

The planned target is macOS 11.0+ (Big Sur), C++17, using ftxui for the TUI and native macOS APIs for metrics.

## Build Setup

**Prerequisites**:
```bash
xcode-select --install
brew install ftxui   # Or use git submodule: git submodule add https://github.com/ArthurSonzogni/FTXUI.git external/ftxui
```

**Build**:
```bash
mkdir build && cd build
cmake ..
make
./resource-monitor
sudo ./resource-monitor  # Required for power metrics (powermetrics)
```

The planned `CMakeLists.txt` links against `IOKit`, `CoreFoundation`, and `ftxui::screen`, `ftxui::dom`, `ftxui::component`. See README.md for the full CMakeLists.txt template.

## Architecture

The app has a clear separation between metric collection and rendering:

```
main.cpp  →  DataCollector  →  Metric Modules (src/metrics/)
                            ↘
                              TUI Renderer (src/ui/)
```

**Metric modules** (`src/metrics/`): Each module is independent and collects one category of data:
- `cpu_metrics` — CPU usage via `host_statistics()` / `host_processor_info()` from `<mach/mach.h>`
- `memory_metrics` — Memory stats via `host_statistics64()` with `HOST_VM_INFO64`
- `thermal_metrics` — CPU temperatures via IOKit SMC (key `"TC0P"` / `"TC0D"`)
- `power_metrics` — Power in watts by spawning `sudo powermetrics --samplers cpu_power,gpu_power -i 1000 -n 1 -f plist` via `popen()` and parsing the plist output
- `fan_metrics` — Fan RPM via IOKit SMC (key `"F0Ac"` for fan 0 actual speed)

**UI layer** (`src/ui/`):
- `renderer` — ftxui-based rendering, polls metric modules, redraws at 1–2s intervals
- `layout` — ftxui component/element layout definitions

**Utilities** (`src/utils/`):
- `iokit_helper` — Wraps `IOConnectCallStructMethod()` for SMC key reads
- `plist_parser` — Parses powermetrics plist/XML output (or converts via `plutil` to JSON first)

## Key Technical Notes

- **SMC access**: Uses `IOConnectCallStructMethod()` targeting the `"AppleSMC"` IOService. SMC keys are 4-char codes. Reference [smcFanControl](https://github.com/hholtmann/smcFanControl) for the SMC protocol implementation.
- **powermetrics requires root**: Detect `getuid() == 0` at startup; show `N/A` for power metrics if not root rather than crashing or prompting.
- **Apple Silicon vs Intel**: Detect architecture via `sysctlbyname("hw.machine", ...)`. Apple Silicon has different SMC keys and exposes E/P-core split and ANE power. Start with one architecture if needed.
- **plist parsing**: Prefer requesting JSON from powermetrics if the macOS version supports it, or pipe through `plutil -convert json -` to avoid manual XML parsing.
- **Refresh rate**: Metrics should not poll faster than sensors actually update (~1s). CPU ticks are delta-based — store previous tick counts between samples.

## Future Rust Port

Planned crates: `io-kit-sys`, `core-foundation-sys`, `sysinfo`, `ratatui`, `tokio`. Port module-by-module against the working C++ version.
