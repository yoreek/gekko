#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <cstdint>
#include <string>

namespace ewfm {

#pragma pack(push, 1)
struct OneWireBusDeviceConfigV1 {
    static constexpr uint32_t kMagicKey = 0x4F573131UL;
    uint8_t enabled{1};
    uint8_t gpioPin{4};
    uint8_t internalPullup{0};
};
#pragma pack(pop)

std::string encodeOneWireBusDeviceConfig(const OneWireBusDeviceConfigV1& config);
bool decodeOneWireBusDeviceConfig(const std::string& blob, OneWireBusDeviceConfigV1& config);
bool parseOneWireBusDeviceConfigJson(const JsonObjectConst& input, OneWireBusDeviceConfigV1& config, std::string& error);
void writeOneWireBusDeviceConfigJson(const OneWireBusDeviceConfigV1& config, JsonObject output);

} // namespace ewfm
