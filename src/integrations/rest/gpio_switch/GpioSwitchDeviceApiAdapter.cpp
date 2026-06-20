#include "integrations/rest/gpio_switch/GpioSwitchDeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
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

bool GpioSwitchDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.persistencePolicy = parsePersistencePolicy(input);
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;

    DeviceBaseConfigV1 base{};
    if (!parseDeviceBaseConfigJson(input, base, error)) {
        return false;
    }
    request.name = base.name;
    request.enabled = base.enabled != 0U;

    GpioSwitchDevicePersistedConfigV1 config{};
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (!configObject.isNull()) {
        if (!parseGpioSwitchDeviceConfigJson(configObject, config, error)) {
            return false;
        }
    }

    config.switchConfig.base = base;
    if (!validateSwitchDeviceConfig(config.switchConfig).ok()) {
        error = "gpio switch config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = gpioSwitchDeviceConfigSize(config);
    if (!encodeGpioSwitchDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode gpio switch config";
        return false;
    }
    return true;
}

bool GpioSwitchDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime,
                                                          DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    if (configObject.isNull()) {
        error = "gpio switch config is required";
        return false;
    }

    DeviceBaseConfigV1 base{};
    base.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(base.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }

    GpioSwitchDevicePersistedConfigV1 config{};
    if (!parseGpioSwitchDeviceConfigJson(configObject, config, error)) {
        return false;
    }
    config.switchConfig.base = base;
    if (!validateSwitchDeviceConfig(config.switchConfig).ok()) {
        error = "gpio switch config is invalid";
        return false;
    }

    request = {};
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = gpioSwitchDeviceConfigSize(config);
    if (!encodeGpioSwitchDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode gpio switch config";
        return false;
    }
    return true;
}

void GpioSwitchDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, JsonObject output) const {
    writeCommonDeviceJson(runtime, typeName(), deviceStatusToString(runtime.status()),
                          persistencePolicyToString(runtime.persistencePolicy()), GpioSwitchDevice::descriptor().supportsRetainedState,
                          output);
    static_cast<const GpioSwitchDevice&>(runtime).writeDeviceJson(output);
}

} // namespace ewfm
