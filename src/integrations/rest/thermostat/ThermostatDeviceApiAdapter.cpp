#include "integrations/rest/thermostat/ThermostatDeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/thermostat/ThermostatDevice.h"

namespace ewfm {

namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    depCount = 0;
    const JsonArrayConst depsArray = input["deps"].as<JsonArrayConst>();
    if (depsArray.isNull()) {
        error = "thermostat deps are required";
        return false;
    }
    bool hasTemperatureSensor = false;
    bool hasSwitch = false;
    for (JsonObjectConst item : depsArray) {
        if (depCount >= kMaxDeviceDependencies) {
            error = "thermostat deps exceed supported count";
            return false;
        }
        DeviceDependencyRole role{DeviceDependencyRole::Unknown};
        if (!parseDeviceDependencyRole(item["role"] | "", role)) {
            error = "thermostat dependency role is invalid";
            return false;
        }
        const DeviceId deviceId = static_cast<DeviceId>(item["deviceId"] | 0U);
        if (deviceId == 0U) {
            error = "thermostat dependency device id is required";
            return false;
        }
        if (role == DeviceDependencyRole::TemperatureSensor) {
            hasTemperatureSensor = true;
        } else if (role == DeviceDependencyRole::Switch) {
            hasSwitch = true;
        } else {
            error = "thermostat dependency role is invalid";
            return false;
        }
        deps[depCount++] = DeviceDependencyLink{role, deviceId};
    }
    if (!hasTemperatureSensor || !hasSwitch) {
        error = "thermostat requires temperature_sensor and switch deps";
        return false;
    }
    return true;
}

DeviceValidationResult validateCapability(const DeviceRegistry& registry, DeviceDependencyRole role, DeviceId deviceId) {
    const IDeviceRuntime* dependency = registry.runtime(deviceId);
    if (dependency == nullptr) {
        return {DeviceError::InvalidRelationship, "thermostat dependency is missing"};
    }
    if (role == DeviceDependencyRole::TemperatureSensor) {
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
    const OutputStateMask mask = switchOutput->supportedOutputStateMask();
    if (!outputStateIsSupported(OutputState::Off, mask) || !outputStateIsSupported(OutputState::On, mask)) {
        return {DeviceError::InvalidRelationship, "switch dependency does not support on/off output"};
    }
    return {};
}
} // namespace

const ThermostatDeviceApiAdapter& ThermostatDeviceApiAdapter::instance() {
    static const ThermostatDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId ThermostatDeviceApiAdapter::typeId() const {
    return kThermostatDeviceTypeId;
}

const char* ThermostatDeviceApiAdapter::typeName() const {
    return "thermostat";
}

bool ThermostatDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = kThermostatDeviceConfigVersion;

    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "thermostat config is required";
        return false;
    }

    ThermostatDeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;
    if (!parseDepsField(configInput, request.deps, request.depCount, error)) {
        return false;
    }
    if (!config.validate().ok()) {
        error = "thermostat config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = thermostatDeviceConfigSize(config);
    if (!encodeThermostatDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode thermostat config";
        return false;
    }
    return true;
}

bool ThermostatDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                          DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "thermostat config is required";
        return false;
    }

    ThermostatDeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    config.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(config.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }
    if (!config.validate().ok()) {
        error = "thermostat config is invalid";
        return false;
    }

    request = {};
    request.configVersion = kThermostatDeviceConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = thermostatDeviceConfigSize(config);
    if (!encodeThermostatDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode thermostat config";
        return false;
    }
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
        if (link.role == DeviceDependencyRole::TemperatureSensor) {
            tempLink = &link;
        } else if (link.role == DeviceDependencyRole::Switch) {
            switchLink = &link;
        }
    }
    if (tempLink == nullptr || switchLink == nullptr) {
        return {DeviceError::InvalidRelationship, "thermostat requires temperature_sensor and switch deps"};
    }

    ThermostatDeviceConfigV1 config{};
    if (!decodeThermostatDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "thermostat config is invalid"};
    }

    const DeviceValidationResult tempResult = validateCapability(registry, DeviceDependencyRole::TemperatureSensor, tempLink->deviceId);
    if (!tempResult.ok()) {
        return tempResult;
    }
    const DeviceValidationResult switchResult = validateCapability(registry, DeviceDependencyRole::Switch, switchLink->deviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    return config.validate();
}

DeviceValidationResult ThermostatDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                               const DeviceConfigUpdateRequest& request,
                                                                               const DeviceRegistry& registry) const {
    ThermostatDeviceConfigV1 config{};
    if (!decodeThermostatDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "thermostat config is invalid"};
    }

    DeviceId temperatureDeviceId = runtime.dependencyDeviceId(DeviceDependencyRole::TemperatureSensor);
    DeviceId switchDeviceId = runtime.dependencyDeviceId(DeviceDependencyRole::Switch);
    if (request.depsProvided) {
        temperatureDeviceId = 0;
        switchDeviceId = 0;
        for (uint8_t index = 0; index < request.depCount; ++index) {
            const DeviceDependencyLink& link = request.deps[index];
            if (link.role == DeviceDependencyRole::TemperatureSensor) {
                temperatureDeviceId = link.deviceId;
            } else if (link.role == DeviceDependencyRole::Switch) {
                switchDeviceId = link.deviceId;
            }
        }
    }

    if (temperatureDeviceId == 0U || switchDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "thermostat requires temperature_sensor and switch deps"};
    }

    const DeviceValidationResult tempResult = validateCapability(registry, DeviceDependencyRole::TemperatureSensor, temperatureDeviceId);
    if (!tempResult.ok()) {
        return tempResult;
    }
    const DeviceValidationResult switchResult = validateCapability(registry, DeviceDependencyRole::Switch, switchDeviceId);
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
        if (link.role == DeviceDependencyRole::TemperatureSensor) {
            temperatureDependency = &link;
        } else if (link.role == DeviceDependencyRole::Switch) {
            switchDependency = &link;
        }
    }
    if (temperatureDependency == nullptr || switchDependency == nullptr) {
        return {DeviceError::InvalidRelationship, "thermostat requires temperature_sensor and switch deps"};
    }
    const DeviceValidationResult tempResult =
        validateCapability(registry, DeviceDependencyRole::TemperatureSensor, temperatureDependency->deviceId);
    if (!tempResult.ok()) {
        return tempResult;
    }
    const DeviceValidationResult switchResult = validateCapability(registry, DeviceDependencyRole::Switch, switchDependency->deviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    return {};
}

void ThermostatDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                 JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const ThermostatDevice& device = static_cast<const ThermostatDevice&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    device.config().writeJson(config);

    JsonObject runtimeJson = output["runtime"].as<JsonObject>();
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    JsonObject temperature = outputJson.createNestedObject("temperature");
    writeTemperatureOutputJson(device.latestTemperature(), TemperatureUnit::Celsius, device.controlStatus(), temperature);
    outputJson["desiredSwitchState"] = outputStateName(device.desiredOutputState());
    outputJson["actualSwitchState"] = outputStateName(device.actualOutputState());
    outputJson["lastCheckAtMs"] = device.lastCheckAtMs();
    outputJson["controlStatus"] = device.controlStatus();
}

} // namespace ewfm
