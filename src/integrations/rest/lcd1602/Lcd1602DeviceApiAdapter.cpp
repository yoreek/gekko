#include "integrations/rest/lcd1602/Lcd1602DeviceApiAdapter.h"

#include "devices/display/DisplayTextPlaceholderAst.h"
#include "integrations/rest/expander/ExpanderApiAdapterSupport.h"

namespace ewfm {

namespace {
DeviceValidationResult validateLcd1602ExpanderDependency(const DeviceRegistry& registry, const DeviceId expanderDeviceId,
                                                         const Lcd1602DeviceConfigV1& config, const IDeviceRuntime* childRuntime) {
    uint8_t channels[7]{};
    const uint8_t channelCount = lcd1602ConfigChannels(config, channels, 7U);
    return validatePortExpanderDependencyChannels(registry, expanderDeviceId, channels, channelCount, childRuntime);
}
} // namespace

bool Lcd1602DeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                Lcd1602DeviceConfigV1& config, DeviceCreateRequest& request, const char*& error) const {
    (void)input;
    if (!parseExpanderDepsField(configInput, request.deps, request.depCount, error, kDepsRequiredError)) {
        return false;
    }
    if (!collectTextPlaceholderDeviceIds(config.line1, request.deps, request.depCount, error, kInvalidLineError, kDependencyCountError)) {
        return false;
    }
    if (!collectTextPlaceholderDeviceIds(config.line2, request.deps, request.depCount, error, kInvalidLineError, kDependencyCountError)) {
        return false;
    }
    return true;
}

bool Lcd1602DeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                       DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = kConfigRequiredError;
        return false;
    }

    Lcd1602DeviceConfigV1 config = static_cast<const Lcd1602Device&>(runtime).config();
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    if (!config.validate().ok()) {
        error = kInvalidConfigError;
        return false;
    }

    request = {};
    request.configVersion = adapterConfigVersion();
    if (!assignDeviceBaseConfig(request.baseConfig, config.name, config.enabled != 0U)) {
        error = "device base config is invalid";
        return false;
    }
    if (!assignEncodedConfigBlob(config, request.configBlob, error)) {
        return false;
    }

    const bool explicitDepsProvided = !input["deps"].isNull();
    if (explicitDepsProvided) {
        if (!parseExpanderDepsField(input, request.deps, request.depCount, error, kDepsRequiredError)) {
            return false;
        }
    } else {
        request.deps[0] = DeviceDependencyLink{DeviceRole::PortExpander, runtime.dependencyDeviceId(DeviceRole::PortExpander)};
        request.depCount = 1U;
    }
    if (!collectTextPlaceholderDeviceIds(config.line1, request.deps, request.depCount, error, kInvalidLineError, kDependencyCountError)) {
        return false;
    }
    if (!collectTextPlaceholderDeviceIds(config.line2, request.deps, request.depCount, error, kInvalidLineError, kDependencyCountError)) {
        return false;
    }
    request.depsProvided = explicitDepsProvided || request.depCount > 1U;
    return true;
}

void Lcd1602DeviceApiAdapter::writeRuntimeJson(const Lcd1602Device& device, JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    outputJson["line1"] = JsonString(device.renderedLine1(), JsonString::Copied);
    outputJson["line2"] = JsonString(device.renderedLine2(), JsonString::Copied);
}

DeviceValidationResult Lcd1602DeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                      const DeviceRegistry& registry) const {
    Lcd1602DeviceConfigV1 config{};
    if (!decodeLcd1602DeviceConfig(request.configBlob.data(), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateLcd1602ExpanderDependency(registry, findDependencyIdForRole(request.deps, request.depCount, DeviceRole::PortExpander),
                                             config, nullptr);
}

DeviceValidationResult Lcd1602DeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                            const DeviceConfigUpdateRequest& request,
                                                                            const DeviceRegistry& registry) const {
    Lcd1602DeviceConfigV1 config{};
    if (!decodeLcd1602DeviceConfig(request.configBlob.data(), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    const DeviceId expanderDeviceId = request.depsProvided
                                          ? findDependencyIdForRole(request.deps, request.depCount, DeviceRole::PortExpander)
                                          : runtime.dependencyDeviceId(DeviceRole::PortExpander);
    return validateLcd1602ExpanderDependency(registry, expanderDeviceId, config, &runtime);
}

DeviceValidationResult Lcd1602DeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                       const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                       uint8_t depCount, const DeviceRegistry& registry) const {
    uint8_t channels[7]{};
    const uint8_t channelCount = runtime.expanderChannels(channels, 7U);
    return validatePortExpanderDependencyChannels(registry, findDependencyIdForRole(deps, depCount, DeviceRole::PortExpander), channels,
                                                  channelCount, &runtime);
}

} // namespace ewfm
