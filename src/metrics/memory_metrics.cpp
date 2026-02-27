#include "metrics/memory_metrics.h"

#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/types.h>

MemoryInfo MemoryMetrics::sample() {
    MemoryInfo info;

    // ── Total physical RAM ────────────────────────────────────────────────
    {
        uint64_t mem = 0;
        size_t sz = sizeof(mem);
        sysctlbyname("hw.memsize", &mem, &sz, nullptr, 0);
        info.total_bytes = mem;
    }

    // ── VM statistics ────────────────────────────────────────────────────
    {
        vm_statistics64_data_t vmstat{};
        mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
        if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                              reinterpret_cast<host_info64_t>(&vmstat),
                              &count) == KERN_SUCCESS) {
            uint64_t page = vm_kernel_page_size;
            uint64_t active     = vmstat.active_count           * page;
            uint64_t wired      = vmstat.wire_count              * page;
            uint64_t compressor = vmstat.compressor_page_count   * page;
            uint64_t inactive   = vmstat.inactive_count          * page;
            uint64_t free       = vmstat.free_count              * page;

            info.active_bytes     = active;
            info.wired_bytes      = wired;
            info.compressed_bytes = compressor;
            info.inactive_bytes   = inactive;
            info.used_bytes       = active + wired + compressor;
            info.free_bytes       = free;
        }
    }

    // ── Swap usage ────────────────────────────────────────────────────────
    {
        struct xsw_usage sw{};
        size_t sz = sizeof(sw);
        if (sysctlbyname("vm.swapusage", &sw, &sz, nullptr, 0) == 0) {
            info.swap_total_bytes = sw.xsu_total;
            info.swap_used_bytes  = sw.xsu_used;
        }
    }

    if (info.total_bytes > 0)
        info.pressure_percent = 100.0 * info.used_bytes / info.total_bytes;

    // ── Kernel memory pressure level ──────────────────────────────────────
    {
        int level = -1;
        size_t sz = sizeof(level);
        if (sysctlbyname("kern.memorystatus_level", &level, &sz, nullptr, 0) == 0)
            info.kern_pressure = level;
    }

    if (info.kern_pressure >= 0) {
        if (info.kern_pressure >= 75)
            info.pressure_level = MemoryPressureLevel::Normal;
        else if (info.kern_pressure >= 25)
            info.pressure_level = MemoryPressureLevel::Warning;
        else
            info.pressure_level = MemoryPressureLevel::Critical;
    } else if (info.total_bytes > 0) {
        double available_ratio =
            double(info.inactive_bytes + info.free_bytes) / info.total_bytes;
        if (available_ratio >= 0.25)
            info.pressure_level = MemoryPressureLevel::Normal;
        else if (available_ratio >= 0.10)
            info.pressure_level = MemoryPressureLevel::Warning;
        else
            info.pressure_level = MemoryPressureLevel::Critical;
    }

    return info;
}
