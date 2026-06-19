#include "integrations/rest/gpio_switch/GpioSwitchDeviceApiAdapter.h"

#include "devices/switch/gpio/GpioSwitchDevice.h"

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

const GpioSwitchDeviceApiAdapter& GpioSwitchDeviceApiAdapter::instance() {
    static const GpioSwitchDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId GpioSwitchDeviceApiAdapter::typeId() const {
    return GpioSwitchDevice::descriptor().typeId;
}

const char* GpioSwitchDeviceApiAdapter::typeName() const {
    return "gpio_switch";
}

bool GpioSwitchDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, std::string& error) const {
    request = {};
    request.typeId = typeId();
    request.name = input["name"] | "";
    request.enabled = input["enabled"] | true;
    request.persistencePolicy = parsePersistencePolicy(input);
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;

    if (request.name.empty()) {
        error = "device name is required";
        return false;
    }

    GpioSwitchDeviceConfigV1 config{};
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (!configObject.isNull()) {
        if (!parseGpioSwitchDeviceConfigJson(configObject, config, error)) {
            return false;
        }
    }

    config.enabled = request.enabled ? 1U : 0U;
    request.configPayload = encodeGpioSwitchDeviceConfig(config);
    return true;
}

bool GpioSwitchDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const DeviceRecord& record,
                                                          DeviceConfigUpdateRequest& request, std::string& error) const {
    (void)record;
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (configObject.isNull()) {
        error = "gpio switch config is required";
        return false;
    }

    GpioSwitchDeviceConfigV1 config{};
    if (!parseGpioSwitchDeviceConfigJson(configObject, config, error)) {
        return false;
    }

    request = {};
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    request.configPayload = encodeGpioSwitchDeviceConfig(config);
    return true;
}

void GpioSwitchDeviceApiAdapter::writeDeviceJson(const DeviceRecord& record, const IDeviceRuntime* runtime, JsonObject output) const {
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
    output["retained_state_supported"] = GpioSwitchDevice::descriptor().supportsRetainedState;

    GpioSwitchDeviceConfigV1 config{};
    if (decodeGpioSwitchDeviceConfig(record.configPayload, config)) {
        JsonObject configObject = output.createNestedObject("config");
        writeGpioSwitchDeviceConfigJson(config, configObject);
        output["retained_startup_enabled"] = config.restorePreviousState != 0U;
        output["retained_state_in_config_payload"] = false;
    }

    if (runtime != nullptr) {
        const auto* gpioRuntime = static_cast<const GpioSwitchDevice*>(runtime);
        JsonObject outputObject = output.createNestedObject("output");
        outputObject["state"] = outputStateName(gpioRuntime->outputState());
    }
}

} // namespace ewfm
