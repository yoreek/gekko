#pragma once

#include "devices/bus/i2c/I2cDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr uint32_t kPcf857xExpanderConfigVersion = 2;

#pragma pack(push, 1)
struct Pcf857xExpanderConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "PX857X1";
    // PCF857x modules conventionally answer at 0x20 (all address pins low); user-editable since
    // multiple expanders can share a bus at different addresses.
    uint8_t i2cAddress{0x20};
    // Inverts every channel's electrical level before writing the register (for boards wired
    // active-low, e.g. many relay modules) -- distinct from each channel-switch's own per-channel
    // `inverted` flag, which only affects that one channel's logical sense.
    uint8_t inverted{0};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};

struct Pcf857xExpanderConfigV2 : I2cDeviceConfigV1 {
    static constexpr char kMagic[] = "PX857X2";

    Pcf857xExpanderConfigV2() : I2cDeviceConfigV1(0x20U) {}

    uint8_t inverted{0};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    void migrateFrom(const Pcf857xExpanderConfigV1& legacy);
};
#pragma pack(pop)

constexpr size_t pcf857xExpanderConfigSize(const Pcf857xExpanderConfigV1&) {
    return sizeof(Pcf857xExpanderConfigV1::kMagic) - 1U + sizeof(Pcf857xExpanderConfigV1);
}

constexpr size_t pcf857xExpanderConfigSize(const Pcf857xExpanderConfigV2&) {
    return sizeof(Pcf857xExpanderConfigV2::kMagic) - 1U + sizeof(Pcf857xExpanderConfigV2);
}

bool decodePcf857xExpanderConfig(const uint8_t* blob, size_t size, Pcf857xExpanderConfigV2& config);
bool parsePcf857xExpanderConfigJson(const JsonObjectConst& input, Pcf857xExpanderConfigV1& config, const char*& error);
void writePcf857xExpanderConfigJson(const Pcf857xExpanderConfigV1& config, JsonObject output);
bool parsePcf857xExpanderConfigJson(const JsonObjectConst& input, Pcf857xExpanderConfigV2& config, const char*& error);
void writePcf857xExpanderConfigJson(const Pcf857xExpanderConfigV2& config, JsonObject output);

} // namespace ewfm
