#include "devices/bus/i2c/I2cBusConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {}

static_assert(std::is_trivially_copyable<I2cBusDeviceConfigV1>::value, "I2cBusDeviceConfigV1 must be POD");
static_assert(sizeof(I2cBusDeviceConfigV1::kMagic) - 1U + sizeof(I2cBusDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "I2cBusDeviceConfigV1 exceeds device config bound");

bool encodeI2cBusDeviceConfig(const I2cBusDeviceConfigV1& config, uint8_t* blob, size_t capacity) {
    return encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, config, blob, capacity);
}

bool decodeI2cBusDeviceConfig(const uint8_t* blob, size_t size, I2cBusDeviceConfigV1& config) {
    return decodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, blob, size, config) && config.validate().ok();
}

bool parseI2cBusDeviceConfigJson(const JsonObjectConst& input, I2cBusDeviceConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeI2cBusDeviceConfigJson(const I2cBusDeviceConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

bool I2cBusDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    const JsonVariantConst sdaVariant = input["sdaPin"];
    if (!sdaVariant.isNull()) {
        if (!sdaVariant.is<int>()) {
            error = "i2c bus sda pin must be numeric";
            return false;
        }
        const int pin = sdaVariant.as<int>();
        if (pin < 0 || pin > 255) {
            error = "i2c bus sda pin is out of bounds";
            return false;
        }
        sdaPin = static_cast<uint8_t>(pin);
    }

    const JsonVariantConst sclVariant = input["sclPin"];
    if (!sclVariant.isNull()) {
        if (!sclVariant.is<int>()) {
            error = "i2c bus scl pin must be numeric";
            return false;
        }
        const int pin = sclVariant.as<int>();
        if (pin < 0 || pin > 255) {
            error = "i2c bus scl pin is out of bounds";
            return false;
        }
        sclPin = static_cast<uint8_t>(pin);
    }

    internalPullup = (input["internalPullup"] | (internalPullup != 0U)) ? 1U : 0U;

    const JsonVariantConst frequencyVariant = input["frequencyHz"];
    if (!frequencyVariant.isNull()) {
        if (!frequencyVariant.is<unsigned long>() && !frequencyVariant.is<long>() && !frequencyVariant.is<int>()) {
            error = "i2c bus frequency must be numeric";
            return false;
        }
        const long parsed = frequencyVariant.as<long>();
        if (parsed < 0 || static_cast<unsigned long>(parsed) > 0xFFFFFFFFUL) {
            error = "i2c bus frequency is out of bounds";
            return false;
        }
        frequencyHz = static_cast<uint32_t>(parsed);
    }

    return true;
}

DeviceValidationResult I2cBusDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (sdaPin == sclPin) {
        return {DeviceError::InvalidConfig, "i2c bus sda and scl pins must differ"};
    }
    if (frequencyHz < 1000U) {
        return {DeviceError::InvalidConfig, "i2c bus frequency is too low"};
    }
    if (frequencyHz > 400000U) {
        return {DeviceError::InvalidConfig, "i2c bus frequency exceeds supported maximum"};
    }
    return {};
}

void I2cBusDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["sdaPin"] = sdaPin;
    output["sclPin"] = sclPin;
    output["internalPullup"] = internalPullup != 0U;
    output["frequencyHz"] = frequencyHz;
}

} // namespace ewfm
