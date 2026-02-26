#include "metrics/fan_metrics.h"

#include <string>

FanMetrics::FanMetrics() {}

static std::string fan_key(int n, const char* suffix) {
    // e.g. "F0Ac", "F1Mn", "F2Mx"
    return std::string("F") + std::to_string(n) + suffix;
}

FanInfo FanMetrics::sample() {
    FanInfo info;

    if (!smc_.is_open()) return info;

    // Get fan count
    SMCVal_t val{};
    int count = 0;
    if (smc_.read_key("FNum", val)) {
        count = static_cast<int>(val.bytes[0]);
    }
    info.count = count;

    info.rpm.resize(count, 0.0f);
    info.min_rpm.resize(count, 0.0f);
    info.max_rpm.resize(count, 0.0f);

    for (int i = 0; i < count; ++i) {
        SMCVal_t v{};
        if (smc_.read_key(fan_key(i, "Ac"), v))
            info.rpm[i] = IOKitHelper::decode_float(v);
        if (smc_.read_key(fan_key(i, "Mn"), v))
            info.min_rpm[i] = IOKitHelper::decode_float(v);
        if (smc_.read_key(fan_key(i, "Mx"), v))
            info.max_rpm[i] = IOKitHelper::decode_float(v);
    }

    return info;
}
