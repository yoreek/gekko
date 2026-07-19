#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct I2cDeviceConfigV1 : DeviceBaseConfigV1 {
    explicit constexpr I2cDeviceConfigV1(uint8_t defaultAddress = 0U) : i2cAddress(defaultAddress) {}

    uint8_t i2cAddress;

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

} // namespace ewfm
