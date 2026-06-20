#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
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

bool DummyDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.persistencePolicy = parsePersistencePolicy(input);
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;

    DeviceBaseConfigV1 base{};
    if (!parseDeviceBaseConfigJson(input, base, error)) {
        return false;
    }
    request.name = base.name;
    request.enabled = base.enabled != 0U;

    DummyDeviceConfigV1 config{};
    const uint32_t configVersion = input["config_version"] | request.configVersion;
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (!configObject.isNull()) {
        if (!parseDummyDeviceConfigJson(configObject, configVersion, config, error)) {
            return false;
        }
    }

    config = base;
    request.configVersion = configVersion;
    if (configVersion != 1U) {
        error = "unsupported DummyDevice config version";
        return false;
    }

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dummyDeviceConfigSize(config);
    if (!encodeDummyDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode dummy config";
        return false;
    }
    return true;
}

void DummyDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, JsonObject output) const {
    writeCommonDeviceJson(runtime, typeName(), deviceStatusToString(runtime.status()),
                          persistencePolicyToString(runtime.persistencePolicy()), DummyDevice::descriptor().supportsRetainedState, output);
    static_cast<const DummyDevice&>(runtime).writeDeviceJson(output);
}

} // namespace ewfm
