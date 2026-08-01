#pragma once

#include "devices/analog/adc/IAdcInputDriver.h"
#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kCd74hc4067HubTypeId = 27;
constexpr uint32_t kCd74hc4067HubConfigVersion = 1;
constexpr uint8_t kCd74hc4067ChannelCount = 16;

#pragma pack(push, 1)
struct Cd74hc4067HubDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "CD74-HUB-1";

    uint8_t selectPins[4]{16, 17, 18, 19}; // S0..S3
    uint8_t enablePin{kGpioPinUnset};      // 0xFF = not wired (tied to GND on the module)
    // No single canonical default among the 8 valid ADC1 pins; 0xFF forces the user to pick one
    // explicitly (analogPortInputGpioPinIsValid() rejects it). See docs/pin-configuration-conventions.md.
    uint8_t sigPin{0xFFU};
    uint8_t sigAttenuation{static_cast<uint8_t>(AdcAttenuation::Db11)};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t cd74hc4067HubDeviceConfigSize(const Cd74hc4067HubDeviceConfigV1&) {
    return sizeof(Cd74hc4067HubDeviceConfigV1::kMagic) - 1U + sizeof(Cd74hc4067HubDeviceConfigV1);
}

} // namespace ewfm
