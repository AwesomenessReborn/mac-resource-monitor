#include "metrics/power_metrics.h"

#include <unistd.h>
#include <cstdio>
#include <string>

// Minimal JSON value extraction — avoids a full JSON library dependency.
// Searches for  "key": <number>  and returns the number as float.
// Returns -1.0f if not found.
static float json_get_float(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return -1.0f;

    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return -1.0f;

    // Skip whitespace
    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string::npos) return -1.0f;

    // Parse the number
    try {
        size_t consumed = 0;
        float v = std::stof(json.substr(pos), &consumed);
        return v;
    } catch (...) {
        return -1.0f;
    }
}

PowerInfo PowerMetrics::sample() {
    PowerInfo info;

    if (getuid() != 0) {
        info.available = false;
        return info;
    }

    // powermetrics -f json is available on macOS 12+; fall back to plist on
    // older systems. We try json first.
    const char* cmd =
        "powermetrics --samplers cpu_power,gpu_power -i 500 -n 1 -f json 2>/dev/null";

    FILE* fp = popen(cmd, "r");
    if (!fp) return info;

    std::string output;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp))
        output += buf;
    pclose(fp);

    if (output.empty()) return info;

    // powermetrics JSON nests values under "processor"
    // Find the "processor" object and parse within it.
    auto proc_pos = output.find("\"processor\"");
    std::string search_region = (proc_pos != std::string::npos)
        ? output.substr(proc_pos)
        : output;

    float pkg = json_get_float(search_region, "package_watts");
    float cpu = json_get_float(search_region, "cpu_watts");
    float gpu = json_get_float(search_region, "gpu_watts");
    float ane = json_get_float(search_region, "ane_watts");

    info.package_w = (pkg >= 0) ? pkg : 0.0f;
    info.cpu_w     = (cpu >= 0) ? cpu : 0.0f;
    info.gpu_w     = (gpu >= 0) ? gpu : 0.0f;
    info.ane_w     = (ane >= 0) ? ane : 0.0f;
    info.available = true;

    return info;
}
