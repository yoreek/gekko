#include "integrations/common/DeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
#include "integrations/rest/ds18b20/Ds18b20TemperatureSensorDeviceApiAdapter.h"
#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"
#include "integrations/rest/gpio_switch/GpioSwitchDeviceApiAdapter.h"
#include "integrations/rest/i2c_bus/I2cBusDeviceApiAdapter.h"
#include "integrations/rest/onewire_bus/OneWireBusDeviceApiAdapter.h"
#include "integrations/rest/thermostat/ThermostatDeviceApiAdapter.h"

namespace ewfm {

namespace {
const char* deviceStatusToString(const DeviceStatus status) {
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
} // namespace

void IDeviceApiAdapter::writeCommonDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, const char* typeName,
                                              JsonObject output) {
    JsonObject record = output.createNestedObject("record");
    record["id"] = runtime.deviceId();
    record["typeName"] = typeName;
    record["configRevision"] = runtime.configRevision();

    JsonObject config = output.createNestedObject("config");
    config["name"] = JsonString(runtime.name() != nullptr ? runtime.name() : "", JsonString::Copied);
    config["enabled"] = runtime.enabled();
    JsonArray deps = config.createNestedArray("deps");
    const DeviceDependencyLink* dependencyLinks = runtime.dependencyLinks();
    const uint8_t dependencyCount = runtime.dependencyCount();
    for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
        JsonObject item = deps.createNestedObject();
        item["role"] = deviceDependencyRoleName(dependencyLinks[index].role);
        item["deviceId"] = dependencyLinks[index].deviceId;
    }

    JsonObject runtimeJson = output.createNestedObject("runtime");
    runtimeJson["status"] = deviceStatusToString(runtime.status());
    runtimeJson["effectiveStatus"] = deviceStatusToString(effectiveStatus);
}

bool IDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime,
                                                 DeviceConfigUpdateRequest& request, const char*& error) const {
    (void)runtime;
    request = {};
    if (input["config"].isNull()) {
        error = "typed config update requires a config object";
        return false;
    }
    error = "typed config update is unsupported";
    return false;
}

DeviceValidationResult IDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const {
    (void)request;
    (void)registry;
    return {};
}

DeviceValidationResult IDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                      const DeviceConfigUpdateRequest& request,
                                                                      const DeviceRegistry& registry) const {
    (void)runtime;
    (void)request;
    (void)registry;
    return {};
}

DeviceValidationResult IDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                 const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                 uint8_t depCount, const DeviceRegistry& registry) const {
    (void)runtime;
    (void)deps;
    (void)depCount;
    (void)registry;
    return {};
}

bool DeviceApiAdapterRegistry::registerAdapter(const IDeviceApiAdapter& adapter) {
    if (adapter.typeId() == 0 || adapter.typeName() == nullptr) {
        return false;
    }
    if (find(adapter.typeId()) != nullptr || findByName(adapter.typeName()) != nullptr) {
        return false;
    }
    adapters_.push_back(&adapter);
    return true;
}

const IDeviceApiAdapter* DeviceApiAdapterRegistry::find(DeviceTypeId typeId) const {
    for (const auto* adapter : adapters_) {
        if (adapter != nullptr && adapter->typeId() == typeId) {
            return adapter;
        }
    }
    return nullptr;
}

const IDeviceApiAdapter* DeviceApiAdapterRegistry::findByName(const char* name) const {
    if (name == nullptr) {
        return nullptr;
    }
    for (const auto* adapter : adapters_) {
        if (adapter != nullptr && adapter->typeName() != nullptr && std::strcmp(adapter->typeName(), name) == 0) {
            return adapter;
        }
    }
    return nullptr;
}

DeviceApiAdapterRegistry DeviceApiAdapterRegistry::withDefaults() {
    DeviceApiAdapterRegistry registry;
    (void)registry.registerAdapter(DummyDeviceApiAdapter::instance());
    (void)registry.registerAdapter(GpioSwitchDeviceApiAdapter::instance());
    (void)registry.registerAdapter(OneWireBusDeviceApiAdapter::instance());
    (void)registry.registerAdapter(I2cBusDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Ds18b20TemperatureSensorDeviceApiAdapter::instance());
    (void)registry.registerAdapter(ThermostatDeviceApiAdapter::instance());
    return registry;
}

} // namespace ewfm
