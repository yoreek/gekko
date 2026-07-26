#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"

#include <ArduinoJson.h>

namespace ewfm {

// Shared parse/validate helpers used by the PCF8574/PCF8575 expander adapters and the port
// expander switch adapter -- all three deal with a single required dependency link plus a
// hardware-address-like value (i2c address for the expanders, channel index for the switch) that
// must not collide with sibling dependents.
bool parseExpanderDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                            const char*& error, const char* contextLabel);
DeviceId findDependencyIdForRole(const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount, DeviceRole role);
// Role-generic: accepts any dependency whose runtime provides IPortExpanderRuntime (PCF8574,
// PCF8575, or any future expander) -- never checks the dependency's concrete typeId.
DeviceValidationResult validatePortExpanderDependency(const DeviceRegistry& registry, DeviceId expanderDeviceId, uint8_t channel,
                                                      const IDeviceRuntime* childRuntime);
// Multi-channel sibling of validatePortExpanderDependency(), for a dependent that reserves several
// channels of one expander at once (e.g. an HD44780 LCD's RS/E/D4-D7/backlight lines).
DeviceValidationResult validatePortExpanderDependencyChannels(const DeviceRegistry& registry, DeviceId expanderDeviceId,
                                                              const uint8_t* channels, uint8_t channelCount,
                                                              const IDeviceRuntime* childRuntime);

} // namespace ewfm
