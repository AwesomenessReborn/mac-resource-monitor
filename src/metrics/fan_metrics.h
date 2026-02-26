#pragma once

#include <vector>
#include "utils/iokit_helper.h"

struct FanInfo {
    int                count    = 0;
    std::vector<float> rpm;
    std::vector<float> min_rpm;
    std::vector<float> max_rpm;
};

class FanMetrics {
public:
    FanMetrics();
    FanInfo sample();

private:
    IOKitHelper smc_;
};
