#include "integrations/rest/onewire_bus/OneWireBusDeviceApiAdapter.h"

#include "devices/bus/onewire/OneWireBusDevice.h"

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

void writeScanDevice(JsonArray array, const OneWireRomAddress& address) {
    char rom[17]{};
    char family[3]{};
    (void)formatOneWireRomAddress(address, rom);
    family[0] = "0123456789ABCDEF"[(address.bytes[0] >> 4) & 0x0F];
    family[1] = "0123456789ABCDEF"[address.bytes[0] & 0x0F];
    family[2] = '\0';

    JsonObject item = array.createNestedObject();
    item["address"] = rom;
    item["family_code"] = family;
}
} // namespace

const OneWireBusDeviceApiAdapter& OneWireBusDeviceApiAdapter::instance() {
    static const OneWireBusDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId OneWireBusDeviceApiAdapter::typeId() const {
    return OneWireBusDevice::descriptor().typeId;
}

const char* OneWireBusDeviceApiAdapter::typeName() const {
    return "onewire_bus";
}

bool OneWireBusDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, std::string& error) const {
    request = {};
    request.typeId = typeId();
    request.name = input["name"] | "";
    request.enabled = input["enabled"] | true;
    request.persistencePolicy = parsePersistencePolicy(input);
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;

    if (request.name.empty()) {
        error = "device name is required";
        return false;
    }

    OneWireBusDeviceConfigV1 config{};
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (!configObject.isNull()) {
        if (!parseOneWireBusDeviceConfigJson(configObject, config, error)) {
            return false;
        }
    }

    config.enabled = request.enabled ? 1U : 0U;
    request.configPayload = encodeOneWireBusDeviceConfig(config);
    return true;
}

bool OneWireBusDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const DeviceRecord& record,
                                                          DeviceConfigUpdateRequest& request, std::string& error) const {
    (void)record;
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (configObject.isNull()) {
        error = "onewire bus config is required";
        return false;
    }

    OneWireBusDeviceConfigV1 config{};
    if (!parseOneWireBusDeviceConfigJson(configObject, config, error)) {
        return false;
    }

    request = {};
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;
    request.configPayload = encodeOneWireBusDeviceConfig(config);
    return true;
}

void OneWireBusDeviceApiAdapter::writeDeviceJson(const DeviceRecord& record, const IDeviceRuntime* runtime, JsonObject output) const {
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
    output["retained_state_supported"] = OneWireBusDevice::descriptor().supportsRetainedState;

    OneWireBusDeviceConfigV1 config{};
    if (decodeOneWireBusDeviceConfig(record.configPayload, config)) {
        JsonObject configObject = output.createNestedObject("config");
        writeOneWireBusDeviceConfigJson(config, configObject);
    }

    if (runtime != nullptr) {
        const auto* oneWireRuntime = static_cast<const OneWireBusDevice*>(runtime);
        const OneWireScanResult& scan = oneWireRuntime->scan();
        JsonObject scanObject = output.createNestedObject("scan");
        scanObject["in_progress"] = scan.inProgress;
        scanObject["ready"] = scan.ready;
        scanObject["device_count"] = scan.deviceCount;
        scanObject["truncated"] = scan.truncated;
        scanObject["invalid_crc_seen"] = scan.invalidCandidateSeen;
        JsonArray devices = scanObject.createNestedArray("devices");
        for (uint8_t index = 0; index < scan.deviceCount; ++index) {
            writeScanDevice(devices, scan.devices[index]);
        }
    }
}

} // namespace ewfm
