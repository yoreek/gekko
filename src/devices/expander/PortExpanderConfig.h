#pragma once

#include "devices/bus/i2c/I2cDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr uint32_t kPcf857xExpanderConfigVersion = 2;

#pragma pack(push, 1)
// Legacy persisted layout: kept only so an old "PX857X1" blob can be decoded and migrated to V2.
// Only the data + validate() survive; all JSON handling lives on V2.
struct [[deprecated("legacy persisted PCF857x config; decode/migration only")]] Pcf857xExpanderConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "PX857X1";
    uint8_t i2cAddress{0x20};
    uint8_t inverted{0};

    DeviceValidationResult validate() const;
};

struct Pcf857xExpanderConfigV2 : I2cDeviceConfigV1 {
    static constexpr char kMagic[] = "PX857X2";

    Pcf857xExpanderConfigV2() : I2cDeviceConfigV1(0x20U) {}

    uint8_t inverted{0};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Pcf857xExpanderConfigV1& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t pcf857xExpanderConfigSize(const Pcf857xExpanderConfigV1&) {
    return sizeof(Pcf857xExpanderConfigV1::kMagic) - 1U + sizeof(Pcf857xExpanderConfigV1);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t pcf857xExpanderConfigSize(const Pcf857xExpanderConfigV2&) {
    return sizeof(Pcf857xExpanderConfigV2::kMagic) - 1U + sizeof(Pcf857xExpanderConfigV2);
}

bool decodePcf857xExpanderConfig(const uint8_t* blob, size_t size, Pcf857xExpanderConfigV2& config);
bool parsePcf857xExpanderConfigJson(const JsonObjectConst& input, Pcf857xExpanderConfigV2& config, const char*& error);
void writePcf857xExpanderConfigJson(const Pcf857xExpanderConfigV2& config, JsonObject output);

} // namespace ewfm
