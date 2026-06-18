#include "devices/bus/onewire/OneWireBusConfig.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
template <typename T> std::string encodeConfigBlob(uint32_t magicKey, const T& config) {
    std::string blob;
    blob.resize(sizeof(magicKey) + sizeof(T));
    std::memcpy(blob.data(), &magicKey, sizeof(magicKey));
    std::memcpy(blob.data() + sizeof(magicKey), &config, sizeof(T));
    return blob;
}
} // namespace

static_assert(std::is_trivially_copyable<OneWireBusDeviceConfigV1>::value, "OneWireBusDeviceConfigV1 must be POD");
static_assert(sizeof(OneWireBusDeviceConfigV1) == 3, "OneWireBusDeviceConfigV1 layout changed");
static_assert(sizeof(OneWireBusDeviceConfigV1::kMagicKey) + sizeof(OneWireBusDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "OneWireBusDeviceConfigV1 exceeds device config bound");

std::string encodeOneWireBusDeviceConfig(const OneWireBusDeviceConfigV1& config) {
    return encodeConfigBlob(OneWireBusDeviceConfigV1::kMagicKey, config);
}

bool decodeOneWireBusDeviceConfig(const std::string& blob, OneWireBusDeviceConfigV1& config) {
    constexpr size_t kBlobSize = sizeof(OneWireBusDeviceConfigV1::kMagicKey) + sizeof(OneWireBusDeviceConfigV1);
    if (blob.size() != kBlobSize) {
        return false;
    }

    uint32_t magicKey{0};
    std::memcpy(&magicKey, blob.data(), sizeof(magicKey));
    if (magicKey != OneWireBusDeviceConfigV1::kMagicKey) {
        return false;
    }

    std::memcpy(&config, blob.data() + sizeof(magicKey), sizeof(OneWireBusDeviceConfigV1));
    return true;
}

bool parseOneWireBusDeviceConfigJson(const JsonObjectConst& input, OneWireBusDeviceConfigV1& config, std::string& error) {
    config.enabled = (input["enabled"] | true) ? 1U : 0U;
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
    output["enabled"] = config.enabled != 0U;
    output["gpio_pin"] = config.gpioPin;
    output["internal_pullup"] = config.internalPullup != 0U;
}

} // namespace ewfm
