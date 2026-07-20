#include "integrations/rest/analog_input/Ads1115HubDeviceApiAdapter.h"

#include "integrations/rest/common/I2cDeviceApiSupport.h"

namespace ewfm {
namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    return IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error);
}
} // namespace

bool Ads1115HubDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                   Ads1115HubDeviceConfigV1& config, DeviceCreateRequest& request,
                                                   const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool Ads1115HubDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                   Ads1115HubDeviceConfigV1& config, DeviceConfigUpdateRequest& request,
                                                   const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult Ads1115HubDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                         const DeviceRegistry& registry) const {
    Ads1115HubDeviceConfigV1 config{};
    if (!decodeAds1115HubDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateI2cBusDependency(registry, i2cBusDependencyId(request.deps.data(), request.depCount), config.i2cAddress, nullptr);
}

DeviceValidationResult Ads1115HubDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                               const DeviceConfigUpdateRequest& request,
                                                                               const DeviceRegistry& registry) const {
    Ads1115HubDeviceConfigV1 config{};
    if (!decodeAds1115HubDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceId dependencyDeviceId =
        request.depsProvided ? i2cBusDependencyId(request.deps.data(), request.depCount) : runtime.dependencyDeviceId(DeviceRole::I2CBus);
    return validateI2cBusDependency(registry, dependencyDeviceId, config.i2cAddress, &runtime);
}

DeviceValidationResult
Ads1115HubDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                   const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                   const DeviceRegistry& registry) const {
    uint8_t address = 0;
    if (!runtime.i2cAddress(address)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateI2cBusDependency(registry, i2cBusDependencyId(deps.data(), depCount), address, &runtime);
}

} // namespace ewfm
