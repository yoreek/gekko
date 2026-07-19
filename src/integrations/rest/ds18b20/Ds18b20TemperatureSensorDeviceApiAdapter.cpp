#include "integrations/rest/ds18b20/Ds18b20TemperatureSensorDeviceApiAdapter.h"

namespace ewfm {
namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    return IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error);
}

DeviceId onewireBusDependencyId(const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount) {
    for (uint8_t index = 0; index < depCount; ++index) {
        if (deps[index].role == DeviceRole::OneWireBus) {
            return deps[index].deviceId;
        }
    }
    return 0;
}

DeviceValidationResult validateUniqueDependencyAddress(const DeviceRegistry& registry, const IDeviceRuntime* childRuntime,
                                                       const OneWireRomAddress& address, DeviceId dependencyDeviceId) {
    if (dependencyDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "ds18b20 requires a onewire dependency"};
    }

    const IDeviceRuntime* dependencyRuntime = registry.runtime(dependencyDeviceId);
    if (dependencyRuntime == nullptr) {
        return {DeviceError::InvalidRelationship, "ds18b20 dependency is missing or invalid"};
    }
    if (dependencyRuntime->hasDuplicateDependentRomAddress(address, childRuntime)) {
        return {DeviceError::InvalidRelationship, "duplicate ds18b20 address on dependency"};
    }
    return {};
}
} // namespace

bool Ds18b20TemperatureSensorDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                                 Ds18b20TemperatureSensorConfigV1& config, DeviceCreateRequest& request,
                                                                 const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool Ds18b20TemperatureSensorDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                                 Ds18b20TemperatureSensorConfigV1& config,
                                                                 DeviceConfigUpdateRequest& request, const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult Ds18b20TemperatureSensorDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                                       const DeviceRegistry& registry) const {
    const DeviceId dependencyDeviceId = onewireBusDependencyId(request.deps, request.depCount);
    if (dependencyDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "ds18b20 requires a onewire dependency"};
    }

    Ds18b20TemperatureSensorConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(Ds18b20TemperatureSensorConfigV1::kMagic,
                                        reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    return validateUniqueDependencyAddress(registry, nullptr, config.address, dependencyDeviceId);
}

DeviceValidationResult Ds18b20TemperatureSensorDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                             const DeviceConfigUpdateRequest& request,
                                                                                             const DeviceRegistry& registry) const {
    Ds18b20TemperatureSensorConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(Ds18b20TemperatureSensorConfigV1::kMagic,
                                        reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceId dependencyDeviceId =
        request.depsProvided ? onewireBusDependencyId(request.deps, request.depCount) : runtime.dependencyDeviceId(DeviceRole::OneWireBus);
    return validateUniqueDependencyAddress(registry, &runtime, config.address, dependencyDeviceId);
}

DeviceValidationResult
Ds18b20TemperatureSensorDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                 const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                 uint8_t depCount, const DeviceRegistry& registry) const {
    OneWireRomAddress address{};
    if (!runtime.oneWireRomAddress(address)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateUniqueDependencyAddress(registry, &runtime, address, onewireBusDependencyId(deps, depCount));
}

void Ds18b20TemperatureSensorDeviceApiAdapter::writeRuntimeJson(const Ds18b20TemperatureSensorDevice& device,
                                                                JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    JsonObject temperature = outputJson.createNestedObject("temperature");
    writeTemperatureOutputJson(device.reading(),
                               device.config().outputUnit == temperatureUnitToByte(TemperatureUnit::Fahrenheit)
                                   ? TemperatureUnit::Fahrenheit
                                   : TemperatureUnit::Celsius,
                               device.outputStatus(), temperature);
    runtimeJson["consecutiveErrors"] = device.consecutiveErrors();
    runtimeJson["lastDependencyGeneration"] = device.lastDependencyGeneration();
}

} // namespace ewfm
