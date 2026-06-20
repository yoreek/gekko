#include "integrations/rest/onewire_bus/OneWireBusDeviceApiAdapter.h"

#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/core/DeviceBaseConfig.h"

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

bool OneWireBusDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.persistencePolicy = parsePersistencePolicy(input);
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;

    DeviceBaseConfigV1 base{};
    if (!parseDeviceBaseConfigJson(input, base, error)) {
        return false;
    }
    request.name = base.name;
    request.enabled = base.enabled != 0U;

    OneWireBusDeviceConfigV1 config{};
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (!configObject.isNull()) {
        if (!parseOneWireBusDeviceConfigJson(configObject, config, error)) {
            return false;
        }
    }

    config.base = base;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oneWireBusDeviceConfigSize(config);
    if (!encodeOneWireBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode onewire bus config";
        return false;
    }
    return true;
}

bool OneWireBusDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime,
                                                          DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (configObject.isNull()) {
        error = "onewire bus config is required";
        return false;
    }

    DeviceBaseConfigV1 base{};
    base.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(base.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }

    OneWireBusDeviceConfigV1 config{};
    if (!parseOneWireBusDeviceConfigJson(configObject, config, error)) {
        return false;
    }
    config.base = base;

    request = {};
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oneWireBusDeviceConfigSize(config);
    if (!encodeOneWireBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode onewire bus config";
        return false;
    }
    return true;
}

void OneWireBusDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, JsonObject output) const {
    writeCommonDeviceJson(runtime, typeName(), deviceStatusToString(runtime.status()),
                          persistencePolicyToString(runtime.persistencePolicy()), OneWireBusDevice::descriptor().supportsRetainedState,
                          output);
    static_cast<const OneWireBusDevice&>(runtime).writeDeviceJson(output);
}

} // namespace ewfm
