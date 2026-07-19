#pragma once

#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/bus/i2c/II2cBusDriver.h"
#include "devices/core/DeviceTypes.h"

#include <cstdint>
#include <utility>

namespace ewfm {

template <typename Derived, typename RuntimeBase> class I2cDeviceRuntimeBase : public RuntimeBase {
protected:
    template <typename... Args> explicit I2cDeviceRuntimeBase(Args&&... args) : RuntimeBase(std::forward<Args>(args)...) {}

    bool probeI2cPresence(II2cBusDriver& driver) const {
        driver.beginTransmission(static_cast<const Derived*>(this)->config().i2cAddress);
        return driver.endTransmission(true) == 0U;
    }

    template <typename Config>
    static DeviceValidationResult validateI2cConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob,
                                                    bool (*decode)(const uint8_t*, size_t, Config&)) {
        if (record.dependencyDeviceId(DeviceRole::I2CBus) == 0U) {
            return {DeviceError::InvalidRelationship, kI2cBusDependencyRequiredError};
        }
        if (configBlob.size() > kMaxDeviceConfigBytes) {
            return {DeviceError::BoundsExceeded, kI2cDeviceConfigBoundsError};
        }
        Config config{};
        if (!decode(configBlob.data(), configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, kI2cDeviceConfigInvalidError};
        }
        return config.validate();
    }

public:
    bool i2cAddress(uint8_t& address) const final {
        address = static_cast<const Derived*>(this)->config().i2cAddress;
        return true;
    }
};

} // namespace ewfm
