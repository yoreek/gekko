#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/thermostat/ThermostatDevice.h"
#include "devices/thermostat/ThermostatDeviceConfig.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"
#include "integrations/mqtt/HaDiscoveryPayload.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "integrations/mqtt/thermostat/ThermostatHaEntityAdapter.h"

#include <cstdio>
#include <string>
#include <unity.h>
#include <utility>
#include <vector>

using namespace ewfm;

namespace {

// Lightweight fakes covering only what the HA adapter needs to exercise (mode/setpoint/action
// translation) - the full sensor+switch control-loop behavior is already covered by
// test_devices/test_thermostat_device.cpp.
class FakeThermostatTemperatureSensor final : public DeviceRuntimeBase, public ITemperatureReadingRuntime {
public:
    FakeThermostatTemperatureSensor() : DeviceRuntimeBase((PState)&FakeThermostatTemperatureSensor::Idle) {
        status_ = DeviceStatus::Ready;
    }

    void setReading(int32_t milliCelsius, uint32_t measuredAtMs) {
        reading_ = TemperatureReading{milliCelsius, measuredAtMs, true};
    }

    void begin(uint32_t) override {
        status_ = DeviceStatus::Ready;
    }
    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }
    bool latestTemperatureReading(TemperatureReading& reading) const override {
        reading = reading_;
        return true;
    }
    const char* latestTemperatureStatus() const override {
        return "ok";
    }
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override {
        return this;
    }

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }

    TemperatureReading reading_{};
};

class FakeThermostatSwitch final : public DeviceRuntimeBase, public ISwitchOutputRuntime {
public:
    FakeThermostatSwitch() : DeviceRuntimeBase((PState)&FakeThermostatSwitch::Idle) {
        status_ = DeviceStatus::Ready;
    }

    void begin(uint32_t) override {
        status_ = DeviceStatus::Ready;
    }
    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }
    StateType currentOutputState() const override {
        return state_;
    }
    bool requestOutputState(StateType state, uint32_t) override {
        state_ = state;
        return true;
    }
    const ISwitchOutputRuntime* switchOutputRuntime() const override {
        return this;
    }

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }

    StateType state_{false};
};

ThermostatDeviceConfigV1 makeThermostatConfig(ThermostatMode mode, int32_t targetMilliCelsius) {
    ThermostatDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "Tank");
    config.mode = static_cast<uint8_t>(mode);
    config.algorithm = static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis);
    config.targetMilliCelsius = targetMilliCelsius;
    config.minSafeMilliCelsius = 0;
    config.maxSafeMilliCelsius = 50000;
    config.hysteresisCentiCelsius = 50;
    config.checkIntervalMs = 100;
    config.sensorTimeoutMs = 200;
    config.retryAfterErrorMs = 300;
    config.minSwitchIntervalMs = 0;
    return config;
}

// setDependencyRuntime() only ever mutates an existing dependencyLinks_ slot (see
// DeviceRuntimeBase::setDependencyRuntime) - that slot is populated exclusively by
// bindDeviceIdentity(), so a standalone ThermostatDevice needs this before wiring fakes in, exactly
// like test_devices/test_thermostat_device.cpp's bindThermostatIdentity() helper does.
void bindThermostatIdentity(ThermostatDevice& device, DeviceId thermostatId, DeviceId temperatureSensorId, DeviceId switchId) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = thermostatDeviceConfigSize(device.config());
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, device.config(), buffer, size));
    DeviceConfigBlob configBlob{};
    TEST_ASSERT_TRUE(configBlob.assign(buffer, size));

    DeviceRegistryEntry record{};
    record.header.deviceId = thermostatId;
    record.header.typeId = kThermostatDeviceTypeId;
    record.header.configVersion = kThermostatDeviceConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.depCount = 2U;
    record.deps[0] = {DeviceRole::TemperatureSensor, temperatureSensorId};
    record.deps[1] = {DeviceRole::Switch, switchId};
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, configBlob);
}

HaTopicBuilder topicForDevice(DeviceId deviceId) {
    return [deviceId](const char* channel, const char* suffix) {
        return "node1/" + std::string(channel) + "/" + std::to_string(deviceId) + "/" + suffix;
    };
}

std::string publishedFor(const std::vector<std::pair<std::string, std::string>>& published, const std::string& topic) {
    for (const auto& entry : published) {
        if (entry.first == topic) {
            return entry.second;
        }
    }
    return "<missing>";
}

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyPlaceholder(const char* name) {
    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(DeviceBaseConfigV1::kMagic, config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

DeviceCreateRequest makeDummyCreateRequest(const char* name) {
    DeviceCreateRequest request{};
    request.typeId = DummyDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeDummyPlaceholder(name);
    return request;
}

DeviceCreateRequest makeThermostatCreateRequest(const char* name, ThermostatMode mode, int32_t targetMilliCelsius,
                                                DeviceId temperatureSensorId, DeviceId switchId) {
    const ThermostatDeviceConfigV1 config = makeThermostatConfig(mode, targetMilliCelsius);
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, config, buffer, thermostatDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, thermostatDeviceConfigSize(config)));

    DeviceCreateRequest request{};
    request.typeId = kThermostatDeviceTypeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.configVersion = kThermostatDeviceConfigVersion;
    request.configBlob = payload;
    request.depCount = 2;
    request.deps[0] = {DeviceRole::TemperatureSensor, temperatureSensorId};
    request.deps[1] = {DeviceRole::Switch, switchId};
    return request;
}

} // namespace

void test_ha_entity_adapter_registry_resolves_thermostat() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* adapter = registry.find(ThermostatDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("thermostat", adapter->typeName());
    TEST_ASSERT_EQUAL_STRING("climate", adapter->haComponent());
}

void test_thermostat_ha_entity_adapter_builds_discovery_payload() {
    ThermostatDevice thermostat(makeThermostatConfig(ThermostatMode::Heat, 25000));
    const ThermostatHaEntityAdapter& adapter = ThermostatHaEntityAdapter::instance();
    const std::string uniqueId = "node1_thermostat_42";

    DynamicJsonDocument doc(1024);
    JsonObject output = doc.to<JsonObject>();
    adapter.buildDiscoveryPayload(thermostat, uniqueId, "Tank", topicForDevice(42), output);

    TEST_ASSERT_EQUAL_STRING(uniqueId.c_str(), output[ha::key::kUniqueId].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Tank", output[ha::key::kName].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("mdi:thermostat", output[ha::key::kIcon].as<const char*>());

    JsonArray modes = output[ha::key::kModes].as<JsonArray>();
    TEST_ASSERT_EQUAL_INT(3, static_cast<int>(modes.size()));
    TEST_ASSERT_EQUAL_STRING("off", modes[0].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("heat", modes[1].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("cool", modes[2].as<const char*>());

    TEST_ASSERT_EQUAL_STRING("node1/climate_mode/42/state", output[ha::key::kModeStateTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("node1/climate_mode/42/set", output[ha::key::kModeCommandTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("node1/climate_temperature/42/state", output[ha::key::kTemperatureStateTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("node1/climate_temperature/42/set", output[ha::key::kTemperatureCommandTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("node1/climate_current_temperature/42/state", output[ha::key::kCurrentTemperatureTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("node1/climate_action/42/state", output[ha::key::kActionTopic].as<const char*>());

    TEST_ASSERT_EQUAL_STRING("C", output[ha::key::kTemperatureUnit].as<const char*>());
    TEST_ASSERT_EQUAL_FLOAT(0.0F, output[ha::key::kMinTemperature].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(50.0F, output[ha::key::kMaxTemperature].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(0.5F, output[ha::key::kTemperatureStep].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(0.1F, output[ha::key::kPrecision].as<float>());

    // Home Assistant's climate discovery schema requires "precision" to serialize as exactly one of
    // 0.1/0.5/1.0 - comparing the parsed float above isn't enough to catch this, since a `0.1F` literal
    // and a `0.1` (double) literal both parse back to the same nearby float. Only the wire string
    // representation (a float literal renders as "0.100000001" due to binary rounding) reveals it.
    std::string serialized;
    serializeJson(doc, serialized);
    TEST_ASSERT_TRUE(serialized.find("\"precision\":0.1") != std::string::npos);
    TEST_ASSERT_TRUE(serialized.find("\"precision\":0.100000") == std::string::npos);

    writeHaDiscoveryEnvelope(output, "node1", "Node One", "~/status");
    TEST_ASSERT_LESS_THAN(kMaxHaDiscoveryPayloadBytes, measureJson(doc) + 1U);
}

void test_thermostat_ha_entity_adapter_publishes_mode_temperature_and_action() {
    ThermostatDevice thermostat(makeThermostatConfig(ThermostatMode::Heat, 25000));
    FakeThermostatTemperatureSensor sensor;
    FakeThermostatSwitch switchDevice;
    bindThermostatIdentity(thermostat, 40U, 41U, 42U);
    thermostat.setDependencyRuntime(DeviceRole::TemperatureSensor, &sensor);
    thermostat.setDependencyRuntime(DeviceRole::Switch, &switchDevice);
    thermostat.begin(100);
    thermostat.tick100ms(101);

    const ThermostatHaEntityAdapter& adapter = ThermostatHaEntityAdapter::instance();

    // Cold reading below the hysteresis band -> heater turns on.
    sensor.setReading(24000, 110);
    thermostat.tick100ms(200);
    thermostat.tick100ms(201);
    thermostat.tick100ms(202);
    TEST_ASSERT_TRUE(thermostat.actualOutputState() == kSwitchOutputOn);

    std::vector<std::pair<std::string, std::string>> published;
    const HaStatePublisher publish = [&published](const std::string& topic, const std::string& payload) {
        published.emplace_back(topic, payload);
    };
    adapter.publishState(thermostat, topicForDevice(42), publish);

    TEST_ASSERT_EQUAL_STRING("heat", publishedFor(published, "node1/climate_mode/42/state").c_str());
    TEST_ASSERT_EQUAL_STRING("25.00", publishedFor(published, "node1/climate_temperature/42/state").c_str());
    TEST_ASSERT_EQUAL_STRING("24.00", publishedFor(published, "node1/climate_current_temperature/42/state").c_str());
    TEST_ASSERT_EQUAL_STRING("heating", publishedFor(published, "node1/climate_action/42/state").c_str());

    // Warm reading above the band -> heater turns off -> action becomes idle (mode still heat).
    sensor.setReading(26000, 300);
    thermostat.tick100ms(400);
    thermostat.tick100ms(401);
    thermostat.tick100ms(402);
    TEST_ASSERT_TRUE(thermostat.actualOutputState() == kSwitchOutputOff);

    published.clear();
    adapter.publishState(thermostat, topicForDevice(42), publish);
    TEST_ASSERT_EQUAL_STRING("idle", publishedFor(published, "node1/climate_action/42/state").c_str());
}

void test_thermostat_ha_entity_adapter_reports_off_action_when_mode_is_off() {
    ThermostatDevice thermostat(makeThermostatConfig(ThermostatMode::Off, 25000));
    const ThermostatHaEntityAdapter& adapter = ThermostatHaEntityAdapter::instance();

    std::vector<std::pair<std::string, std::string>> published;
    const HaStatePublisher publish = [&published](const std::string& topic, const std::string& payload) {
        published.emplace_back(topic, payload);
    };
    adapter.publishState(thermostat, topicForDevice(42), publish);

    TEST_ASSERT_EQUAL_STRING("off", publishedFor(published, "node1/climate_mode/42/state").c_str());
    TEST_ASSERT_EQUAL_STRING("off", publishedFor(published, "node1/climate_action/42/state").c_str());
    // No reading was ever taken on this bare instance - current_temperature must be skipped, not published as garbage.
    TEST_ASSERT_EQUAL_STRING("<missing>", publishedFor(published, "node1/climate_current_temperature/42/state").c_str());
}

void test_thermostat_ha_entity_adapter_applies_mode_and_temperature_commands() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(950);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult sensor = registry.create(makeDummyCreateRequest("sensor-placeholder"), 10);
    TEST_ASSERT_TRUE_MESSAGE(sensor.ok(), sensor.validation.message);
    DeviceCreateResult switchDevice = registry.create(makeDummyCreateRequest("switch-placeholder"), 11);
    TEST_ASSERT_TRUE_MESSAGE(switchDevice.ok(), switchDevice.validation.message);

    DeviceCreateResult thermostat =
        registry.create(makeThermostatCreateRequest("Tank", ThermostatMode::Heat, 25000, sensor.deviceId, switchDevice.deviceId), 12);
    TEST_ASSERT_TRUE_MESSAGE(thermostat.ok(), thermostat.validation.message);

    const ThermostatHaEntityAdapter& adapter = ThermostatHaEntityAdapter::instance();

    IDeviceRuntime* runtime = registry.runtime(thermostat.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_TRUE(adapter.applyCommand(registry, *runtime, thermostat.deviceId, "climate_mode", "cool", 20));
    runtime = registry.runtime(thermostat.deviceId);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ThermostatMode::Cool), static_cast<const ThermostatDevice*>(runtime)->config().mode);

    TEST_ASSERT_TRUE(adapter.applyCommand(registry, *runtime, thermostat.deviceId, "climate_temperature", "24.5", 21));
    runtime = registry.runtime(thermostat.deviceId);
    TEST_ASSERT_EQUAL_INT32(24500, static_cast<const ThermostatDevice*>(runtime)->config().targetMilliCelsius);

    // Out-of-range setpoint must be rejected by config validation (registry.updateConfig), not silently applied.
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, *runtime, thermostat.deviceId, "climate_temperature", "999", 22));
    TEST_ASSERT_EQUAL_INT32(24500,
                            static_cast<const ThermostatDevice*>(registry.runtime(thermostat.deviceId))->config().targetMilliCelsius);

    TEST_ASSERT_FALSE(adapter.applyCommand(registry, *runtime, thermostat.deviceId, "climate_mode", "bogus", 23));
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, *runtime, thermostat.deviceId, "unknown_channel", "heat", 23));
}
