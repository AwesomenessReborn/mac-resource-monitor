#pragma once

struct PowerInfo {
    float package_w = -1.0f;
    float cpu_w     = -1.0f;
    float gpu_w     = -1.0f;
    float ane_w     = -1.0f;
    bool  available = false;
};

class PowerMetrics {
public:
    PowerInfo sample();
};
