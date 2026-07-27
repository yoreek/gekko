#include "devices/rtc/ds3231/Ds3231RtcDeviceConfig.h"

#include "devices/bus/i2c/I2cAddress.h"
#include "devices/core/ConfigCodec.h"
#include "devices/rtc/common/RtcSyncConfig.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<Ds3231RtcDeviceConfigV1>::value, "Ds3231RtcDeviceConfigV1 must be POD");
static_assert(sizeof(Ds3231RtcDeviceConfigV1::kMagic) - 1U + sizeof(Ds3231RtcDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Ds3231RtcDeviceConfigV1 exceeds device config bound");
EWFM_LEGACY_CONFIG_USE_END
static_assert(std::is_trivially_copyable<Ds3231RtcDeviceConfigV2>::value, "Ds3231RtcDeviceConfigV2 must be POD");
static_assert(std::is_base_of<I2cDeviceConfigV1, Ds3231RtcDeviceConfigV2>::value, "Ds3231RtcDeviceConfigV2 must use the shared I2C config");
static_assert(sizeof(Ds3231RtcDeviceConfigV2::kMagic) - 1U + sizeof(Ds3231RtcDeviceConfigV2) <= kMaxDeviceConfigBytes,
              "Ds3231RtcDeviceConfigV2 exceeds device config bound");

bool decodeDs3231RtcDeviceConfig(const uint8_t* blob, const size_t size, Ds3231RtcDeviceConfigV2& config) {
    if (decodeFixedConfigBlob(Ds3231RtcDeviceConfigV2::kMagic, blob, size, config) && config.validate().ok()) {
        return true;
    }
    EWFM_LEGACY_CONFIG_USE_BEGIN
    Ds3231RtcDeviceConfigV1 legacy{};
    if (!decodeFixedConfigBlob(Ds3231RtcDeviceConfigV1::kMagic, blob, size, legacy) || !legacy.validate().ok()) {
        return false;
    }
    EWFM_LEGACY_CONFIG_USE_END
    config.migrateFrom(legacy);
    return config.validate().ok();
}

bool parseDs3231RtcDeviceConfigJson(const JsonObjectConst& input, Ds3231RtcDeviceConfigV2& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeDs3231RtcDeviceConfigJson(const Ds3231RtcDeviceConfigV2& config, JsonObject output) {
    config.writeJson(output);
}

EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult Ds3231RtcDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (useForSystemTimeSync > 1U) {
        return {DeviceError::InvalidConfig, "ds3231 useForSystemTimeSync is invalid"};
    }
    return validateI2cAddress(i2cAddress);
}
EWFM_LEGACY_CONFIG_USE_END

DeviceValidationResult Ds3231RtcDeviceConfigV2::validate() const {
    const DeviceValidationResult i2cValidation = I2cDeviceConfigV1::validate();
    if (!i2cValidation.ok()) {
        return i2cValidation;
    }
    return validateRtcSyncField(useForSystemTimeSync);
}

bool Ds3231RtcDeviceConfigV2::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!I2cDeviceConfigV1::parseJson(input, error)) {
        return false;
    }
    return parseRtcSyncFieldJson(input, useForSystemTimeSync, error);
}

void Ds3231RtcDeviceConfigV2::writeJson(JsonObject output) const {
    I2cDeviceConfigV1::writeJson(output);
    writeRtcSyncFieldJson(useForSystemTimeSync, output);
}

EWFM_LEGACY_CONFIG_USE_BEGIN
void Ds3231RtcDeviceConfigV2::migrateFrom(const Ds3231RtcDeviceConfigV1& legacy) {
    enabled = legacy.enabled;
    std::memcpy(name, legacy.name, sizeof(name));
    i2cAddress = legacy.i2cAddress;
    useForSystemTimeSync = legacy.useForSystemTimeSync;
}
EWFM_LEGACY_CONFIG_USE_END

} // namespace ewfm
