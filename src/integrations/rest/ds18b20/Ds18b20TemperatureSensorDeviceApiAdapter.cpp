#include "integrations/rest/ds18b20/Ds18b20TemperatureSensorDeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"

#include <cstring>

namespace ewfm {

namespace {
const char* deviceStatusToString(DeviceStatus status) {
    switch (status) {
    case DeviceStatus::Creating:
        return "creating";
    case DeviceStatus::Starting:
        return "starting";
    case DeviceStatus::Ready:
        return "ready";
    case DeviceStatus::Disabled:
        return "disabled";
    case DeviceStatus::Faulted:
        return "faulted";
    case DeviceStatus::DependencyBlocked:
        return "dependency_blocked";
    case DeviceStatus::Reconfiguring:
        return "reconfiguring";
    case DeviceStatus::Stopping:
        return "stopping";
    case DeviceStatus::Deleting:
        return "deleting";
    case DeviceStatus::Unknown:
    default:
        return "unknown";
    }
}

const char* persistencePolicyToString(DevicePersistencePolicy policy) {
    switch (policy) {
    case DevicePersistencePolicy::Immediate:
        return "immediate";
    case DevicePersistencePolicy::Delayed:
        return "delayed";
    case DevicePersistencePolicy::Coalesced:
        return "coalesced";
    }
    return "delayed";
}

DevicePersistencePolicy parsePersistencePolicy(const JsonObjectConst& input) {
    const char* policy = input["persistence_policy"] | "immediate";
    if (std::strcmp(policy, "delayed") == 0) {
        return DevicePersistencePolicy::Delayed;
    }
    if (std::strcmp(policy, "coalesced") == 0) {
        return DevicePersistencePolicy::Coalesced;
    }
    return DevicePersistencePolicy::Immediate;
}

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
        const DeviceId deviceId = static_cast<DeviceId>(item["device_id"] | 0U);
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
    if (dependencyRuntime->hasDuplicateChildRomAddress(address, childRuntime)) {
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
    request.persistencePolicy = parsePersistencePolicy(input);
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
    if (configObject.isNull()) {
        error = "ds18b20 config is required";
        return false;
    }

    Ds18b20TemperatureSensorConfigV1 config{};
    if (!parseDs18b20TemperatureSensorConfigJson(configObject, config, error)) {
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
    if (configObject.isNull()) {
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
    if (!parseDs18b20TemperatureSensorConfigJson(configObject, config, error)) {
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

void Ds18b20TemperatureSensorDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, JsonObject output) const {
    writeCommonDeviceJson(runtime, typeName(), deviceStatusToString(runtime.status()),
                          persistencePolicyToString(runtime.persistencePolicy()), false, output);
    static_cast<const Ds18b20TemperatureSensorDevice&>(runtime).writeDeviceJson(output);
}

} // namespace ewfm
