#pragma once

#include <cstdint>

struct MemoryInfo {
    uint64_t total_bytes       = 0;
    uint64_t used_bytes        = 0;
    uint64_t free_bytes        = 0;
    uint64_t swap_used_bytes   = 0;
    uint64_t swap_total_bytes  = 0;
    double   pressure_percent  = 0.0;
};

class MemoryMetrics {
public:
    MemoryInfo sample();
};
