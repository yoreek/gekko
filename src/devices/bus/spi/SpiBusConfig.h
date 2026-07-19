#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr uint8_t kSpiBusHostHspi = 1U;
constexpr uint8_t kSpiBusHostVspi = 2U;

#pragma pack(push, 1)
struct SpiBusDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "SPB1";
    uint8_t host{kSpiBusHostVspi};
    uint8_t sckPin{18};
    uint8_t mosiPin{23};
    int16_t misoPin{-1};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t spiBusDeviceConfigSize(const SpiBusDeviceConfigV1&) {
    return sizeof(SpiBusDeviceConfigV1::kMagic) - 1U + sizeof(SpiBusDeviceConfigV1);
}

bool parseSpiBusDeviceConfigJson(const JsonObjectConst& input, SpiBusDeviceConfigV1& config, const char*& error);
void writeSpiBusDeviceConfigJson(const SpiBusDeviceConfigV1& config, JsonObject output);

bool spiBusHostIsValid(uint8_t host);

} // namespace ewfm
