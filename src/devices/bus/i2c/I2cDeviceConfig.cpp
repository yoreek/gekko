#include "devices/bus/i2c/I2cDeviceConfig.h"

#include "devices/bus/i2c/I2cAddress.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<I2cDeviceConfigV1>::value, "I2cDeviceConfigV1 must be POD");
static_assert(sizeof(I2cDeviceConfigV1) == sizeof(DeviceBaseConfigV1) + sizeof(uint8_t), "I2cDeviceConfigV1 layout changed");

bool I2cDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    return DeviceBaseConfigV1::parseJson(input, error) && parseI2cAddressJson(input["i2cAddress"], i2cAddress, error);
}

DeviceValidationResult I2cDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    return validateI2cAddress(i2cAddress);
}

void I2cDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    writeI2cAddressJson(i2cAddress, output);
}

} // namespace ewfm
