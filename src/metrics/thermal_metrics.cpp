#include "metrics/thermal_metrics.h"

#include <sys/sysctl.h>
#include <cstring>
#include <string>

ThermalMetrics::ThermalMetrics() {
    char machine[64]{};
    size_t sz = sizeof(machine);
    if (sysctlbyname("hw.machine", machine, &sz, nullptr, 0) == 0) {
        is_arm64_ = (std::string(machine).find("arm64") != std::string::npos);
    }
}

ThermalInfo ThermalMetrics::sample() {
    ThermalInfo info;

    if (!smc_.is_open()) return info;

    SMCVal_t val{};

    if (is_arm64_) {
        // Apple Silicon: try several known CPU-die keys in order of preference
        const char* cpu_keys[] = { "Tp09", "Tp01", "Tp05", "TC0P", nullptr };
        for (int i = 0; cpu_keys[i]; ++i) {
            if (smc_.read_key(cpu_keys[i], val)) {
                float t = IOKitHelper::decode_float(val);
                if (t > 0.0f && t < 150.0f) { info.cpu_temp = t; break; }
            }
        }

        // GPU die
        const char* gpu_keys[] = { "Tg05", "Tg0D", "Tg0P", nullptr };
        for (int i = 0; gpu_keys[i]; ++i) {
            if (smc_.read_key(gpu_keys[i], val)) {
                float t = IOKitHelper::decode_float(val);
                if (t > 0.0f && t < 150.0f) {
                    info.gpu_temp     = t;
                    info.has_gpu_temp = true;
                    break;
                }
            }
        }
    } else {
        // Intel
        const char* cpu_keys[] = { "TC0P", "TC0D", "TC0E", nullptr };
        for (int i = 0; cpu_keys[i]; ++i) {
            if (smc_.read_key(cpu_keys[i], val)) {
                float t = IOKitHelper::decode_float(val);
                if (t > 0.0f && t < 150.0f) { info.cpu_temp = t; break; }
            }
        }
    }

    return info;
}
