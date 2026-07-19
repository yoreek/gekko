#include "integrations/rest/thermostat/ThermostatDeviceApiAdapter.h"

#include "devices/switch/SwitchOutputState.h"

namespace ewfm {

namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error)) {
        return false;
    }
    bool hasTemperatureSensor = false;
    bool hasSwitch = false;
    for (uint8_t index = 0; index < depCount; ++index) {
        const DeviceRole role = deps[index].role;
        if (role == DeviceRole::TemperatureSensor) {
            hasTemperatureSensor = true;
        } else if (role == DeviceRole::Switch) {
            hasSwitch = true;
        } else {
            error = "thermostat dependency role is invalid";
            return false;
        }
    }
    if (!hasTemperatureSensor || !hasSwitch) {
        error = "thermostat requires temperature_sensor and switch deps";
        return false;
    }
    return true;
}

DeviceValidationResult validateCapability(const DeviceRegistry& registry, DeviceRole role, DeviceId deviceId) {
    const IDeviceRuntime* dependency = registry.runtime(deviceId);
    if (dependency == nullptr) {
        return {DeviceError::InvalidRelationship, "thermostat dependency is missing"};
    }
    if (role == DeviceRole::TemperatureSensor) {
        const ITemperatureReadingRuntime* temperature = dependency->temperatureReadingRuntime();
        if (temperature == nullptr) {
            return {DeviceError::InvalidRelationship, "temperature_sensor dependency lacks temperature capability"};
        }
        TemperatureReading reading{};
        (void)temperature->latestTemperatureReading(reading);
        return {};
    }
    const ISwitchOutputRuntime* switchOutput = dependency->switchOutputRuntime();
    if (switchOutput == nullptr) {
        return {DeviceError::InvalidRelationship, "switch dependency lacks output capability"};
    }
    return {};
}
} // namespace

bool ThermostatDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                   ThermostatDeviceConfigV1& config, DeviceCreateRequest& request,
                                                   const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool ThermostatDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                   ThermostatDeviceConfigV1& config, DeviceConfigUpdateRequest& request,
                                                   const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult ThermostatDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                         const DeviceRegistry& registry) const {
    if (request.dependencyCount() != 2U) {
        return {DeviceError::InvalidRelationship, "thermostat requires exactly two deps"};
    }
    const DeviceDependencyLink* tempLink = nullptr;
    const DeviceDependencyLink* switchLink = nullptr;
    for (uint8_t index = 0; index < request.dependencyCount(); ++index) {
        const DeviceDependencyLink& link = request.dependencyLinks()[index];
        if (link.role == DeviceRole::TemperatureSensor) {
            tempLink = &link;
        } else if (link.role == DeviceRole::Switch) {
            switchLink = &link;
        }
    }
    if (tempLink == nullptr || switchLink == nullptr) {
        return {DeviceError::InvalidRelationship, "thermostat requires temperature_sensor and switch deps"};
    }

    ThermostatDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceValidationResult tempResult = validateCapability(registry, DeviceRole::TemperatureSensor, tempLink->deviceId);
    if (!tempResult.ok()) {
        return tempResult;
    }
    const DeviceValidationResult switchResult = validateCapability(registry, DeviceRole::Switch, switchLink->deviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    return config.validate();
}

DeviceValidationResult ThermostatDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                               const DeviceConfigUpdateRequest& request,
                                                                               const DeviceRegistry& registry) const {
    ThermostatDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    DeviceId temperatureDeviceId = runtime.dependencyDeviceId(DeviceRole::TemperatureSensor);
    DeviceId switchDeviceId = runtime.dependencyDeviceId(DeviceRole::Switch);
    if (request.depsProvided) {
        temperatureDeviceId = 0;
        switchDeviceId = 0;
        for (uint8_t index = 0; index < request.depCount; ++index) {
            const DeviceDependencyLink& link = request.deps[index];
            if (link.role == DeviceRole::TemperatureSensor) {
                temperatureDeviceId = link.deviceId;
            } else if (link.role == DeviceRole::Switch) {
                switchDeviceId = link.deviceId;
            }
        }
    }

    if (temperatureDeviceId == 0U || switchDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "thermostat requires temperature_sensor and switch deps"};
    }

    const DeviceValidationResult tempResult = validateCapability(registry, DeviceRole::TemperatureSensor, temperatureDeviceId);
    if (!tempResult.ok()) {
        return tempResult;
    }
    const DeviceValidationResult switchResult = validateCapability(registry, DeviceRole::Switch, switchDeviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    return config.validate();
}

DeviceValidationResult
ThermostatDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                   const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                   const DeviceRegistry& registry) const {
    (void)runtime;
    if (depCount != 2U) {
        return {DeviceError::InvalidRelationship, "thermostat requires exactly two deps"};
    }
    const DeviceDependencyLink* temperatureDependency = nullptr;
    const DeviceDependencyLink* switchDependency = nullptr;
    for (uint8_t index = 0; index < depCount; ++index) {
        const DeviceDependencyLink& link = deps[index];
        if (link.role == DeviceRole::TemperatureSensor) {
            temperatureDependency = &link;
        } else if (link.role == DeviceRole::Switch) {
            switchDependency = &link;
        }
    }
    if (temperatureDependency == nullptr || switchDependency == nullptr) {
        return {DeviceError::InvalidRelationship, "thermostat requires temperature_sensor and switch deps"};
    }
    const DeviceValidationResult tempResult = validateCapability(registry, DeviceRole::TemperatureSensor, temperatureDependency->deviceId);
    if (!tempResult.ok()) {
        return tempResult;
    }
    const DeviceValidationResult switchResult = validateCapability(registry, DeviceRole::Switch, switchDependency->deviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    return {};
}

void ThermostatDeviceApiAdapter::writeRuntimeJson(const ThermostatDevice& device, JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    JsonObject temperature = outputJson.createNestedObject("temperature");
    writeTemperatureOutputJson(device.latestTemperature(), TemperatureUnit::Celsius, device.controlStatus(), temperature);
    outputJson["desiredSwitchState"] = device.desiredOutputState();
    outputJson["actualSwitchState"] = device.actualOutputState();
    outputJson["lastCheckAtMs"] = device.lastCheckAtMs();
    outputJson["controlStatus"] = device.controlStatus();
}

} // namespace ewfm
