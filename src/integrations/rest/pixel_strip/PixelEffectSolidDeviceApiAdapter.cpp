#include "integrations/rest/pixel_strip/PixelEffectSolidDeviceApiAdapter.h"

#include "devices/core/DeviceDependencyValidation.h"
#include "devices/pixel/PixelColorJson.h"

namespace ewfm {

namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error)) {
        return false;
    }
    if (depCount != 1U || deps[0].role != DeviceRole::PixelStrip) {
        error = "pixel effect solid requires exactly one pixel_strip dependency";
        return false;
    }
    return true;
}

DeviceValidationResult validateTarget(const DeviceRegistry& registry, DeviceId deviceId) {
    const IDeviceRuntime* dependency = registry.runtime(deviceId);
    if (dependency == nullptr || dependency->pixelStripRuntime() == nullptr) {
        return {DeviceError::InvalidRelationship, "pixel strip dependency is missing"};
    }
    return {};
}
} // namespace

bool PixelEffectSolidDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                         PixelEffectSolidDeviceConfigV1& config, DeviceCreateRequest& request,
                                                         const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool PixelEffectSolidDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                         PixelEffectSolidDeviceConfigV1& config, DeviceConfigUpdateRequest& request,
                                                         const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    return !request.depsProvided || parseDepsField(input, request.deps, request.depCount, error);
}

DeviceValidationResult PixelEffectSolidDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                               const DeviceRegistry& registry) const {
    if (request.dependencyCount() != 1U || request.dependencyLinks()[0].role != DeviceRole::PixelStrip) {
        return {DeviceError::InvalidRelationship, "pixel effect solid requires exactly one pixel_strip dependency"};
    }
    PixelEffectSolidDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(PixelEffectSolidDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    const DeviceValidationResult targetResult = validateTarget(registry, request.dependencyLinks()[0].deviceId);
    if (!targetResult.ok()) {
        return targetResult;
    }
    return config.validate();
}

DeviceValidationResult PixelEffectSolidDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                     const DeviceConfigUpdateRequest& request,
                                                                                     const DeviceRegistry& registry) const {
    PixelEffectSolidDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(PixelEffectSolidDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    DeviceId targetDeviceId = runtime.dependencyDeviceId(DeviceRole::PixelStrip);
    if (request.depsProvided) {
        if (request.depCount != 1U || request.deps[0].role != DeviceRole::PixelStrip) {
            return {DeviceError::InvalidRelationship, "pixel effect solid requires exactly one pixel_strip dependency"};
        }
        targetDeviceId = request.deps[0].deviceId;
    }
    if (targetDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "pixel strip dependency is required"};
    }
    const DeviceValidationResult targetResult = validateTarget(registry, targetDeviceId);
    if (!targetResult.ok()) {
        return targetResult;
    }
    return config.validate();
}

DeviceValidationResult
PixelEffectSolidDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                         const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                         uint8_t depCount, const DeviceRegistry& registry) const {
    (void)runtime;
    if (depCount != 1U || deps[0].role != DeviceRole::PixelStrip) {
        return {DeviceError::InvalidRelationship, "pixel effect solid requires exactly one pixel_strip dependency"};
    }
    return validateTarget(registry, deps[0].deviceId);
}

void PixelEffectSolidDeviceApiAdapter::writeRuntimeJson(const PixelEffectSolidDevice& device, JsonObject runtimeJson) const {
    JsonObject output = runtimeJson.createNestedObject("output");
    writePixelColorJson(output, "color", device.liveColor());
    output["on"] = device.liveOn();
}

} // namespace ewfm
