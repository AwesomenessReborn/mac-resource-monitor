#pragma once

#include <vector>
#include <cstdint>

struct CoreTicks {
    uint64_t user;
    uint64_t system;
    uint64_t idle;
    uint64_t nice;
};

struct CPUUsage {
    double overall = 0.0;
    std::vector<double> per_core;
};

class CpuMetrics {
public:
    CpuMetrics();
    // Returns CPU usage since the last call (delta-based).
    CPUUsage sample();
    // Labels like "P1", "P2", "E1", "E2" on Apple Silicon; "Core 0" etc. on Intel.
    const std::vector<std::string>& core_labels() const { return labels_; }

private:
    std::vector<CoreTicks> prev_ticks_;
    std::vector<std::string> labels_;
};
