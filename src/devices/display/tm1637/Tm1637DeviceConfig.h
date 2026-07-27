#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/tm1637/Tm1637PanelProfile.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct Tm1637DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "TM1637-1";

    uint8_t panel{static_cast<uint8_t>(Tm1637PanelKind::FourDigitDecimal036)};
    uint8_t brightness{7U};
    uint8_t rotation{0U};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t tm1637DeviceConfigSize(const Tm1637DeviceConfigV1&) {
    return sizeof(Tm1637DeviceConfigV1::kMagic) - 1U + sizeof(Tm1637DeviceConfigV1);
}

bool decodeTm1637DeviceConfig(const uint8_t* blob, size_t size, Tm1637DeviceConfigV1& config);

} // namespace ewfm
