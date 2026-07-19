#pragma once

#include "devices/analog/AnalogOutputDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kLedcAnalogOutputDeviceTypeId = 20;
constexpr uint32_t kLedcAnalogOutputDeviceConfigVersion = 1;

#pragma pack(push, 1)
struct LedcAnalogOutputDeviceConfigV1 : AnalogOutputDeviceConfigV1 {
    static constexpr char kMagic[] = "AOUT-1";
    uint8_t pin{0xFFU};
    uint8_t ledcChannel{0U};
    uint32_t frequencyHz{5000U};
    uint8_t dutyBits{12U};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t ledcAnalogOutputDeviceConfigSize(const LedcAnalogOutputDeviceConfigV1&) {
    return sizeof(LedcAnalogOutputDeviceConfigV1::kMagic) - 1U + sizeof(LedcAnalogOutputDeviceConfigV1);
}

bool decodeLedcAnalogOutputDeviceConfig(const uint8_t* blob, size_t size, LedcAnalogOutputDeviceConfigV1& config);

} // namespace ewfm
