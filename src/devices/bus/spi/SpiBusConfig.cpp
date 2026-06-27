#include "devices/bus/spi/SpiBusConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {}

static_assert(std::is_trivially_copyable<SpiBusDeviceConfigV1>::value, "SpiBusDeviceConfigV1 must be POD");
static_assert(sizeof(SpiBusDeviceConfigV1) == 39, "SpiBusDeviceConfigV1 layout changed");
static_assert(sizeof(SpiBusDeviceConfigV1::kMagic) - 1U + sizeof(SpiBusDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "SpiBusDeviceConfigV1 exceeds device config bound");

bool spiBusHostIsValid(uint8_t host) {
    return host == kSpiBusHostHspi || host == kSpiBusHostVspi;
}

bool encodeSpiBusDeviceConfig(const SpiBusDeviceConfigV1& config, uint8_t* blob, size_t capacity) {
    return encodeFixedConfigBlob(SpiBusDeviceConfigV1::kMagic, config, blob, capacity);
}

bool decodeSpiBusDeviceConfig(const uint8_t* blob, size_t size, SpiBusDeviceConfigV1& config) {
    return decodeFixedConfigBlob(SpiBusDeviceConfigV1::kMagic, blob, size, config) && config.validate().ok();
}

bool parseSpiBusDeviceConfigJson(const JsonObjectConst& input, SpiBusDeviceConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeSpiBusDeviceConfigJson(const SpiBusDeviceConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

bool SpiBusDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    const JsonVariantConst hostVariant = input["host"];
    if (!hostVariant.isNull()) {
        if (!hostVariant.is<unsigned long>() && !hostVariant.is<long>() && !hostVariant.is<int>()) {
            error = "spi bus host must be numeric";
            return false;
        }
        const long parsed = hostVariant.as<long>();
        if (parsed < 0 || parsed > 255L) {
            error = "spi bus host is out of bounds";
            return false;
        }
        host = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst sckVariant = input["sckPin"];
    if (!sckVariant.isNull()) {
        if (!sckVariant.is<unsigned long>() && !sckVariant.is<long>() && !sckVariant.is<int>()) {
            error = "spi bus sck pin must be numeric";
            return false;
        }
        const long parsed = sckVariant.as<long>();
        if (parsed < 0 || parsed > 255L) {
            error = "spi bus sck pin is out of bounds";
            return false;
        }
        sckPin = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst mosiVariant = input["mosiPin"];
    if (!mosiVariant.isNull()) {
        if (!mosiVariant.is<unsigned long>() && !mosiVariant.is<long>() && !mosiVariant.is<int>()) {
            error = "spi bus mosi pin must be numeric";
            return false;
        }
        const long parsed = mosiVariant.as<long>();
        if (parsed < 0 || parsed > 255L) {
            error = "spi bus mosi pin is out of bounds";
            return false;
        }
        mosiPin = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst misoVariant = input["misoPin"];
    if (!misoVariant.isNull()) {
        if (!misoVariant.is<unsigned long>() && !misoVariant.is<long>() && !misoVariant.is<int>()) {
            error = "spi bus miso pin must be numeric";
            return false;
        }
        const long parsed = misoVariant.as<long>();
        if (parsed < -1L || parsed > 255L) {
            error = "spi bus miso pin is out of bounds";
            return false;
        }
        misoPin = static_cast<int16_t>(parsed);
    }

    return true;
}

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

void SpiBusDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["host"] = host;
    output["sckPin"] = sckPin;
    output["mosiPin"] = mosiPin;
    if (misoPin >= 0) {
        output["misoPin"] = static_cast<int>(misoPin);
    }
}

} // namespace ewfm
