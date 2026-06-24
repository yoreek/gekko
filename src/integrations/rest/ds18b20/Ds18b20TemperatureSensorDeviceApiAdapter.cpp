#include "integrations/rest/ds18b20/Ds18b20TemperatureSensorDeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"

namespace ewfm {
namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    depCount = 0;
    const JsonArrayConst depsArray = input["deps"].as<JsonArrayConst>();
    if (depsArray.isNull()) {
        error = "ds18b20 deps are required";
        return false;
    }
    for (JsonObjectConst item : depsArray) {
        if (depCount >= kMaxDeviceDependencies) {
            error = "ds18b20 deps exceed supported count";
            return false;
        }
        DeviceDependencyRole role{DeviceDependencyRole::Unknown};
        if (!parseDeviceDependencyRole(item["role"] | "", role)) {
            error = "ds18b20 dependency role is invalid";
            return false;
        }
        const DeviceId deviceId = static_cast<DeviceId>(item["deviceId"] | 0U);
        if (deviceId == 0U) {
            error = "ds18b20 dependency device id is required";
            return false;
        }
        deps[depCount++] = DeviceDependencyLink{role, deviceId};
    }
    return true;
}

DeviceId onewireBusDependencyId(const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount) {
    for (uint8_t index = 0; index < depCount; ++index) {
        if (deps[index].role == DeviceDependencyRole::OneWireBus) {
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

const Ds18b20TemperatureSensorDeviceApiAdapter& Ds18b20TemperatureSensorDeviceApiAdapter::instance() {
    static const Ds18b20TemperatureSensorDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId Ds18b20TemperatureSensorDeviceApiAdapter::typeId() const {
    return kDs18b20TemperatureSensorTypeId;
}

const char* Ds18b20TemperatureSensorDeviceApiAdapter::typeName() const {
    return "ds18b20_temperature_sensor";
}

bool Ds18b20TemperatureSensorDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                                                  const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = kDs18b20TemperatureSensorConfigVersion;

    DeviceBaseConfigV1 base{};
    if (!parseDeviceBaseConfigJson(input, base, error)) {
        return false;
    }
    request.name = base.name;
    request.enabled = base.enabled != 0U;
    if (!parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }

    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    const JsonObjectConst configInput = configObject.isNull() ? input : configObject;
    Ds18b20TemperatureSensorConfigV1 config{};
    if (!parseDs18b20TemperatureSensorConfigJson(configInput, config, error)) {
        return false;
    }
    config.base = base;
    if (!validateDs18b20TemperatureSensorConfig(config).ok()) {
        error = "ds18b20 config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ds18b20TemperatureSensorConfigSize(config);
    if (!encodeDs18b20TemperatureSensorConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode ds18b20 config";
        return false;
    }
    return true;
}

bool Ds18b20TemperatureSensorDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime,
                                                                        DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    const JsonObjectConst configInput = configObject.isNull() ? input : configObject;
    if (configObject.isNull() && input["address"].isNull() && input["resolution"].isNull() && input["unit"].isNull() &&
        input["pollMs"].isNull() && input["reportDeltaCelsius"].isNull() && input["reportDeltaCentiCelsius"].isNull() &&
        input["reportAlways"].isNull()) {
        error = "ds18b20 config is required";
        return false;
    }

    DeviceBaseConfigV1 base{};
    base.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(base.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }

    Ds18b20TemperatureSensorConfigV1 config{};
    if (!parseDs18b20TemperatureSensorConfigJson(configInput, config, error)) {
        return false;
    }
    config.base = base;
    if (!validateDs18b20TemperatureSensorConfig(config).ok()) {
        error = "ds18b20 config is invalid";
        return false;
    }

    request = {};
    request.configVersion = kDs18b20TemperatureSensorConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ds18b20TemperatureSensorConfigSize(config);
    if (!encodeDs18b20TemperatureSensorConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode ds18b20 config";
        return false;
    }
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
    if (!decodeDs18b20TemperatureSensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                              config)) {
        return {DeviceError::InvalidConfig, "ds18b20 config is invalid"};
    }

    return validateUniqueDependencyAddress(registry, nullptr, config.address, dependencyDeviceId);
}

DeviceValidationResult Ds18b20TemperatureSensorDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                             const DeviceConfigUpdateRequest& request,
                                                                                             const DeviceRegistry& registry) const {
    Ds18b20TemperatureSensorConfigV1 config{};
    if (!decodeDs18b20TemperatureSensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                              config)) {
        return {DeviceError::InvalidConfig, "ds18b20 config is invalid"};
    }

    const DeviceId dependencyDeviceId = request.depsProvided ? onewireBusDependencyId(request.deps, request.depCount)
                                                             : runtime.dependencyDeviceId(DeviceDependencyRole::OneWireBus);
    return validateUniqueDependencyAddress(registry, &runtime, config.address, dependencyDeviceId);
}

DeviceValidationResult
Ds18b20TemperatureSensorDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                 const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                 uint8_t depCount, const DeviceRegistry& registry) const {
    OneWireRomAddress address{};
    if (!runtime.oneWireRomAddress(address)) {
        return {DeviceError::InvalidConfig, "ds18b20 config is invalid"};
    }
    return validateUniqueDependencyAddress(registry, &runtime, address, onewireBusDependencyId(deps, depCount));
}

void Ds18b20TemperatureSensorDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                               JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const Ds18b20TemperatureSensorDevice& device = static_cast<const Ds18b20TemperatureSensorDevice&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    writeDs18b20TemperatureSensorConfigJson(device.config(), config);

    JsonObject runtimeJson = output["runtime"].as<JsonObject>();
    JsonObject temperature = runtimeJson.createNestedObject("temperature");
    writeTemperatureOutputJson(device.reading(),
                               device.config().outputUnit == temperatureUnitToByte(TemperatureUnit::Fahrenheit)
                                   ? TemperatureUnit::Fahrenheit
                                   : TemperatureUnit::Celsius,
                               device.outputStatus(), temperature);
    runtimeJson["consecutiveErrors"] = device.consecutiveErrors();
    runtimeJson["lastDependencyGeneration"] = device.lastDependencyGeneration();
}

} // namespace ewfm
