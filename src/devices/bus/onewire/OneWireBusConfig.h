#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct OneWireBusDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OWB1";
    // No real default GPIO exists for a OneWire bus; 0xFF forces the user to pick one explicitly
    // (validate() rejects it via gpioSwitchPinIsValid()). See docs/pin-configuration-conventions.md.
    uint8_t gpioPin{0xFFU};
    uint8_t internalPullup{0};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t oneWireBusDeviceConfigSize(const OneWireBusDeviceConfigV1&) {
    return sizeof(OneWireBusDeviceConfigV1::kMagic) - 1U + sizeof(OneWireBusDeviceConfigV1);
}

bool parseOneWireBusDeviceConfigJson(const JsonObjectConst& input, OneWireBusDeviceConfigV1& config, const char*& error);
void writeOneWireBusDeviceConfigJson(const OneWireBusDeviceConfigV1& config, JsonObject output);

} // namespace ewfm
