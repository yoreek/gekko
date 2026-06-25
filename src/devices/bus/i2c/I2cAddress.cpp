#include "devices/bus/i2c/I2cAddress.h"

#include <cstdio>
#include <cstdlib>

namespace ewfm {

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

} // namespace ewfm
