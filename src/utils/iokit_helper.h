#pragma once

#include <IOKit/IOKitLib.h>
#include <string>

// SMC data types
#define SMC_CMD_READ_BYTES  5
#define SMC_CMD_READ_KEYINFO 9

#pragma pack(push, 1)
struct SMCKeyData_vers_t {
    char major;
    char minor;
    char build;
    char reserved[1];
    uint16_t release;
};

struct SMCKeyData_pLimitData_t {
    uint16_t version;
    uint16_t length;
    uint32_t cpuPLimit;
    uint32_t gpuPLimit;
    uint32_t memPLimit;
};

struct SMCKeyData_keyInfo_t {
    uint32_t dataSize;
    uint32_t dataType;
    uint8_t  dataAttributes;
};

struct SMCKeyData_t {
    uint32_t                 key;
    SMCKeyData_vers_t        vers;
    SMCKeyData_pLimitData_t  pLimitData;
    SMCKeyData_keyInfo_t     keyInfo;
    uint8_t                  result;
    uint8_t                  status;
    uint8_t                  data8;
    uint32_t                 data32;
    uint8_t                  bytes[32];
};

struct SMCVal_t {
    char     key[5];
    uint32_t dataSize;
    char     dataType[5];
    uint8_t  bytes[32];
};
#pragma pack(pop)

class IOKitHelper {
public:
    IOKitHelper();
    ~IOKitHelper();

    bool is_open() const { return conn_ != 0; }

    // Read an SMC key. Returns true on success; result in val.
    bool read_key(const std::string& key, SMCVal_t& val);

    // Convenience: decode a float from SMCVal_t (handles "flt ", "sp78", "ui8 " etc.)
    static float decode_float(const SMCVal_t& val);

private:
    io_connect_t conn_ = 0;

    bool read_keyinfo(uint32_t key32, SMCKeyData_keyInfo_t& info);
    uint32_t str_to_key(const std::string& s);
};
