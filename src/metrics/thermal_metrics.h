#pragma once

#include "utils/iokit_helper.h"

struct ThermalInfo {
    float cpu_temp     = 0.0f;
    float gpu_temp     = 0.0f;
    bool  has_gpu_temp = false;
};

class ThermalMetrics {
public:
    ThermalMetrics();
    ThermalInfo sample();

private:
    IOKitHelper smc_;
    bool        is_arm64_ = false;
};
