#include "integrations/common/DeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
#include "integrations/rest/aht10/Aht10SensorDeviceApiAdapter.h"
#include "integrations/rest/analog_input/Ads1115HubDeviceApiAdapter.h"
#include "integrations/rest/analog_input/AnalogInputChannelDeviceApiAdapter.h"
#include "integrations/rest/analog_input/AnalogPortInputDeviceApiAdapter.h"
#include "integrations/rest/analog_input/Cd74hc4067HubDeviceApiAdapter.h"
#include "integrations/rest/analog_output/AnalogOutputComposerDeviceApiAdapter.h"
#include "integrations/rest/analog_output/FadeAnalogOutputDeviceApiAdapter.h"
#include "integrations/rest/analog_output/LedcAnalogOutputDeviceApiAdapter.h"
#include "integrations/rest/analog_output/ScheduledAnalogOutputDeviceApiAdapter.h"
#include "integrations/rest/auto_switch/AutoSwitchDeviceApiAdapter.h"
#include "integrations/rest/binary_sensor/BinarySensorDeviceApiAdapter.h"
#include "integrations/rest/dht11/Dht11SensorDeviceApiAdapter.h"
#include "integrations/rest/dosing_pump/DosingPumpDeviceApiAdapter.h"
#include "integrations/rest/ds18b20/Ds18b20TemperatureSensorDeviceApiAdapter.h"
#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"
#include "integrations/rest/expander/Pcf8574ExpanderDeviceApiAdapter.h"
#include "integrations/rest/expander/Pcf8575ExpanderDeviceApiAdapter.h"
#include "integrations/rest/expander/PortExpanderSwitchDeviceApiAdapter.h"
#include "integrations/rest/gpio_switch/GpioSwitchDeviceApiAdapter.h"
#include "integrations/rest/htu21/Htu21SensorDeviceApiAdapter.h"
#include "integrations/rest/i2c_bus/I2cBusDeviceApiAdapter.h"
#include "integrations/rest/lcd1602/Lcd1602DeviceApiAdapter.h"
#include "integrations/rest/lcd2004/Lcd2004DeviceApiAdapter.h"
#include "integrations/rest/ntc_thermistor/NtcThermistorTemperatureSensorDeviceApiAdapter.h"
#include "integrations/rest/onewire_bus/OneWireBusDeviceApiAdapter.h"
#include "integrations/rest/rtc_ds1302/Ds1302RtcDeviceApiAdapter.h"
#include "integrations/rest/rtc_ds3231/Ds3231RtcDeviceApiAdapter.h"
#include "integrations/rest/schedule/ScheduleDeviceApiAdapter.h"
#include "integrations/rest/spi_bus/SpiBusDeviceApiAdapter.h"
#include "integrations/rest/ssd1306/Ssd1306DeviceApiAdapter.h"
#include "integrations/rest/st7735/St7735DeviceApiAdapter.h"
#include "integrations/rest/thermostat/ThermostatDeviceApiAdapter.h"
#include "integrations/rest/tm1637/Tm1637DeviceApiAdapter.h"

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

void writeDeviceRecordJson(JsonObject record, const IDeviceRuntime& runtime, const char* typeName, const int64_t configVersion) {
    record["id"] = runtime.deviceId();
    record["typeName"] = typeName;
    if (configVersion >= 0) {
        record["configVersion"] = static_cast<uint32_t>(configVersion);
    }
    record["configRevision"] = runtime.configRevision();
}

void IDeviceApiAdapter::writeDependenciesJson(JsonArray deps, const IDeviceRuntime& runtime) {
    const DeviceDependencyLink* dependencyLinks = runtime.dependencyLinks();
    const uint8_t dependencyCount = runtime.dependencyCount();
    for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
        JsonObject item = deps.createNestedObject();
        item["role"] = deviceRoleName(dependencyLinks[index].role);
        item["deviceId"] = dependencyLinks[index].deviceId;
        if (dependencyLinks[index].role == DeviceRole::Condition && dependencyLinks[index].invert) {
            item["invert"] = true;
        }
    }
}

void IDeviceApiAdapter::writeCommonDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, const char* typeName,
                                              JsonObject output) {
    JsonObject record = output.createNestedObject("record");
    writeDeviceRecordJson(record, runtime, typeName); // REST envelope omits configVersion

    // name/enabled are config-blob fields emitted by the type's writeConfigJson (via the base
    // DeviceBaseConfigV1::writeJson); deps are registry relationships not held in the config blob,
    // so the common envelope owns them here.
    JsonObject config = output.createNestedObject("config");
    writeDependenciesJson(config.createNestedArray("deps"), runtime);

    JsonObject runtimeJson = output.createNestedObject("runtime");
    runtimeJson["status"] = deviceStatusToString(runtime.status());
    runtimeJson["effectiveStatus"] = deviceStatusToString(effectiveStatus);
}

bool IDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                                 const char*& error) const {
    (void)runtime;
    request = {};
    if (input["config"].isNull()) {
        error = "typed config update requires a config object";
        return false;
    }
    error = "typed config update is unsupported";
    return false;
}

bool IDeviceApiAdapter::parseDependenciesJson(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                              uint8_t& depCount, const char*& error, const bool required) {
    deps = {};
    depCount = 0;
    const JsonArrayConst depsArray = input["deps"].as<JsonArrayConst>();
    if (depsArray.isNull()) {
        if (required) {
            error = "deps are required";
            return false;
        }
        error = nullptr;
        return true;
    }

    for (JsonObjectConst item : depsArray) {
        if (depCount >= kMaxDeviceDependencies) {
            error = "deps exceed supported count";
            return false;
        }
        DeviceRole role{DeviceRole::Unknown};
        if (!parseDeviceRole(item["role"] | "", role)) {
            error = "dependency role is invalid";
            return false;
        }
        const DeviceId deviceId = static_cast<DeviceId>(item["deviceId"] | 0U);
        if (deviceId == 0U) {
            error = "dependency deviceId is required";
            return false;
        }
        const bool invert = role == DeviceRole::Condition && (item["invert"] | false);
        deps[depCount++] = DeviceDependencyLink{role, deviceId, invert};
    }
    error = nullptr;
    return true;
}

bool IDeviceApiAdapter::parseSetDepsRequest(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                            uint8_t& depCount, const char*& error) const {
    return parseDependenciesJson(input, deps, depCount, error);
}

bool IDeviceApiAdapter::parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                                         DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const {
    (void)input;
    (void)request;
    persistedRequest = {};
    error = nullptr;
    return true;
}

DeviceValidationResult IDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const {
    (void)request;
    (void)registry;
    return {};
}

DeviceValidationResult IDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                const DeviceCreatePersistenceRequest& persistedRequest,
                                                                const DeviceRegistry& registry) const {
    (void)persistedRequest;
    return validateCreateRequest(request, registry);
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

void IDeviceApiAdapter::writeSetupTransferConfigJson(const IDeviceRuntime& runtime, JsonObject config) const {
    writeDependenciesJson(config.createNestedArray("deps"), runtime);
    writeConfigJson(runtime, config);
}

void IDeviceApiAdapter::writeFallbackDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, const char* typeName,
                                                JsonObject output) {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName, output);
    JsonObject config = output["config"].as<JsonObject>();
    config["name"] = JsonString(runtime.name() != nullptr ? runtime.name() : "", JsonString::Copied);
    config["enabled"] = runtime.enabled();
}

std::unique_ptr<IJsonChunkProducer> IDeviceApiAdapter::createLayoutJsonProducer(const IDeviceRuntime& runtime,
                                                                                const int onlyPageIndex) const {
    (void)runtime;
    (void)onlyPageIndex;
    return nullptr;
}

std::unique_ptr<IJsonChunkProducer> IDeviceApiAdapter::createSetupExportJsonProducer(const IDeviceRuntime& runtime) const {
    (void)runtime;
    return nullptr;
}

std::unique_ptr<IDeviceSetupImportSession> IDeviceApiAdapter::createSetupImportSession(const DeviceId deviceId) const {
    (void)deviceId;
    return nullptr;
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
    (void)registry.registerAdapter(SpiBusDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Ssd1306DeviceApiAdapter::instance());
    (void)registry.registerAdapter(St7735DeviceApiAdapter::instance());
    (void)registry.registerAdapter(Ds18b20TemperatureSensorDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Aht10SensorDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Dht11SensorDeviceApiAdapter::instance());
    (void)registry.registerAdapter(NtcThermistorTemperatureSensorDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Htu21SensorDeviceApiAdapter::instance());
    (void)registry.registerAdapter(ThermostatDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Ds3231RtcDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Ds1302RtcDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Pcf8574ExpanderDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Pcf8575ExpanderDeviceApiAdapter::instance());
    (void)registry.registerAdapter(LedcAnalogOutputDeviceApiAdapter::instance());
    (void)registry.registerAdapter(FadeAnalogOutputDeviceApiAdapter::instance());
    (void)registry.registerAdapter(ScheduledAnalogOutputDeviceApiAdapter::instance());
    (void)registry.registerAdapter(AnalogOutputComposerDeviceApiAdapter::instance());
    (void)registry.registerAdapter(PortExpanderSwitchDeviceApiAdapter::instance());
    (void)registry.registerAdapter(ScheduleDeviceApiAdapter::instance());
    (void)registry.registerAdapter(AutoSwitchDeviceApiAdapter::instance());
    (void)registry.registerAdapter(BinarySensorDeviceApiAdapter::instance());
    (void)registry.registerAdapter(DosingPumpDeviceApiAdapter::instance());
    (void)registry.registerAdapter(AnalogPortInputDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Cd74hc4067HubDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Ads1115HubDeviceApiAdapter::instance());
    (void)registry.registerAdapter(AnalogInputChannelDeviceApiAdapter::instance());
    (void)registry.registerAdapter(Lcd1602DeviceApiAdapter::instance());
    (void)registry.registerAdapter(Lcd2004DeviceApiAdapter::instance());
    (void)registry.registerAdapter(Tm1637DeviceApiAdapter::instance());
    return registry;
}

} // namespace ewfm
