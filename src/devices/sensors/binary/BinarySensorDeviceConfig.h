#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/binary/IGpioInputDriver.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kBinarySensorDeviceTypeId = 18;
constexpr uint32_t kBinarySensorDeviceConfigVersion = 1;
constexpr uint16_t kBinarySensorMaxDebounceMs = 60000U;

#pragma pack(push, 1)
struct BinarySensorDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "BINSEN-1";
    // No real default GPIO exists for a generic digital sensor; 0xFF forces the user to pick one
    // explicitly (binarySensorPinIsValid() rejects it). See docs/pin-configuration-conventions.md.
    uint8_t gpioPin{0xFFU};
    uint8_t pullMode{static_cast<uint8_t>(GpioInputPullMode::PullUp)};
    uint8_t inverted{0};
    uint16_t debounceMs{50};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t binarySensorDeviceConfigSize(const BinarySensorDeviceConfigV1&) {
    return sizeof(BinarySensorDeviceConfigV1::kMagic) - 1U + sizeof(BinarySensorDeviceConfigV1);
}

bool gpioInputPullModeFromString(const char* value, GpioInputPullMode& pullMode);
const char* gpioInputPullModeName(GpioInputPullMode pullMode);
bool binarySensorPinIsValid(uint8_t pin);
bool binarySensorPinSupportsPull(uint8_t pin);

} // namespace ewfm
