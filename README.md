# macOS Resource Monitor - Project Specification

## Project Overview

Custom terminal-based resource monitor for macOS focusing on essential metrics: temperature, fan speed, CPU/memory consumption, and power usage (watts).

**Primary Goal**: Build a lightweight, focused alternative to btop/htop that displays exactly what I need without bloat.

**Secondary Goal**: Learn systems programming on macOS by working with IOKit and kernel APIs directly.

**Future Goal**: Port to Rust after completing C/C++ implementation for learning purposes.

---

## Core Requirements

### Must-Have Metrics
- **CPU Usage**: Per-core utilization, overall percentage
- **Memory**: Used/available, pressure indicators
- **Temperature**: CPU package temp, individual core temps if available
- **Fan Speed**: RPM for all fans
- **Power**: Real-time wattage (total system power, CPU power, GPU power if available)

### Nice-to-Have Metrics
- **Disk I/O**: Read/write rates
- **Network**: Upload/download rates
- **GPU**: Usage percentage, power consumption (Apple Silicon)
- **Process list**: Top consumers by CPU/memory

### UI Requirements
- Terminal-based TUI (text user interface)
- Real-time updates (1-2 second refresh)
- Clean, readable layout
- Minimal resource footprint
- Keyboard navigation optional (display-only is acceptable for v1)

---

## Technical Stack

### Language & Build
- **Primary**: C++ (C++17 or later)
- **Build system**: CMake or simple Makefile
- **Compiler**: Clang (native macOS toolchain)

### TUI Library Options (choose one)
1. **ftxui** (recommended) - Modern C++, component-based, nice API
2. **ncurses** - Battle-tested, verbose but reliable
3. **termbox** - Minimal, simple API
4. **imtui** - Immediate mode, similar to Dear ImGui

**Decision**: Start with **ftxui** for modern C++ approach. Fall back to ncurses if issues arise.

### System APIs

#### IOKit (C API)
- **Purpose**: Direct hardware sensor access (temps, fans)
- **Headers**: `<IOKit/IOKitLib.h>`, `<IOKit/ps/IOPowerSources.h>`
- **Access**: SMC (System Management Controller) sensors
- **No sudo required**: For most read-only sensor access

#### powermetrics
- **Purpose**: Power consumption data (watts)
- **Access method**: Shell execution via `popen()`, parse output
- **Format**: Request plist/JSON output for easy parsing
- **Requires sudo**: For full power metrics
- **Command**: `sudo powermetrics --samplers cpu_power,gpu_power,thermal -i 1000 -f plist`

#### sysctl (C API)
- **Purpose**: CPU info, load averages, some performance counters
- **Headers**: `<sys/sysctl.h>`
- **Functions**: `sysctlbyname()`, `sysctl()`
- **Examples**: 
  - `hw.ncpu` - CPU count
  - `vm.loadavg` - Load average
  - `hw.cpufrequency` - CPU frequency

#### libproc (C API)
- **Purpose**: Process information (optional for process list)
- **Headers**: `<libproc.h>`
- **Functions**: `proc_listpids()`, `proc_pidinfo()`

#### host_statistics / mach (C API)
- **Purpose**: CPU usage, memory stats
- **Headers**: `<mach/mach.h>`, `<mach/host_info.h>`
- **Functions**: `host_statistics()`, `host_processor_info()`
- **Use for**: Real-time CPU/memory data

---

## Architecture

### Component Structure

```
┌─────────────────────────────────────┐
│         Main Application            │
│  - Event loop                       │
│  - Refresh timer                    │
│  - Keyboard input (future)          │
└──────────┬──────────────────────────┘
           │
    ┌──────┴──────┐
    │             │
┌───▼────┐   ┌───▼─────┐
│ Data   │   │  TUI    │
│Collector│  │Renderer │
└───┬────┘   └───▲─────┘
    │             │
    │    ┌────────┘
    │    │
┌───▼────▼────────────────────┐
│   Metric Modules             │
│  - CPUMetrics               │
│  - MemoryMetrics            │
│  - ThermalMetrics           │
│  - PowerMetrics             │
│  - (FanMetrics)             │
└─────────────────────────────┘
```

### File Structure

```
macos-resource-monitor/
├── CMakeLists.txt          # Build configuration
├── README.md               # Project documentation
├── src/
│   ├── main.cpp           # Entry point, main loop
│   ├── metrics/
│   │   ├── cpu_metrics.h/cpp      # CPU usage via mach
│   │   ├── memory_metrics.h/cpp   # Memory via host_statistics
│   │   ├── thermal_metrics.h/cpp  # Temps via IOKit SMC
│   │   ├── power_metrics.h/cpp    # Power via powermetrics
│   │   └── fan_metrics.h/cpp      # Fan speeds via IOKit SMC
│   ├── ui/
│   │   ├── renderer.h/cpp         # TUI rendering logic
│   │   └── layout.h/cpp           # UI layout definitions
│   └── utils/
│       ├── iokit_helper.h/cpp     # IOKit wrapper utilities
│       └── plist_parser.h/cpp     # Parse powermetrics plist
└── build/                  # Build output directory
```

---

## Implementation Phases

### Phase 1: Foundation (Day 1-2)
**Goal**: Get basic structure working with one metric

**Tasks**:
1. Set up project structure (CMakeLists.txt, directories)
2. Integrate ftxui library (via git submodule or system install)
3. Create basic main loop with refresh timer
4. Implement ONE metric module (start with CPU - easiest)
5. Display CPU usage in simple TUI

**Success criteria**: Terminal app that shows CPU percentage, updates every second

### Phase 2: Core Metrics (Day 3-4)
**Goal**: Add all essential metrics

**Tasks**:
1. Implement memory metrics module (via host_statistics)
2. Implement thermal metrics module (IOKit SMC access)
3. Implement power metrics module (powermetrics parsing)
4. Basic error handling for each module

**Success criteria**: All four core metrics displaying simultaneously

### Phase 3: Fan & Polish (Day 5-6)
**Goal**: Complete feature set and improve UX

**Tasks**:
1. Implement fan metrics (IOKit SMC)
2. Improve TUI layout (boxes, colors, labels)
3. Add units and formatting (°C, RPM, W, %)
4. Optimize refresh rates (don't poll faster than sensors update)
5. Handle sudo requirement for powermetrics gracefully

**Success criteria**: Production-ready tool with all metrics and clean UI

### Phase 4: Optional Enhancements (Day 7+)
**Goal**: Nice-to-haves if time permits

**Tasks**:
- Add historical graphs (last 60 seconds of data)
- Process list (top CPU/memory consumers)
- Keyboard shortcuts (pause, refresh rate adjustment)
- Config file support (colors, layout preferences)
- Network I/O metrics

---

## API Usage Examples

### CPU Usage via Mach

```cpp
#include <mach/mach.h>

struct CPUUsage {
    double user_percent;
    double system_percent;
    double idle_percent;
};

CPUUsage get_cpu_usage() {
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                       (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        
        unsigned long long totalTicks = 0;
        for(int i = 0; i < CPU_STATE_MAX; i++) {
            totalTicks += cpuinfo.cpu_ticks[i];
        }
        
        // Calculate percentages...
    }
    
    return {/* ... */};
}
```

### Memory Stats via host_statistics

```cpp
#include <mach/mach.h>

struct MemoryInfo {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
};

MemoryInfo get_memory_info() {
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                         (host_info64_t)&vm_stat, &count) == KERN_SUCCESS) {
        
        uint64_t page_size;
        host_page_size(mach_host_self(), &page_size);
        
        uint64_t used = (vm_stat.active_count + 
                        vm_stat.inactive_count +
                        vm_stat.wire_count) * page_size;
        
        // Return memory info...
    }
    
    return {/* ... */};
}
```

### Temperature via IOKit SMC

```cpp
#include <IOKit/IOKitLib.h>

float read_cpu_temperature() {
    io_service_t service = IOServiceGetMatchingService(
        kIOMasterPortDefault,
        IOServiceMatching("AppleSMC")
    );
    
    if (!service) return -1.0f;
    
    // SMC key for CPU proximity temp: "TC0P" or "TC0D"
    // Need to read SMC key via IOKit property access
    // (Implementation requires SMC key reading logic)
    
    IOObjectRelease(service);
    return temperature;
}
```

### Power via powermetrics

```cpp
#include <stdio.h>
#include <string>

struct PowerMetrics {
    double package_watts;
    double cpu_watts;
    double gpu_watts;
};

PowerMetrics get_power_metrics() {
    FILE* fp = popen("sudo powermetrics --samplers cpu_power,gpu_power "
                    "-i 1000 -n 1 -f plist", "r");
    
    if (!fp) return {-1, -1, -1};
    
    // Read plist output
    std::string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        output += buffer;
    }
    pclose(fp);
    
    // Parse plist XML/JSON for power values
    // Extract: processor.package_watts, processor.cpu_watts, processor.gpu_watts
    
    return {/* parsed values */};
}
```

---

## Key Technical Challenges

### 1. SMC Access for Temps/Fans
**Challenge**: Reading SMC keys requires understanding SMC protocol
**Solution**: 
- Use existing open-source SMC reading code as reference (iStats gem, smcFanControl)
- SMC keys are 4-character codes: "TC0P" (CPU temp), "F0Ac" (Fan 0 actual speed)
- Need to use `IOConnectCallStructMethod()` to communicate with SMC

### 2. powermetrics Requires Sudo
**Challenge**: Normal users can't run powermetrics
**Options**:
- Prompt user to run app with sudo (annoying)
- Run powermetrics with sudo, cache credentials (security risk)
- Fall back gracefully: Show "N/A" for power if not root
- **Recommended**: Detect if running as root, show warning if not, disable power metrics

### 3. Parsing powermetrics Output
**Challenge**: plist/XML parsing in C++
**Options**:
- Use system `plutil` to convert plist to JSON, parse JSON
- Manual XML parsing (painful)
- Use libplist (external dependency)
- **Recommended**: Request JSON output if available, or use simple regex for specific values

### 4. Apple Silicon vs Intel Differences
**Challenge**: Different sensors, different power metrics
**Solution**:
- Detect architecture via `sysctl hw.machine`
- Conditionally access Apple Silicon-specific metrics (ANE, E/P cores)
- Test on both if possible, or focus on one architecture initially

---

## Build Instructions

### Dependencies

**System requirements**:
- macOS 11.0+ (Big Sur or later)
- Xcode Command Line Tools: `xcode-select --install`

**Libraries**:
- **ftxui**: Install via Homebrew or include as git submodule
  ```bash
  # Option 1: Homebrew
  brew install ftxui
  
  # Option 2: Git submodule
  git submodule add https://github.com/ArthurSonzogni/FTXUI.git external/ftxui
  ```

### Build Commands

```bash
# Clone/create project
mkdir macos-resource-monitor && cd macos-resource-monitor

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run
./resource-monitor

# Run with power metrics (requires sudo)
sudo ./resource-monitor
```

### CMakeLists.txt Template

```cmake
cmake_minimum_required(VERSION 3.15)
project(ResourceMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find IOKit framework
find_library(IOKIT_LIBRARY IOKit REQUIRED)
find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)

# Find or add ftxui
find_package(ftxui QUIET)
if(NOT ftxui_FOUND)
    add_subdirectory(external/ftxui)
endif()

# Source files
add_executable(resource-monitor
    src/main.cpp
    src/metrics/cpu_metrics.cpp
    src/metrics/memory_metrics.cpp
    src/metrics/thermal_metrics.cpp
    src/metrics/power_metrics.cpp
    src/ui/renderer.cpp
)

# Link libraries
target_link_libraries(resource-monitor
    ${IOKIT_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
    ftxui::screen
    ftxui::dom
    ftxui::component
)
```

---

## Testing Strategy

### Unit Testing
- Test each metric module independently
- Mock IOKit/mach responses for deterministic testing
- Verify parsing logic (especially powermetrics output)

### Integration Testing
- Run on actual hardware, verify accuracy against Activity Monitor
- Test on both Intel and Apple Silicon if possible
- Verify no memory leaks (use Instruments or valgrind)

### Edge Cases
- Running without sudo (powermetrics should fail gracefully)
- Missing sensors (older Macs might not have all SMC keys)
- Invalid powermetrics output (parsing errors)
- Rapid refresh rates (ensure no resource spikes)

---

## Success Metrics

**Functional**:
- [ ] Displays accurate CPU usage (±2% vs Activity Monitor)
- [ ] Shows correct memory consumption
- [ ] Reports CPU temperature (±1°C vs iStats)
- [ ] Displays fan speeds in RPM
- [ ] Shows power consumption in watts (when available)

**Performance**:
- [ ] Refresh rate: 1-2 seconds, smooth updates
- [ ] CPU overhead: <1% when idle
- [ ] Memory footprint: <10MB

**Code Quality**:
- [ ] No memory leaks (run with sanitizers)
- [ ] Handles errors gracefully (no crashes)
- [ ] Clean separation of concerns (metrics vs UI)
- [ ] Documented code with comments

---

## Resources & References

### Documentation
- [IOKit Fundamentals](https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/IOKitFundamentals/)
- [Mach Overview](https://web.mit.edu/darwin/src/modules/xnu/osfmk/man/)
- [sysctl man page](https://man.freebsd.org/cgi/man.cgi?sysctl(3))
- [powermetrics man page](https://www.unix.com/man-page/osx/1/powermetrics/)

### Example Projects
- [mactop](https://github.com/context-labs/mactop) - Swift resource monitor
- [iStats](https://github.com/Chris911/iStats) - Ruby SMC reader
- [asitop](https://github.com/tlkh/asitop) - Python Apple Silicon monitor
- [smcFanControl](https://github.com/hholtmann/smcFanControl) - SMC access example

### Libraries
- [ftxui](https://github.com/ArthurSonzogni/FTXUI) - Modern C++ TUI
- [ncurses](https://invisible-island.net/ncurses/) - Classic TUI library

### Learning
- "Mac OS X and iOS Internals" by Jonathan Levin
- "The Mac Hacker's Handbook" by Charlie Miller
- XNU source code: https://opensource.apple.com/source/xnu/

---

## Next Steps

1. **Immediate**: Set up project structure, get ftxui working with "Hello World"
2. **Day 1**: Implement CPU metrics module, display basic percentage
3. **Day 2**: Add memory metrics, create dual-panel layout
4. **Day 3**: Tackle IOKit SMC reading for temperature
5. **Day 4**: Integrate powermetrics for power data
6. **Day 5**: Add fan speed, polish UI
7. **Day 6**: Testing, bug fixes, documentation

---

## Conversion to Rust (Future)

When converting to Rust after C/C++ version:

**Crates to use**:
- `io-kit-sys` - IOKit bindings
- `core-foundation-sys` - Core Foundation bindings
- `sysinfo` - Cross-platform system info (might replace some custom code)
- `ratatui` - TUI library (Rust equivalent of ftxui)
- `tokio` - Async runtime (if adding async features)

**Learning opportunities**:
- FFI and `unsafe` blocks (calling C APIs)
- Ownership/borrowing with real-world constraints
- Error handling with `Result<T, E>`
- Async I/O (if polling multiple sources)

**Not a rewrite**: Port incrementally, module by module, comparing against C version.

---

## Notes

- Focus on functionality first, polish later
- Don't over-engineer: Simple > Perfect
- Test frequently on real hardware
- Document weird IOKit quirks as you find them
- Keep sudo requirements minimal (only for powermetrics)
- Prepare for Apple Silicon differences (different SMC keys)

**Remember**: This is a learning project. The goal is understanding macOS system programming, not competing with btop.
