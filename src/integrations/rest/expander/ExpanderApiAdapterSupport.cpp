#include "integrations/rest/expander/ExpanderApiAdapterSupport.h"

#include "integrations/common/DeviceApiAdapter.h"

namespace ewfm {

bool parseExpanderDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                            const char*& error, const char* contextLabel) {
    if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error)) {
        error = contextLabel;
        return false;
    }
    return true;
}

DeviceId findDependencyIdForRole(const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount, DeviceRole role) {
    for (uint8_t index = 0; index < depCount; ++index) {
        if (deps[index].role == role) {
            return deps[index].deviceId;
        }
    }
    return 0;
}

DeviceValidationResult validatePortExpanderDependency(const DeviceRegistry& registry, DeviceId expanderDeviceId, uint8_t channel,
                                                      const IDeviceRuntime* childRuntime) {
    if (expanderDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "port expander switch requires a port expander dependency"};
    }
    const IDeviceRuntime* expanderRuntime = registry.runtime(expanderDeviceId);
    if (expanderRuntime == nullptr || expanderRuntime->portExpanderRuntime() == nullptr) {
        return {DeviceError::InvalidRelationship, "port expander switch dependency is missing or invalid"};
    }
    if (channel >= expanderRuntime->portExpanderRuntime()->channelCount()) {
        return {DeviceError::InvalidConfig, "port expander switch channel is out of range"};
    }
    if (expanderRuntime->hasDuplicateDependentChannel(channel, childRuntime)) {
        return {DeviceError::InvalidRelationship, "duplicate port expander channel on dependency"};
    }
    return {};
}

DeviceValidationResult validatePortExpanderDependencyChannels(const DeviceRegistry& registry, DeviceId expanderDeviceId,
                                                              const uint8_t* channels, uint8_t channelCount,
                                                              const IDeviceRuntime* childRuntime) {
    if (expanderDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "port expander dependency is required"};
    }
    const IDeviceRuntime* expanderRuntime = registry.runtime(expanderDeviceId);
    if (expanderRuntime == nullptr || expanderRuntime->portExpanderRuntime() == nullptr) {
        return {DeviceError::InvalidRelationship, "port expander dependency is missing or invalid"};
    }
    const uint8_t available = expanderRuntime->portExpanderRuntime()->channelCount();
    for (uint8_t index = 0; index < channelCount; ++index) {
        if (channels[index] >= available) {
            return {DeviceError::InvalidConfig, "port expander channel is out of range"};
        }
        if (expanderRuntime->hasDuplicateDependentChannel(channels[index], childRuntime)) {
            return {DeviceError::InvalidRelationship, "duplicate port expander channel on dependency"};
        }
    }
    return {};
}

} // namespace ewfm
