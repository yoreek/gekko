#include "integrations/rest/auto_switch/AutoSwitchDeviceApiAdapter.h"

namespace ewfm {

namespace {
// A device must not appear more than once in the same auto_switch's dependency list, whether as
// two condition links pointing at the same device or as the target switch reused as its own
// condition - either would be a confusing, almost-certainly-accidental configuration.
bool hasDuplicateDeviceId(const DeviceDependencyLink* links, uint8_t count) {
    for (uint8_t index = 0; index < count; ++index) {
        for (uint8_t otherIndex = index + 1; otherIndex < count; ++otherIndex) {
            if (links[index].deviceId == links[otherIndex].deviceId) {
                return true;
            }
        }
    }
    return false;
}

bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error)) {
        return false;
    }
    bool hasSwitch = false;
    uint8_t conditionCount = 0;
    for (uint8_t index = 0; index < depCount; ++index) {
        const DeviceRole role = deps[index].role;
        if (role == DeviceRole::Switch) {
            if (hasSwitch) {
                error = "auto switch supports only one switch dependency";
                return false;
            }
            hasSwitch = true;
        } else if (role == DeviceRole::Condition) {
            if (conditionCount >= kMaxAutoSwitchConditions) {
                error = "auto switch supports at most kMaxAutoSwitchConditions condition dependencies";
                return false;
            }
            ++conditionCount;
        } else {
            // Clean-break: the old single `role=schedule` dependency link is no longer accepted -
            // schedules (and switches, and other AutoSwitchDevices) are now attached uniformly as
            // `role=condition` links.
            error = "auto switch dependency role is invalid, use role=condition";
            return false;
        }
    }
    if (!hasSwitch) {
        error = "auto switch requires a switch dependency";
        return false;
    }
    if (hasDuplicateDeviceId(deps.data(), depCount)) {
        error = "auto switch dependency device id is duplicated";
        return false;
    }
    return true;
}

DeviceValidationResult validateCapability(const DeviceRegistry& registry, DeviceRole role, DeviceId deviceId) {
    const IDeviceRuntime* dependency = registry.runtime(deviceId);
    if (dependency == nullptr) {
        return {DeviceError::InvalidRelationship, "auto switch dependency is missing"};
    }
    if (role == DeviceRole::Condition) {
        if (dependency->statusRuntime() == nullptr) {
            return {DeviceError::InvalidRelationship, "condition dependency lacks status capability"};
        }
        return {};
    }
    const ISwitchOutputRuntime* switchOutput = dependency->switchOutputRuntime();
    if (switchOutput == nullptr) {
        return {DeviceError::InvalidRelationship, "switch dependency lacks output capability"};
    }
    return {};
}
} // namespace

bool AutoSwitchDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                   AutoSwitchDeviceConfigV1& config, DeviceCreateRequest& request,
                                                   const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool AutoSwitchDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                   AutoSwitchDeviceConfigV1& config, DeviceConfigUpdateRequest& request,
                                                   const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult AutoSwitchDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                         const DeviceRegistry& registry) const {
    const DeviceDependencyLink* switchLink = nullptr;
    std::array<const DeviceDependencyLink*, kMaxAutoSwitchConditions> conditionLinks{};
    uint8_t conditionCount = 0;
    for (uint8_t index = 0; index < request.dependencyCount(); ++index) {
        const DeviceDependencyLink& link = request.dependencyLinks()[index];
        if (link.role == DeviceRole::Switch) {
            switchLink = &link;
        } else if (link.role == DeviceRole::Condition && conditionCount < kMaxAutoSwitchConditions) {
            conditionLinks[conditionCount++] = &link;
        }
    }
    if (switchLink == nullptr) {
        return {DeviceError::InvalidRelationship, "auto switch requires a switch dependency"};
    }
    if (hasDuplicateDeviceId(request.dependencyLinks(), request.dependencyCount())) {
        return {DeviceError::InvalidRelationship, "auto switch dependency device id is duplicated"};
    }

    AutoSwitchDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceValidationResult switchResult = validateCapability(registry, DeviceRole::Switch, switchLink->deviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    for (uint8_t index = 0; index < conditionCount; ++index) {
        const DeviceValidationResult conditionResult = validateCapability(registry, DeviceRole::Condition, conditionLinks[index]->deviceId);
        if (!conditionResult.ok()) {
            return conditionResult;
        }
    }
    return config.validate();
}

DeviceValidationResult AutoSwitchDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                               const DeviceConfigUpdateRequest& request,
                                                                               const DeviceRegistry& registry) const {
    AutoSwitchDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    DeviceId switchDeviceId = runtime.dependencyDeviceId(DeviceRole::Switch);
    std::array<DeviceId, kMaxAutoSwitchConditions> conditionDeviceIds{};
    uint8_t conditionCount = 0;

    if (request.depsProvided) {
        if (hasDuplicateDeviceId(request.deps.data(), request.depCount)) {
            return {DeviceError::InvalidRelationship, "auto switch dependency device id is duplicated"};
        }
        switchDeviceId = 0;
        for (uint8_t index = 0; index < request.depCount; ++index) {
            const DeviceDependencyLink& link = request.deps[index];
            if (link.role == DeviceRole::Switch) {
                switchDeviceId = link.deviceId;
            } else if (link.role == DeviceRole::Condition && conditionCount < kMaxAutoSwitchConditions) {
                conditionDeviceIds[conditionCount++] = link.deviceId;
            }
        }
    } else {
        // deps weren't touched by this update - re-validate against the runtime's existing wiring
        // (dependencyDeviceId() only ever returns the first match per role, so Condition links,
        // which can repeat, are enumerated directly from the live dependency list instead).
        const DeviceDependencyLink* links = runtime.dependencyLinks();
        const uint8_t count = runtime.dependencyCount();
        for (uint8_t index = 0; index < count && links != nullptr; ++index) {
            if (links[index].role == DeviceRole::Condition && conditionCount < kMaxAutoSwitchConditions) {
                conditionDeviceIds[conditionCount++] = links[index].deviceId;
            }
        }
    }

    if (switchDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "auto switch requires a switch dependency"};
    }

    const DeviceValidationResult switchResult = validateCapability(registry, DeviceRole::Switch, switchDeviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    for (uint8_t index = 0; index < conditionCount; ++index) {
        const DeviceValidationResult conditionResult = validateCapability(registry, DeviceRole::Condition, conditionDeviceIds[index]);
        if (!conditionResult.ok()) {
            return conditionResult;
        }
    }
    return config.validate();
}

DeviceValidationResult
AutoSwitchDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                   const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                   const DeviceRegistry& registry) const {
    (void)runtime;
    if (hasDuplicateDeviceId(deps.data(), depCount)) {
        return {DeviceError::InvalidRelationship, "auto switch dependency device id is duplicated"};
    }
    const DeviceDependencyLink* switchDependency = nullptr;
    std::array<const DeviceDependencyLink*, kMaxAutoSwitchConditions> conditionDependencies{};
    uint8_t conditionCount = 0;
    for (uint8_t index = 0; index < depCount; ++index) {
        const DeviceDependencyLink& link = deps[index];
        if (link.role == DeviceRole::Switch) {
            switchDependency = &link;
        } else if (link.role == DeviceRole::Condition) {
            if (conditionCount >= kMaxAutoSwitchConditions) {
                return {DeviceError::InvalidRelationship, "auto switch supports at most kMaxAutoSwitchConditions condition dependencies"};
            }
            conditionDependencies[conditionCount++] = &link;
        }
    }
    if (switchDependency == nullptr) {
        return {DeviceError::InvalidRelationship, "auto switch requires a switch dependency"};
    }
    const DeviceValidationResult switchResult = validateCapability(registry, DeviceRole::Switch, switchDependency->deviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    for (uint8_t index = 0; index < conditionCount; ++index) {
        const DeviceValidationResult conditionResult =
            validateCapability(registry, DeviceRole::Condition, conditionDependencies[index]->deviceId);
        if (!conditionResult.ok()) {
            return conditionResult;
        }
    }
    return {};
}

void AutoSwitchDeviceApiAdapter::writeRuntimeJson(const AutoSwitchDevice& device, JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    outputJson["mode"] = autoSwitchModeName(device.mode());
    outputJson["paused"] = device.paused();
    outputJson["pausedUntilMs"] = device.pausedUntilMs();
    outputJson["conditionsSatisfied"] = device.conditionsSatisfied();
    outputJson["state"] = device.currentOutputState();
}

} // namespace ewfm
