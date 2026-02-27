#pragma once

#include <cstdint>

enum class MemoryPressureLevel { Normal, Warning, Critical };

struct MemoryInfo {
    uint64_t total_bytes       = 0;
    uint64_t used_bytes        = 0;
    uint64_t active_bytes      = 0;
    uint64_t wired_bytes       = 0;
    uint64_t compressed_bytes  = 0;
    uint64_t inactive_bytes    = 0;
    uint64_t free_bytes        = 0;
    uint64_t swap_used_bytes   = 0;
    uint64_t swap_total_bytes  = 0;
    double   pressure_percent  = 0.0;
    int      kern_pressure     = -1;
    MemoryPressureLevel pressure_level = MemoryPressureLevel::Normal;
};

class MemoryMetrics {
public:
    MemoryInfo sample();
};
