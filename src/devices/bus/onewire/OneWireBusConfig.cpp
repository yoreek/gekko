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
    config.internalPullup = (input["internal_pullup"] | false) ? 1U : 0U;

    const JsonVariantConst pinVariant = input["gpio_pin"];
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
        config.gpioPin = static_cast<uint8_t>(pin);
    }

    return true;
}

void writeOneWireBusDeviceConfigJson(const OneWireBusDeviceConfigV1& config, JsonObject output) {
    writeDeviceBaseConfigJson(config.base, output);
    output["gpio_pin"] = config.gpioPin;
    output["internal_pullup"] = config.internalPullup != 0U;
}

} // namespace ewfm
