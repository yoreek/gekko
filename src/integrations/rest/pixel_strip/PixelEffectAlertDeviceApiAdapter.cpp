#include "integrations/rest/pixel_strip/PixelEffectAlertDeviceApiAdapter.h"

#include "devices/core/DeviceDependencyValidation.h"

namespace ewfm {

namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error)) {
        return false;
    }
    bool hasStrip = false;
    uint8_t conditionCount = 0;
    for (uint8_t index = 0; index < depCount; ++index) {
        const DeviceRole role = deps[index].role;
        if (role == DeviceRole::PixelStrip) {
            if (hasStrip) {
                error = "pixel effect alert supports only one pixel_strip dependency";
                return false;
            }
            hasStrip = true;
        } else if (role == DeviceRole::Condition) {
            if (conditionCount >= kMaxPixelEffectAlertConditions) {
                error = "pixel effect alert supports at most kMaxPixelEffectAlertConditions condition dependencies";
                return false;
            }
            ++conditionCount;
        } else {
            error = "pixel effect alert dependency role must be pixel_strip or condition";
            return false;
        }
    }
    if (!hasStrip) {
        error = "pixel effect alert requires a pixel_strip dependency";
        return false;
    }
    if (dependencyLinksHaveDuplicateDeviceIds(deps.data(), depCount)) {
        error = "pixel effect alert dependency device id is duplicated";
        return false;
    }
    return true;
}

DeviceValidationResult validateCapability(const DeviceRegistry& registry, DeviceRole role, DeviceId deviceId) {
    const IDeviceRuntime* dependency = registry.runtime(deviceId);
    if (dependency == nullptr) {
        return {DeviceError::InvalidRelationship, "pixel effect alert dependency is missing"};
    }
    if (role == DeviceRole::Condition) {
        if (dependency->statusRuntime() == nullptr) {
            return {DeviceError::InvalidRelationship, "condition dependency lacks status capability"};
        }
        return {};
    }
    if (dependency->pixelStripRuntime() == nullptr) {
        return {DeviceError::InvalidRelationship, "pixel strip dependency lacks pixel strip capability"};
    }
    return {};
}
} // namespace

bool PixelEffectAlertDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                         PixelEffectAlertDeviceConfigV1& config, DeviceCreateRequest& request,
                                                         const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool PixelEffectAlertDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                         PixelEffectAlertDeviceConfigV1& config, DeviceConfigUpdateRequest& request,
                                                         const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult PixelEffectAlertDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                               const DeviceRegistry& registry) const {
    const DeviceDependencyLink* stripLink = nullptr;
    std::array<const DeviceDependencyLink*, kMaxPixelEffectAlertConditions> conditionLinks{};
    uint8_t conditionCount = 0;
    for (uint8_t index = 0; index < request.dependencyCount(); ++index) {
        const DeviceDependencyLink& link = request.dependencyLinks()[index];
        if (link.role == DeviceRole::PixelStrip) {
            stripLink = &link;
        } else if (link.role == DeviceRole::Condition && conditionCount < kMaxPixelEffectAlertConditions) {
            conditionLinks[conditionCount++] = &link;
        }
    }
    if (stripLink == nullptr) {
        return {DeviceError::InvalidRelationship, "pixel effect alert requires a pixel_strip dependency"};
    }
    if (dependencyLinksHaveDuplicateDeviceIds(request.dependencyLinks(), request.dependencyCount())) {
        return {DeviceError::InvalidRelationship, "pixel effect alert dependency device id is duplicated"};
    }

    PixelEffectAlertDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(PixelEffectAlertDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceValidationResult stripResult = validateCapability(registry, DeviceRole::PixelStrip, stripLink->deviceId);
    if (!stripResult.ok()) {
        return stripResult;
    }
    for (uint8_t index = 0; index < conditionCount; ++index) {
        const DeviceValidationResult conditionResult = validateCapability(registry, DeviceRole::Condition, conditionLinks[index]->deviceId);
        if (!conditionResult.ok()) {
            return conditionResult;
        }
    }
    return config.validate();
}

DeviceValidationResult PixelEffectAlertDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                     const DeviceConfigUpdateRequest& request,
                                                                                     const DeviceRegistry& registry) const {
    PixelEffectAlertDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(PixelEffectAlertDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    DeviceId stripDeviceId = runtime.dependencyDeviceId(DeviceRole::PixelStrip);
    std::array<DeviceId, kMaxPixelEffectAlertConditions> conditionDeviceIds{};
    uint8_t conditionCount = 0;

    if (request.depsProvided) {
        if (dependencyLinksHaveDuplicateDeviceIds(request.deps.data(), request.depCount)) {
            return {DeviceError::InvalidRelationship, "pixel effect alert dependency device id is duplicated"};
        }
        stripDeviceId = 0;
        for (uint8_t index = 0; index < request.depCount; ++index) {
            const DeviceDependencyLink& link = request.deps[index];
            if (link.role == DeviceRole::PixelStrip) {
                stripDeviceId = link.deviceId;
            } else if (link.role == DeviceRole::Condition && conditionCount < kMaxPixelEffectAlertConditions) {
                conditionDeviceIds[conditionCount++] = link.deviceId;
            }
        }
    } else {
        const DeviceDependencyLink* links = runtime.dependencyLinks();
        const uint8_t count = runtime.dependencyCount();
        for (uint8_t index = 0; index < count && links != nullptr; ++index) {
            if (links[index].role == DeviceRole::Condition && conditionCount < kMaxPixelEffectAlertConditions) {
                conditionDeviceIds[conditionCount++] = links[index].deviceId;
            }
        }
    }

    if (stripDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "pixel effect alert requires a pixel_strip dependency"};
    }

    const DeviceValidationResult stripResult = validateCapability(registry, DeviceRole::PixelStrip, stripDeviceId);
    if (!stripResult.ok()) {
        return stripResult;
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
PixelEffectAlertDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                         const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                         uint8_t depCount, const DeviceRegistry& registry) const {
    (void)runtime;
    if (dependencyLinksHaveDuplicateDeviceIds(deps.data(), depCount)) {
        return {DeviceError::InvalidRelationship, "pixel effect alert dependency device id is duplicated"};
    }
    const DeviceDependencyLink* stripDependency = nullptr;
    std::array<const DeviceDependencyLink*, kMaxPixelEffectAlertConditions> conditionDependencies{};
    uint8_t conditionCount = 0;
    for (uint8_t index = 0; index < depCount; ++index) {
        const DeviceDependencyLink& link = deps[index];
        if (link.role == DeviceRole::PixelStrip) {
            stripDependency = &link;
        } else if (link.role == DeviceRole::Condition) {
            if (conditionCount >= kMaxPixelEffectAlertConditions) {
                return {DeviceError::InvalidRelationship, "pixel effect alert supports at most kMaxPixelEffectAlertConditions condition "
                                                          "dependencies"};
            }
            conditionDependencies[conditionCount++] = &link;
        }
    }
    if (stripDependency == nullptr) {
        return {DeviceError::InvalidRelationship, "pixel effect alert requires a pixel_strip dependency"};
    }
    const DeviceValidationResult stripResult = validateCapability(registry, DeviceRole::PixelStrip, stripDependency->deviceId);
    if (!stripResult.ok()) {
        return stripResult;
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

void PixelEffectAlertDeviceApiAdapter::writeRuntimeJson(const PixelEffectAlertDevice& device, JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    outputJson["active"] = device.conditionsSatisfied();
}

} // namespace ewfm
