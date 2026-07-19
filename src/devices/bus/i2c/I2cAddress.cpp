#include "devices/bus/i2c/I2cAddress.h"

#include <cstdio>
#include <cstdlib>

namespace ewfm {
namespace {
constexpr const char* kI2cAddressNumericError = "i2c address must be numeric";
constexpr const char* kI2cAddressBoundsError = "i2c address is out of bounds";
} // namespace

bool i2cAddressIsValid(const I2cAddress& address) {
    return address.value <= 0x7FU;
}

bool formatI2cAddress(const I2cAddress& address, char (&out)[3]) {
    if (!i2cAddressIsValid(address)) {
        out[0] = '\0';
        return false;
    }
    std::snprintf(out, sizeof(out), "%02X", address.value);
    return true;
}

bool parseI2cAddress(const char* input, I2cAddress& address) {
    if (input == nullptr || *input == '\0') {
        return false;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(input, &end, 0);
    if (end == input || *end != '\0' || parsed > 0x7FU) {
        return false;
    }

    address.value = static_cast<uint8_t>(parsed);
    return true;
}

bool parseI2cAddressJson(const JsonVariantConst& input, uint8_t& address, const char*& error) {
    if (input.isNull()) {
        return true;
    }
    if (!input.is<unsigned long>() && !input.is<long>() && !input.is<int>()) {
        error = kI2cAddressNumericError;
        return false;
    }
    const long parsed = input.as<long>();
    if (parsed < 0 || parsed > 0x7F) {
        error = kI2cAddressBoundsError;
        return false;
    }
    address = static_cast<uint8_t>(parsed);
    return true;
}

DeviceValidationResult validateI2cAddress(uint8_t address) {
    if (!i2cAddressIsValid(I2cAddress{address})) {
        return {DeviceError::InvalidConfig, kI2cAddressBoundsError};
    }
    return {};
}

void writeI2cAddressJson(uint8_t address, JsonObject output) {
    output["i2cAddress"] = address;
}

} // namespace ewfm
