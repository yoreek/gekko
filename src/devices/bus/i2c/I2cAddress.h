#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

struct I2cAddress {
    uint8_t value{0};
};

bool i2cAddressIsValid(const I2cAddress& address);
bool formatI2cAddress(const I2cAddress& address, char (&out)[3]);
bool parseI2cAddress(const char* input, I2cAddress& address);
bool parseI2cAddressJson(const JsonVariantConst& input, uint8_t& address, const char*& error);
DeviceValidationResult validateI2cAddress(uint8_t address);
void writeI2cAddressJson(uint8_t address, JsonObject output);

} // namespace ewfm
