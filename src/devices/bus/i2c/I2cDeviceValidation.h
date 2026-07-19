#pragma once

namespace ewfm {

inline constexpr const char* kI2cBusDependencyRequiredError = "i2c bus dependency is required";
inline constexpr const char* kI2cBusDependencyInvalidError = "i2c bus dependency is invalid";
inline constexpr const char* kI2cAddressConflictError = "i2c address is already in use on the selected bus";
inline constexpr const char* kI2cDeviceConfigBoundsError = "i2c device config exceeds supported size";
inline constexpr const char* kI2cDeviceConfigInvalidError = "i2c device config is invalid";

} // namespace ewfm
