#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include "metrics/cpu_metrics.h"
#include "metrics/fan_metrics.h"
#include "metrics/memory_metrics.h"
#include "metrics/power_metrics.h"
#include "metrics/thermal_metrics.h"

static std::string fmt_bytes(uint64_t bytes) {
    double gb = bytes / 1073741824.0;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << gb << " GB";
    return ss.str();
}

static std::string fmt_w(float w) {
    if (w < 0) return "N/A";
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << w << "W";
    return ss.str();
}

int main() {
    CpuMetrics cpu;
    MemoryMetrics mem;
    FanMetrics fans;
    ThermalMetrics thermal;
    PowerMetrics power;

    // Prime CPU delta (constructor already called sample() once, just sleep and
    // sample again so we get a non-zero reading on the first print).
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ── CPU ──────────────────────────────────────────────────────────────────
    CPUUsage cpu_u = cpu.sample();
    const auto& labels = cpu.core_labels();

    std::cout << "=== CPU ===\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Overall: " << cpu_u.overall << "%\n";
    for (size_t i = 0; i < cpu_u.per_core.size(); ++i) {
        std::string lbl = (i < labels.size()) ? labels[i] : "Core " + std::to_string(i);
        std::cout << "  " << std::left << std::setw(4) << lbl
                  << " " << cpu_u.per_core[i] << "%\n";
    }

    // ── Memory ───────────────────────────────────────────────────────────────
    MemoryInfo mem_i = mem.sample();
    double used_pct = mem_i.total_bytes > 0
        ? 100.0 * mem_i.used_bytes / mem_i.total_bytes : 0.0;

    auto pressure_label = [](MemoryPressureLevel lvl) -> const char* {
        switch (lvl) {
            case MemoryPressureLevel::Normal:   return "Normal   [GREEN]";
            case MemoryPressureLevel::Warning:  return "Warning  [YELLOW]";
            case MemoryPressureLevel::Critical: return "Critical [RED]";
        }
        return "Unknown";
    };

    std::cout << "\n=== Memory ===\n";
    std::cout << "Used:     " << fmt_bytes(mem_i.used_bytes)
              << " / " << fmt_bytes(mem_i.total_bytes)
              << " (" << std::fixed << std::setprecision(2) << used_pct << "%)\n";
    std::cout << "Free:     " << fmt_bytes(mem_i.free_bytes) << "\n";
    if (mem_i.swap_total_bytes > 0) {
        std::cout << "Swap:     " << fmt_bytes(mem_i.swap_used_bytes)
                  << " / " << fmt_bytes(mem_i.swap_total_bytes) << "\n";
    }
    std::cout << "Pressure: " << pressure_label(mem_i.pressure_level);
    if (mem_i.kern_pressure >= 0)
        std::cout << "  (kern=" << mem_i.kern_pressure << ")";
    std::cout << "\n";
    std::cout << "  Active:     " << fmt_bytes(mem_i.active_bytes) << "\n";
    std::cout << "  Wired:      " << fmt_bytes(mem_i.wired_bytes) << "\n";
    std::cout << "  Compressed: " << fmt_bytes(mem_i.compressed_bytes) << "\n";
    std::cout << "  Inactive:   " << fmt_bytes(mem_i.inactive_bytes) << " (reclaimable)\n";

    // ── Fans ─────────────────────────────────────────────────────────────────
    FanInfo fan_i = fans.sample();
    std::cout << "\n=== Fans ===\n";
    if (fan_i.count == 0) {
        std::cout << "  No fans detected\n";
    }
    for (int i = 0; i < fan_i.count; ++i) {
        std::cout << "  Fan " << i << ": "
                  << std::fixed << std::setprecision(0) << fan_i.rpm[i] << " RPM"
                  << "  (min " << fan_i.min_rpm[i]
                  << ", max " << fan_i.max_rpm[i] << ")\n";
    }

    // ── Thermal ──────────────────────────────────────────────────────────────
    ThermalInfo therm_i = thermal.sample();
    std::cout << "\n=== Thermal ===\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  CPU: " << therm_i.cpu_temp << " °C";
    if (therm_i.has_gpu_temp)
        std::cout << "  GPU: " << therm_i.gpu_temp << " °C";
    std::cout << "\n";

    // ── Power ────────────────────────────────────────────────────────────────
    PowerInfo pow_i = power.sample();
    std::cout << "\n=== Power ===\n";
    if (!pow_i.available) {
        std::cout << "  (requires sudo)\n";
    } else {
        std::cout << "  Package: " << fmt_w(pow_i.package_w)
                  << "  CPU: " << fmt_w(pow_i.cpu_w)
                  << "  GPU: " << fmt_w(pow_i.gpu_w)
                  << "  ANE: " << fmt_w(pow_i.ane_w) << "\n";
    }

    return 0;
}
