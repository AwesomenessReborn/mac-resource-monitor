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
            uint64_t active    = vmstat.active_count    * page;
            uint64_t wired     = vmstat.wire_count       * page;
            uint64_t compressor= vmstat.compressor_page_count * page;
            uint64_t free      = vmstat.free_count       * page;

            info.used_bytes = active + wired + compressor;
            info.free_bytes = free;
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

    return info;
}
