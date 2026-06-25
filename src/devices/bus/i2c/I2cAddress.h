#pragma once

#include <cstdint>

namespace ewfm {

struct I2cAddress {
    uint8_t value{0};
};

bool i2cAddressIsValid(const I2cAddress& address);
bool formatI2cAddress(const I2cAddress& address, char (&out)[3]);
bool parseI2cAddress(const char* input, I2cAddress& address);

} // namespace ewfm
