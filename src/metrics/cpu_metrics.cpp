#include "metrics/cpu_metrics.h"

#include <mach/mach.h>
#include <mach/processor_info.h>
#include <sys/sysctl.h>

// Builds per-core labels. On Apple Silicon, perflevel0 = P-cores (come first
// in host_processor_info), perflevel1 = E-cores. Falls back to "Core N".
static std::vector<std::string> build_labels(natural_t cpu_count) {
    int p_count = 0, e_count = 0;
    size_t sz = sizeof(int);
    bool ok = sysctlbyname("hw.perflevel0.logicalcpu", &p_count, &sz, nullptr, 0) == 0
           && sysctlbyname("hw.perflevel1.logicalcpu", &e_count, &sz, nullptr, 0) == 0;

    std::vector<std::string> labels(cpu_count);
    if (ok && static_cast<natural_t>(p_count + e_count) == cpu_count) {
        // host_processor_info enumerates E-cores first on Apple Silicon.
        for (int i = 0; i < e_count; ++i)
            labels[i] = "E" + std::to_string(i + 1);
        for (int i = 0; i < p_count; ++i)
            labels[e_count + i] = "P" + std::to_string(i + 1);
    } else {
        for (natural_t i = 0; i < cpu_count; ++i)
            labels[i] = "Core " + std::to_string(i);
    }
    return labels;
}

CpuMetrics::CpuMetrics() {
    // Prime prev_ticks_ so the first real sample() returns a valid delta.
    sample();
}

CPUUsage CpuMetrics::sample() {
    natural_t cpu_count = 0;
    processor_info_array_t info_array;
    mach_msg_type_number_t info_count;

    kern_return_t kr = host_processor_info(mach_host_self(),
                                           PROCESSOR_CPU_LOAD_INFO,
                                           &cpu_count,
                                           &info_array,
                                           &info_count);
    if (kr != KERN_SUCCESS) {
        return {};
    }

    // Copy raw ticks out of the mach-allocated array.
    std::vector<CoreTicks> curr(cpu_count);
    auto* load = reinterpret_cast<processor_cpu_load_info_t>(info_array);
    for (natural_t i = 0; i < cpu_count; ++i) {
        curr[i] = {
            load[i].cpu_ticks[CPU_STATE_USER],
            load[i].cpu_ticks[CPU_STATE_SYSTEM],
            load[i].cpu_ticks[CPU_STATE_IDLE],
            load[i].cpu_ticks[CPU_STATE_NICE],
        };
    }
    vm_deallocate(mach_task_self(),
                  (vm_address_t)info_array,
                  (vm_size_t)info_count * sizeof(integer_t));

    CPUUsage result;
    result.per_core.resize(cpu_count, 0.0);

    if (prev_ticks_.size() == cpu_count) {
        double total_used = 0, total_all = 0;
        for (natural_t i = 0; i < cpu_count; ++i) {
            uint64_t d_user   = curr[i].user   - prev_ticks_[i].user;
            uint64_t d_system = curr[i].system - prev_ticks_[i].system;
            uint64_t d_idle   = curr[i].idle   - prev_ticks_[i].idle;
            uint64_t d_nice   = curr[i].nice   - prev_ticks_[i].nice;
            uint64_t d_total  = d_user + d_system + d_idle + d_nice;

            if (d_total > 0) {
                result.per_core[i] = 100.0 * (d_user + d_system + d_nice) / d_total;
            }
            total_used += d_user + d_system + d_nice;
            total_all  += d_total;
        }
        if (total_all > 0) {
            result.overall = 100.0 * total_used / total_all;
        }
    }

    if (labels_.empty())
        labels_ = build_labels(cpu_count);

    prev_ticks_ = std::move(curr);
    return result;
}
