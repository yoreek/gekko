#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"

#include "devices/dummy/DummyDevice.h"

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

} // namespace

const DummyDeviceApiAdapter& DummyDeviceApiAdapter::instance() {
    static const DummyDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId DummyDeviceApiAdapter::typeId() const {
    return DummyDevice::descriptor().typeId;
}

const char* DummyDeviceApiAdapter::typeName() const {
    return "dummy";
}

bool DummyDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, std::string& error) const {
    request = {};
    request.typeId = typeId();
    request.name = input["name"] | "";
    request.enabled = input["enabled"] | true;
    request.persistencePolicy = parsePersistencePolicy(input);
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;

    if (request.name.empty()) {
        error = "device name is required";
        return false;
    }

    DummyDeviceConfigV2 config{};
    const uint32_t configVersion = input["config_version"] | request.configVersion;
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (!configObject.isNull()) {
        if (!parseDummyDeviceConfigJson(configObject, configVersion, config, error)) {
            return false;
        }
    } else {
        config.enabled = request.enabled;
    }

    config.enabled = request.enabled;
    request.configVersion = configVersion;
    if (configVersion == 1U) {
        DummyDeviceConfigV1 legacy{};
        legacy.enabled = config.enabled;
        legacy.restorePreviousState = config.restorePreviousState;
        legacy.defaultOutput = config.defaultOutput;
        legacy.currentOutput = config.currentOutput;
        request.configPayload = encodeDummyDeviceConfig(legacy);
    } else {
        request.configPayload = encodeDummyDeviceConfig(config);
    }
    return true;
}

void DummyDeviceApiAdapter::writeDeviceJson(const DeviceRecord& record, const IDeviceRuntime* runtime, JsonObject output) const {
    (void)runtime;
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
    output["retained_state_supported"] = DummyDevice::descriptor().supportsRetainedState;

    DummyDeviceConfigV2 config{};
    if (decodeDummyDeviceConfig(record.configPayload, config)) {
        JsonObject configObject = output.createNestedObject("config");
        writeDummyDeviceConfigJson(config, configObject);
        output["retained_startup_enabled"] = config.restorePreviousState != 0U;
        output["retained_startup_fallback_output"] = config.defaultOutput != 0U;
        output["retained_state_in_config_payload"] = false;
    }
}

} // namespace ewfm
