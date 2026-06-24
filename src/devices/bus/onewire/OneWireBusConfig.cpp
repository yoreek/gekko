#include "devices/bus/onewire/OneWireBusConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {}

static_assert(std::is_trivially_copyable<OneWireBusDeviceConfigV1>::value, "OneWireBusDeviceConfigV1 must be POD");
static_assert(sizeof(OneWireBusDeviceConfigV1) == 36, "OneWireBusDeviceConfigV1 layout changed");
static_assert(sizeof(OneWireBusDeviceConfigV1::kMagic) - 1U + sizeof(OneWireBusDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "OneWireBusDeviceConfigV1 exceeds device config bound");

bool encodeOneWireBusDeviceConfig(const OneWireBusDeviceConfigV1& config, uint8_t* blob, size_t capacity) {
    return encodeFixedConfigBlob(OneWireBusDeviceConfigV1::kMagic, config, blob, capacity);
}

bool decodeOneWireBusDeviceConfig(const uint8_t* blob, size_t size, OneWireBusDeviceConfigV1& config) {
    return decodeFixedConfigBlob(OneWireBusDeviceConfigV1::kMagic, blob, size, config);
}

bool parseOneWireBusDeviceConfigJson(const JsonObjectConst& input, OneWireBusDeviceConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeOneWireBusDeviceConfigJson(const OneWireBusDeviceConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

bool OneWireBusDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    internalPullup = (input["internalPullup"] | false) ? 1U : 0U;

    const JsonVariantConst pinVariant = input["gpioPin"];
    if (!pinVariant.isNull()) {
        if (!pinVariant.is<int>()) {
            error = "onewire bus pin must be numeric";
            return false;
        }
        const int pin = pinVariant.as<int>();
        if (pin < 0 || pin > 255) {
            error = "onewire bus pin is out of bounds";
            return false;
        }
        gpioPin = static_cast<uint8_t>(pin);
    }

    return true;
}

DeviceValidationResult OneWireBusDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    return {};
}

void OneWireBusDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["gpioPin"] = gpioPin;
    output["internalPullup"] = internalPullup != 0U;
}

} // namespace ewfm
