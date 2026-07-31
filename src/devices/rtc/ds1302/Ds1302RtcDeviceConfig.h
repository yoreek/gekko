#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kDs1302RtcTypeId = 32;
constexpr uint32_t kDs1302RtcConfigVersion = 1;
// No real default GPIOs exist for a DS1302's 3 bit-banged wires; 0xFF forces the user to pick
// them explicitly (gpioSwitchPinIsValid() rejects it). See docs/pin-configuration-conventions.md.
constexpr uint8_t kDs1302DefaultClkPin = 0xFFU;
constexpr uint8_t kDs1302DefaultDataPin = 0xFFU;
constexpr uint8_t kDs1302DefaultRstPin = 0xFFU;

#pragma pack(push, 1)
// DS1302 is a bit-banged 3-wire RTC (CLK/DAT/RST-CE), not an I2C device - no bus dependency, just
// three directly-owned GPIO pins, mirroring Dht11SensorConfigV1's shape rather than
// Ds3231RtcDeviceConfigV2's I2C-bus composition.
struct Ds1302RtcDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DS1302-1";

    uint8_t clkPin{kDs1302DefaultClkPin};
    uint8_t dataPin{kDs1302DefaultDataPin};
    uint8_t rstPin{kDs1302DefaultRstPin};
    uint8_t useForSystemTimeSync{0};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t ds1302RtcDeviceConfigSize(const Ds1302RtcDeviceConfigV1&) {
    return sizeof(Ds1302RtcDeviceConfigV1::kMagic) - 1U + sizeof(Ds1302RtcDeviceConfigV1);
}

bool decodeDs1302RtcDeviceConfig(const uint8_t* blob, size_t size, Ds1302RtcDeviceConfigV1& config);
bool parseDs1302RtcDeviceConfigJson(const JsonObjectConst& input, Ds1302RtcDeviceConfigV1& config, const char*& error);
void writeDs1302RtcDeviceConfigJson(const Ds1302RtcDeviceConfigV1& config, JsonObject output);

} // namespace ewfm
