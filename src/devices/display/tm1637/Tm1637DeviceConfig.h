#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/tm1637/Tm1637PanelProfile.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

// V1 drove CLK/DIO through two switch dependencies, so a migrated V1 blob carries no pin numbers.
// Such a config stays decodable (name, panel, brightness, rotation and the persisted layout
// survive) but reports its pins as unset; the runtime then faults on start until the pins are
// filled in from the portal. REST never accepts an unset pin - see Tm1637DeviceConfigV2::parseJson.
constexpr uint8_t kTm1637UnsetPin = 0xFFU;

#pragma pack(push, 1)
// Legacy persisted layout: kept only so old blobs can be decoded and migrated to V2.
struct [[deprecated("legacy persisted tm1637 config; decode/migration only")]] Tm1637DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "TM1637-1";

    uint8_t panel{static_cast<uint8_t>(Tm1637PanelKind::FourDigitDecimal036)};
    uint8_t brightness{7U};
    uint8_t rotation{0U};

    DeviceValidationResult validate() const;
};

struct Tm1637DeviceConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "TM1637-2";

    uint8_t panel{static_cast<uint8_t>(Tm1637PanelKind::FourDigitDecimal036)};
    uint8_t brightness{7U};
    uint8_t rotation{0U};
    uint8_t clkPin{kTm1637UnsetPin};
    uint8_t dioPin{kTm1637UnsetPin};

    bool pinsConfigured() const;
    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Tm1637DeviceConfigV1& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t tm1637DeviceConfigSize(const Tm1637DeviceConfigV1&) {
    return sizeof(Tm1637DeviceConfigV1::kMagic) - 1U + sizeof(Tm1637DeviceConfigV1);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t tm1637DeviceConfigSize(const Tm1637DeviceConfigV2&) {
    return sizeof(Tm1637DeviceConfigV2::kMagic) - 1U + sizeof(Tm1637DeviceConfigV2);
}

bool decodeTm1637DeviceConfig(const uint8_t* blob, size_t size, Tm1637DeviceConfigV2& config);

} // namespace ewfm
