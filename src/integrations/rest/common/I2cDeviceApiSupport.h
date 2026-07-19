#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"

namespace ewfm {

DeviceId i2cBusDependencyId(const DeviceDependencyLink* dependencies, uint8_t dependencyCount);
DeviceValidationResult validateI2cBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId, uint8_t i2cAddress,
                                                const IDeviceRuntime* childRuntime);

} // namespace ewfm
