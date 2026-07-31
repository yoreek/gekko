#include "devices/bus/spi/SpiBusConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {

bool parsePinField(const JsonVariantConst& variant, const char* rangeError, const char*& error, long minValue, long maxValue,
                   uint8_t& out) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned long>() && !variant.is<long>() && !variant.is<int>()) {
        error = "spi bus field must be numeric";
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < minValue || parsed > maxValue) {
        error = rangeError;
        return false;
    }
    out = static_cast<uint8_t>(parsed);
    return true;
}

} // namespace

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<SpiBusDeviceConfigV1>::value, "SpiBusDeviceConfigV1 must be POD");
static_assert(sizeof(SpiBusDeviceConfigV1) == 39, "SpiBusDeviceConfigV1 layout changed");
EWFM_LEGACY_CONFIG_USE_END

static_assert(std::is_trivially_copyable<SpiBusDeviceConfigV2>::value, "SpiBusDeviceConfigV2 must be POD");
static_assert(sizeof(SpiBusDeviceConfigV2) == 38, "SpiBusDeviceConfigV2 layout changed");
static_assert(sizeof(SpiBusDeviceConfigV2::kMagic) - 1U + sizeof(SpiBusDeviceConfigV2) <= kMaxDeviceConfigBytes,
              "SpiBusDeviceConfigV2 exceeds device config bound");

bool spiBusHostIsValid(uint8_t host) {
    return host == kSpiBusHostHspi || host == kSpiBusHostVspi;
}

bool decodeSpiBusDeviceConfig(const uint8_t* blob, const size_t size, SpiBusDeviceConfigV2& config) {
    if (decodeValidatedFixedConfigBlob(SpiBusDeviceConfigV2::kMagic, blob, size, config)) {
        return true;
    }

    EWFM_LEGACY_CONFIG_USE_BEGIN
    SpiBusDeviceConfigV1 legacy{};
    if (!decodeFixedConfigBlob(SpiBusDeviceConfigV1::kMagic, blob, size, legacy) || !legacy.validate().ok()) {
        return false;
    }
    config.migrateFrom(legacy);
    EWFM_LEGACY_CONFIG_USE_END
    return config.validate().ok();
}

EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult SpiBusDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!spiBusHostIsValid(host)) {
        return {DeviceError::InvalidConfig, "spi bus host is invalid"};
    }
    if (sckPin == mosiPin) {
        return {DeviceError::InvalidConfig, "spi bus sck and mosi pins must differ"};
    }
    if (misoPin >= 0 && (misoPin > 255 || static_cast<uint8_t>(misoPin) == sckPin || static_cast<uint8_t>(misoPin) == mosiPin)) {
        return {DeviceError::InvalidConfig, "spi bus miso pin must differ from sck and mosi"};
    }
    return {};
}

void SpiBusDeviceConfigV2::migrateFrom(const SpiBusDeviceConfigV1& legacy) {
    static_cast<DeviceBaseConfigV1&>(*this) = static_cast<const DeviceBaseConfigV1&>(legacy);
    host = legacy.host;
    sckPin = legacy.sckPin;
    mosiPin = legacy.mosiPin;
    misoPin = legacy.misoPin >= 0 ? static_cast<uint8_t>(legacy.misoPin) : kSpiBusMisoUnset;
}
EWFM_LEGACY_CONFIG_USE_END

bool SpiBusDeviceConfigV2::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    if (!parsePinField(input["host"], "spi bus host is out of bounds", error, 0L, 255L, host) ||
        !parsePinField(input["sckPin"], "spi bus sck pin is out of bounds", error, 0L, 255L, sckPin) ||
        !parsePinField(input["mosiPin"], "spi bus mosi pin is out of bounds", error, 0L, 255L, mosiPin) ||
        !parsePinField(input["misoPin"], "spi bus miso pin is out of bounds", error, 0L, 255L, misoPin)) {
        return false;
    }
    return true;
}

DeviceValidationResult SpiBusDeviceConfigV2::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!spiBusHostIsValid(host)) {
        return {DeviceError::InvalidConfig, "spi bus host is invalid"};
    }
    if (sckPin == mosiPin) {
        return {DeviceError::InvalidConfig, "spi bus sck and mosi pins must differ"};
    }
    if (misoPin != kSpiBusMisoUnset && (misoPin == sckPin || misoPin == mosiPin)) {
        return {DeviceError::InvalidConfig, "spi bus miso pin must differ from sck and mosi"};
    }
    return {};
}

void SpiBusDeviceConfigV2::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["host"] = host;
    output["sckPin"] = sckPin;
    output["mosiPin"] = mosiPin;
    if (misoPin != kSpiBusMisoUnset) {
        output["misoPin"] = misoPin;
    }
}

} // namespace ewfm
