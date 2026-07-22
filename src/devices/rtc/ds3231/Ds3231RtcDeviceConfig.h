#pragma once

#include "devices/bus/i2c/I2cDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kDs3231RtcTypeId = 11;
constexpr uint32_t kDs3231RtcConfigVersion = 2;

#pragma pack(push, 1)
// Legacy persisted layout: kept only so an old "DS3231-1" blob can be decoded and migrated to
// V2. Only the data + validate() survive; all JSON/runtime handling lives on V2.
struct [[deprecated("legacy persisted DS3231 config; decode/migration only")]] Ds3231RtcDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DS3231-1";
    uint8_t useForSystemTimeSync{0};
    // DS3231 modules conventionally answer at 0x68, but the field stays user-editable (scan +
    // pick, mirroring Ssd1306DeviceConfigV4::i2cAddress) since some breakout boards/clones ship
    // at a different address.
    uint8_t i2cAddress{0x68};

    DeviceValidationResult validate() const;
};

struct Ds3231RtcDeviceConfigV2 : I2cDeviceConfigV1 {
    static constexpr char kMagic[] = "DS3231-2";

    Ds3231RtcDeviceConfigV2() : I2cDeviceConfigV1(0x68U) {}

    uint8_t useForSystemTimeSync{0};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Ds3231RtcDeviceConfigV1& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t ds3231RtcDeviceConfigSize(const Ds3231RtcDeviceConfigV1&) {
    return sizeof(Ds3231RtcDeviceConfigV1::kMagic) - 1U + sizeof(Ds3231RtcDeviceConfigV1);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t ds3231RtcDeviceConfigSize(const Ds3231RtcDeviceConfigV2&) {
    return sizeof(Ds3231RtcDeviceConfigV2::kMagic) - 1U + sizeof(Ds3231RtcDeviceConfigV2);
}

bool decodeDs3231RtcDeviceConfig(const uint8_t* blob, size_t size, Ds3231RtcDeviceConfigV2& config);
bool parseDs3231RtcDeviceConfigJson(const JsonObjectConst& input, Ds3231RtcDeviceConfigV2& config, const char*& error);
void writeDs3231RtcDeviceConfigJson(const Ds3231RtcDeviceConfigV2& config, JsonObject output);

} // namespace ewfm
