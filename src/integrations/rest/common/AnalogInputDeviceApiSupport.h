#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"

namespace ewfm {

DeviceId analogInputHubDependencyId(const DeviceDependencyLink* dependencies, uint8_t dependencyCount);
DeviceId analogInputDependencyId(const DeviceDependencyLink* dependencies, uint8_t dependencyCount);

// For the two AnalogInputHub-dependent leaves (ads1115_input, cd74hc4067_input): the target must
// provide DeviceRole::AnalogInputHub (matched by interface, not by concrete type -- an ADS1115 hub
// and a CD74HC4067 hub are equally valid), `channel` must be in range for that specific hub, and
// the channel must not already be claimed by another dependent leaf.
DeviceValidationResult validateAnalogInputHubDependency(const DeviceRegistry& registry, DeviceId hubDeviceId, uint8_t channel,
                                                        const IDeviceRuntime* childRuntime);

// For any device consuming a plain AnalogInput reading (e.g. the NTC thermistor sensor): the
// target must provide DeviceRole::AnalogInput, again matched by interface only.
DeviceValidationResult validateAnalogInputDependency(const DeviceRegistry& registry, DeviceId deviceId);

} // namespace ewfm
