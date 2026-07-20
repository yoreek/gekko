#include "integrations/rest/analog_input/AnalogInputChannelDeviceApiAdapter.h"

#include "devices/analog/input/AnalogInputJson.h"
#include "integrations/rest/common/AnalogInputDeviceApiSupport.h"

namespace ewfm {
namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    return IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error);
}
} // namespace

bool AnalogInputChannelDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                           AnalogInputChannelDeviceConfigV1& config, DeviceCreateRequest& request,
                                                           const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool AnalogInputChannelDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                           AnalogInputChannelDeviceConfigV1& config, DeviceConfigUpdateRequest& request,
                                                           const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult AnalogInputChannelDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                                 const DeviceRegistry& registry) const {
    AnalogInputChannelDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic,
                                        reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateAnalogInputHubDependency(registry, analogInputHubDependencyId(request.deps.data(), request.depCount), config.channel,
                                            nullptr);
}

DeviceValidationResult AnalogInputChannelDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                       const DeviceConfigUpdateRequest& request,
                                                                                       const DeviceRegistry& registry) const {
    AnalogInputChannelDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic,
                                        reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceId dependencyDeviceId = request.depsProvided ? analogInputHubDependencyId(request.deps.data(), request.depCount)
                                                             : runtime.dependencyDeviceId(DeviceRole::AnalogInputHub);
    return validateAnalogInputHubDependency(registry, dependencyDeviceId, config.channel, &runtime);
}

DeviceValidationResult
AnalogInputChannelDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                           const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                           uint8_t depCount, const DeviceRegistry& registry) const {
    uint8_t channel = 0;
    if (!runtime.expanderChannel(channel)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateAnalogInputHubDependency(registry, analogInputHubDependencyId(deps.data(), depCount), channel, &runtime);
}

void AnalogInputChannelDeviceApiAdapter::writeRuntimeJson(const AnalogInputChannelDevice& device, JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    JsonObject analogInput = outputJson.createNestedObject("analogInput");
    writeAnalogInputOutputJson(device.reading(), device.outputStatus(), analogInput);
}

} // namespace ewfm
