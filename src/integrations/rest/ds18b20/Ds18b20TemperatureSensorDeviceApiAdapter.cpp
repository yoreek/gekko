#include "integrations/rest/ds18b20/Ds18b20TemperatureSensorDeviceApiAdapter.h"

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

bool parseParentFields(const JsonObjectConst& input, bool requireParent, bool& hasParent, DeviceId& parentDeviceId, std::string& error) {
    hasParent = input["has_parent"] | true;
    parentDeviceId = static_cast<DeviceId>(input["parent_device_id"] | 0U);
    if (requireParent && (!hasParent || parentDeviceId == 0U)) {
        error = "ds18b20 parent is required";
        return false;
    }
    return true;
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
                                                                  std::string& error) const {
    request = {};
    request.typeId = typeId();
    request.name = input["name"] | "";
    request.enabled = input["enabled"] | true;
    request.persistencePolicy = parsePersistencePolicy(input);
    request.configVersion = kDs18b20TemperatureSensorConfigVersion;

    if (request.name.empty()) {
        error = "device name is required";
        return false;
    }
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
    config.enabled = request.enabled ? 1U : 0U;
    request.configPayload = encodeDs18b20TemperatureSensorConfig(config);
    return true;
}

bool Ds18b20TemperatureSensorDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const DeviceRecord& record,
                                                                        DeviceConfigUpdateRequest& request, std::string& error) const {
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (configObject.isNull()) {
        error = "ds18b20 config is required";
        return false;
    }

    Ds18b20TemperatureSensorConfigV1 config{};
    if (!parseDs18b20TemperatureSensorConfigJson(configObject, config, error)) {
        return false;
    }
    config.enabled = record.enabled ? 1U : 0U;

    request = {};
    request.configVersion = kDs18b20TemperatureSensorConfigVersion;
    request.configPayload = encodeDs18b20TemperatureSensorConfig(config);
    request.parentFieldsProvided = !input["has_parent"].isNull() || !input["parent_device_id"].isNull();
    if (request.parentFieldsProvided && !parseParentFields(input, true, request.hasParent, request.parentDeviceId, error)) {
        return false;
    }
    return true;
}

void Ds18b20TemperatureSensorDeviceApiAdapter::writeDeviceJson(const DeviceRecord& record, const IDeviceRuntime* runtime,
                                                               JsonObject output) const {
    output["device_id"] = record.header.deviceId;
    output["type_id"] = record.header.typeId;
    output["type"] = typeName();
    output["name"] = record.name;
    output["enabled"] = record.enabled;
    output["status"] = deviceStatusToString(record.status);
    output["config_version"] = record.header.configVersion;
    output["config_revision"] = record.header.configRevision;
    output["persistence_policy"] = persistencePolicyToString(record.persistencePolicy);
    output["has_parent"] = record.hasParent;
    output["parent_device_id"] = record.parentDeviceId;
    output["retained_state_supported"] = false;

    Ds18b20TemperatureSensorConfigV1 config{};
    const bool hasConfig = decodeDs18b20TemperatureSensorConfig(record.configPayload, config);
    if (hasConfig) {
        JsonObject configObject = output.createNestedObject("config");
        writeDs18b20TemperatureSensorConfigJson(config, configObject);
    }

    JsonObject outputObject = output.createNestedObject("output");
    JsonObject temperature = outputObject.createNestedObject("temperature");
    TemperatureReading reading{};
    const char* status = "not_ready";
    if (runtime != nullptr) {
        const auto* sensorRuntime = static_cast<const Ds18b20TemperatureSensorDevice*>(runtime);
        reading = sensorRuntime->reading();
        status = sensorRuntime->outputStatus();
    }
    TemperatureUnit unit{TemperatureUnit::Celsius};
    if (hasConfig) {
        (void)temperatureUnitFromByte(config.outputUnit, unit);
    }
    writeTemperatureOutputJson(reading, unit, status, temperature);
}

} // namespace ewfm
