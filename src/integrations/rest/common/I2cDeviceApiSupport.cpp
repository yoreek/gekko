#include "integrations/rest/common/I2cDeviceApiSupport.h"

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceValidation.h"

namespace ewfm {

DeviceId i2cBusDependencyId(const DeviceDependencyLink* dependencies, uint8_t dependencyCount) {
    if (dependencies == nullptr) {
        return 0;
    }
    for (uint8_t index = 0; index < dependencyCount; ++index) {
        if (dependencies[index].role == DeviceRole::I2CBus) {
            return dependencies[index].deviceId;
        }
    }
    return 0;
}

DeviceValidationResult validateI2cBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId, uint8_t i2cAddress,
                                                const IDeviceRuntime* childRuntime) {
    if (busDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, kI2cBusDependencyRequiredError};
    }
    const IDeviceRuntime* busRuntime = registry.runtime(busDeviceId);
    if (busRuntime == nullptr || busRuntime->typeId() != I2cBusDevice::descriptor().typeId) {
        return {DeviceError::InvalidRelationship, kI2cBusDependencyInvalidError};
    }
    if (busRuntime->hasDuplicateDependentI2cAddress(i2cAddress, childRuntime)) {
        return {DeviceError::InvalidRelationship, kI2cAddressConflictError};
    }
    return {};
}

} // namespace ewfm
