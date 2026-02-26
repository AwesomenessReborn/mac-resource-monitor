#include "utils/iokit_helper.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <cstring>
#include <cmath>

IOKitHelper::IOKitHelper() {
    io_service_t svc = IOServiceGetMatchingService(
        kIOMainPortDefault,
        IOServiceMatching("AppleSMC")
    );
    if (svc == IO_OBJECT_NULL) return;

    IOServiceOpen(svc, mach_task_self(), 0, &conn_);
    IOObjectRelease(svc);
}

IOKitHelper::~IOKitHelper() {
    if (conn_) {
        IOServiceClose(conn_);
        conn_ = 0;
    }
}

uint32_t IOKitHelper::str_to_key(const std::string& s) {
    uint32_t k = 0;
    for (int i = 0; i < 4; ++i) {
        k = (k << 8) | (i < static_cast<int>(s.size()) ? static_cast<uint8_t>(s[i]) : ' ');
    }
    return k;
}

bool IOKitHelper::read_keyinfo(uint32_t key32, SMCKeyData_keyInfo_t& info) {
    SMCKeyData_t in{};
    SMCKeyData_t out{};
    in.key    = key32;
    in.data8  = SMC_CMD_READ_KEYINFO;

    size_t sz = sizeof(out);
    kern_return_t kr = IOConnectCallStructMethod(
        conn_, 2,
        &in,  sizeof(in),
        &out, &sz
    );
    if (kr != KERN_SUCCESS) return false;
    info = out.keyInfo;
    return true;
}

bool IOKitHelper::read_key(const std::string& key, SMCVal_t& val) {
    if (!conn_) return false;

    uint32_t key32 = str_to_key(key);

    SMCKeyData_keyInfo_t info{};
    if (!read_keyinfo(key32, info)) return false;

    SMCKeyData_t in{};
    SMCKeyData_t out{};
    in.key              = key32;
    in.keyInfo.dataSize = info.dataSize;
    in.data8            = SMC_CMD_READ_BYTES;

    size_t sz = sizeof(out);
    kern_return_t kr = IOConnectCallStructMethod(
        conn_, 2,
        &in,  sizeof(in),
        &out, &sz
    );
    if (kr != KERN_SUCCESS) return false;

    // Populate result
    std::strncpy(val.key, key.c_str(), 4);
    val.key[4]   = '\0';
    val.dataSize = info.dataSize;

    // Decode type tag back to chars
    uint32_t dt = info.dataType;
    val.dataType[0] = (dt >> 24) & 0xff;
    val.dataType[1] = (dt >> 16) & 0xff;
    val.dataType[2] = (dt >>  8) & 0xff;
    val.dataType[3] =  dt        & 0xff;
    val.dataType[4] = '\0';

    std::memcpy(val.bytes, out.bytes, sizeof(val.bytes));
    return true;
}

float IOKitHelper::decode_float(const SMCVal_t& val) {
    const uint8_t* b = val.bytes;
    std::string type(val.dataType);

    if (type == "flt ") {
        float f;
        std::memcpy(&f, b, sizeof(f));
        return f;
    }

    // sp78: signed fixed point 7.8 (sign bit, 7 integer bits, 8 fractional bits)
    if (type == "sp78") {
        int16_t raw = static_cast<int16_t>((b[0] << 8) | b[1]);
        return raw / 256.0f;
    }

    // sp5a, sp69, sp4b, sp3c, sp2d, sp1e, sp0f — all signed fixed-point
    // Generic: type is "spXX" where XX encodes integer bits
    if (type[0] == 's' && type[1] == 'p') {
        int16_t raw = static_cast<int16_t>((b[0] << 8) | b[1]);
        // Fractional bits = second nibble as hex digit value
        // e.g. "sp78" → 8 frac bits, "sp5a" → 10 frac bits
        int frac = (type[3] >= '0' && type[3] <= '9') ? (type[3] - '0')
                 : (type[3] >= 'a' && type[3] <= 'f') ? (type[3] - 'a' + 10) : 8;
        return raw / static_cast<float>(1 << frac);
    }

    // ui8
    if (type == "ui8 ") return static_cast<float>(b[0]);

    // ui16
    if (type == "ui16") return static_cast<float>((b[0] << 8) | b[1]);

    // fp1f, fp2e, fp3d, fp4c, fp5b, fp6a, fp79, fp88, fpe0 — unsigned fixed-point
    if (type[0] == 'f' && type[1] == 'p') {
        uint16_t raw = static_cast<uint16_t>((b[0] << 8) | b[1]);
        int frac = (type[3] >= '0' && type[3] <= '9') ? (type[3] - '0')
                 : (type[3] >= 'a' && type[3] <= 'f') ? (type[3] - 'a' + 10) : 8;
        return raw / static_cast<float>(1 << frac);
    }

    return 0.0f;
}
