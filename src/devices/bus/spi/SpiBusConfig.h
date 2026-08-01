#pragma once

#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr uint8_t kSpiBusHostHspi = 1U;
constexpr uint8_t kSpiBusHostVspi = 2U;

#pragma pack(push, 1)
// Legacy persisted layout: kept only so old blobs can be decoded and migrated to V2. misoPin was
// int16_t with -1 meaning "not wired"; V2 narrows it to uint8_t with the project-wide 0xFF
// sentinel, which is why this needs a version bump instead of an in-place edit.
struct [[deprecated("legacy persisted spi_bus config; decode/migration only")]] SpiBusDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "SPB1";
    uint8_t host{kSpiBusHostVspi};
    uint8_t sckPin{18};
    uint8_t mosiPin{23};
    int16_t misoPin{-1};

    DeviceValidationResult validate() const;
};

struct SpiBusDeviceConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "SPB2";
    uint8_t host{kSpiBusHostVspi};
    uint8_t sckPin{18};
    uint8_t mosiPin{23};
    uint8_t misoPin{kGpioPinUnset};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const SpiBusDeviceConfigV1& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t spiBusDeviceConfigSize(const SpiBusDeviceConfigV1&) {
    return sizeof(SpiBusDeviceConfigV1::kMagic) - 1U + sizeof(SpiBusDeviceConfigV1);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t spiBusDeviceConfigSize(const SpiBusDeviceConfigV2&) {
    return sizeof(SpiBusDeviceConfigV2::kMagic) - 1U + sizeof(SpiBusDeviceConfigV2);
}

bool decodeSpiBusDeviceConfig(const uint8_t* blob, size_t size, SpiBusDeviceConfigV2& config);

bool spiBusHostIsValid(uint8_t host);

} // namespace ewfm
