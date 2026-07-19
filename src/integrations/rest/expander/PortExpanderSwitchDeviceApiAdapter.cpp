#include "integrations/rest/expander/PortExpanderSwitchDeviceApiAdapter.h"

namespace ewfm {

DeviceValidationResult PortExpanderSwitchDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                                 const DeviceRegistry& registry) const {
    PortExpanderSwitchDeviceConfigV3 config{};
    if (!decodePortExpanderSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                              config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validatePortExpanderDependency(registry, findDependencyIdForRole(request.deps, request.depCount, DeviceRole::PortExpander),
                                          config.channel, nullptr);
}

DeviceValidationResult PortExpanderSwitchDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                       const DeviceConfigUpdateRequest& request,
                                                                                       const DeviceRegistry& registry) const {
    PortExpanderSwitchDeviceConfigV3 config{};
    if (!decodePortExpanderSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                              config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    const DeviceId dependencyDeviceId = request.depsProvided
                                            ? findDependencyIdForRole(request.deps, request.depCount, DeviceRole::PortExpander)
                                            : runtime.dependencyDeviceId(DeviceRole::PortExpander);
    return validatePortExpanderDependency(registry, dependencyDeviceId, config.channel, &runtime);
}

DeviceValidationResult
PortExpanderSwitchDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                           const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                           uint8_t depCount, const DeviceRegistry& registry) const {
    uint8_t channel = 0;
    if (!runtime.expanderChannel(channel)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validatePortExpanderDependency(registry, findDependencyIdForRole(deps, depCount, DeviceRole::PortExpander), channel, &runtime);
}

} // namespace ewfm
