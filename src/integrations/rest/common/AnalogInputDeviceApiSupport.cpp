#include "integrations/rest/common/AnalogInputDeviceApiSupport.h"

#include "integrations/rest/common/AnalogInputDeviceValidation.h"

namespace ewfm {

namespace {
DeviceId dependencyIdForRole(const DeviceDependencyLink* dependencies, uint8_t dependencyCount, DeviceRole role) {
    if (dependencies == nullptr) {
        return 0;
    }
    for (uint8_t index = 0; index < dependencyCount; ++index) {
        if (dependencies[index].role == role) {
            return dependencies[index].deviceId;
        }
    }
    return 0;
}
} // namespace

DeviceId analogInputHubDependencyId(const DeviceDependencyLink* dependencies, uint8_t dependencyCount) {
    return dependencyIdForRole(dependencies, dependencyCount, DeviceRole::AnalogInputHub);
}

DeviceId analogInputDependencyId(const DeviceDependencyLink* dependencies, uint8_t dependencyCount) {
    return dependencyIdForRole(dependencies, dependencyCount, DeviceRole::AnalogInput);
}

DeviceValidationResult validateAnalogInputHubDependency(const DeviceRegistry& registry, DeviceId hubDeviceId, uint8_t channel,
                                                        const IDeviceRuntime* childRuntime) {
    if (hubDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, kAnalogInputHubDependencyRequiredError};
    }
    const IDeviceRuntime* hubRuntime = registry.runtime(hubDeviceId);
    const IAnalogInputHubRuntime* hub = hubRuntime != nullptr ? hubRuntime->analogInputHubRuntime() : nullptr;
    if (hub == nullptr) {
        return {DeviceError::InvalidRelationship, kAnalogInputHubDependencyInvalidError};
    }
    if (channel >= hub->channelCount()) {
        return {DeviceError::InvalidConfig, kAnalogInputChannelOutOfRangeError};
    }
    if (hubRuntime->hasDuplicateDependentChannel(channel, childRuntime)) {
        return {DeviceError::InvalidRelationship, kAnalogInputChannelConflictError};
    }
    return {};
}

DeviceValidationResult validateAnalogInputDependency(const DeviceRegistry& registry, DeviceId deviceId) {
    if (deviceId == 0U) {
        return {DeviceError::InvalidRelationship, kAnalogInputDependencyRequiredError};
    }
    const IDeviceRuntime* runtime = registry.runtime(deviceId);
    if (runtime == nullptr || runtime->analogInputRuntime() == nullptr) {
        return {DeviceError::InvalidRelationship, kAnalogInputDependencyInvalidError};
    }
    return {};
}

} // namespace ewfm
