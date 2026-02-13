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

private:
    std::vector<CoreTicks> prev_ticks_;
};
