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

bool parseParentFields(const JsonObjectConst& input, bool requireParent, bool& hasParent, DeviceId& parentDeviceId, const char*& error) {
    hasParent = input["has_parent"] | true;
    parentDeviceId = static_cast<DeviceId>(input["parent_device_id"] | 0U);
    if (requireParent && (!hasParent || parentDeviceId == 0U)) {
        error = "ds18b20 parent is required";
        return false;
    }
    return true;
}

DeviceValidationResult validateUniqueParentAddress(const DeviceRegistry& registry, const IDeviceRuntime* childRuntime,
                                                   const OneWireRomAddress& address, bool hasParent, DeviceId parentDeviceId) {
    if (!hasParent || parentDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "ds18b20 requires a onewire parent"};
    }

    const IDeviceRuntime* parentRuntime = registry.runtime(parentDeviceId);
    if (parentRuntime == nullptr) {
        return {DeviceError::InvalidRelationship, "ds18b20 parent is missing or invalid"};
    }
    if (parentRuntime->hasDuplicateChildRomAddress(address, childRuntime)) {
        return {DeviceError::InvalidRelationship, "duplicate ds18b20 address on parent"};
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
    if (!parseParentFields(input, true, request.hasParent, request.parentDeviceId, error)) {
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
    request.parentFieldsProvided = !input["has_parent"].isNull() || !input["parent_device_id"].isNull();
    if (request.parentFieldsProvided && !parseParentFields(input, true, request.hasParent, request.parentDeviceId, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult Ds18b20TemperatureSensorDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                                       const DeviceRegistry& registry) const {
    if (!request.hasParent || request.parentDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "ds18b20 requires a onewire parent"};
    }

    Ds18b20TemperatureSensorConfigV1 config{};
    if (!decodeDs18b20TemperatureSensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                              config)) {
        return {DeviceError::InvalidConfig, "ds18b20 config is invalid"};
    }

    return validateUniqueParentAddress(registry, nullptr, config.address, request.hasParent, request.parentDeviceId);
}

DeviceValidationResult Ds18b20TemperatureSensorDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                             const DeviceConfigUpdateRequest& request,
                                                                                             const DeviceRegistry& registry) const {
    Ds18b20TemperatureSensorConfigV1 config{};
    if (!decodeDs18b20TemperatureSensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                              config)) {
        return {DeviceError::InvalidConfig, "ds18b20 config is invalid"};
    }

    const bool hasParent = request.parentFieldsProvided ? request.hasParent : runtime.hasParent();
    const DeviceId parentDeviceId = request.parentFieldsProvided ? request.parentDeviceId : runtime.parentDeviceId();
    return validateUniqueParentAddress(registry, &runtime, config.address, hasParent, parentDeviceId);
}

DeviceValidationResult Ds18b20TemperatureSensorDeviceApiAdapter::validateSetParentRequest(const IDeviceRuntime& runtime, bool hasParent,
                                                                                          DeviceId parentDeviceId,
                                                                                          const DeviceRegistry& registry) const {
    OneWireRomAddress address{};
    if (!runtime.oneWireRomAddress(address)) {
        return {DeviceError::InvalidConfig, "ds18b20 config is invalid"};
    }
    return validateUniqueParentAddress(registry, &runtime, address, hasParent, parentDeviceId);
}

void Ds18b20TemperatureSensorDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, JsonObject output) const {
    writeCommonDeviceJson(runtime, typeName(), deviceStatusToString(runtime.status()),
                          persistencePolicyToString(runtime.persistencePolicy()), false, output);
    static_cast<const Ds18b20TemperatureSensorDevice&>(runtime).writeDeviceJson(output);
}

} // namespace ewfm
