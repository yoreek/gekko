#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct I2cBusDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "I2B1";
    uint8_t sdaPin{21};
    uint8_t sclPin{22};
    uint8_t internalPullup{1};
    uint32_t frequencyHz{100000};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t i2cBusDeviceConfigSize(const I2cBusDeviceConfigV1&) {
    return sizeof(I2cBusDeviceConfigV1::kMagic) - 1U + sizeof(I2cBusDeviceConfigV1);
}

bool parseI2cBusDeviceConfigJson(const JsonObjectConst& input, I2cBusDeviceConfigV1& config, const char*& error);
void writeI2cBusDeviceConfigJson(const I2cBusDeviceConfigV1& config, JsonObject output);

} // namespace ewfm
