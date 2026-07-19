#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dosing/DosingPumpDevice.h"
#include "devices/dosing/DosingPumpDeviceConfig.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "integrations/mqtt/dosing/DosingPumpHaEntityAdapters.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <unity.h>
#include <utility>
#include <vector>

using namespace ewfm;

namespace {

class FakeDosingPumpSwitch final : public DeviceRuntimeBase, public ISwitchOutputRuntime {
public:
    FakeDosingPumpSwitch() : DeviceRuntimeBase((PState)&FakeDosingPumpSwitch::Idle) {
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
    const ISwitchOutputRuntime* switchOutputRuntime() const override {
        return this;
    }
    StateType currentOutputState() const override {
        return state_;
    }
    bool requestOutputState(StateType state, uint32_t) override {
        state_ = state;
        return true;
    }

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }

    StateType state_{false};
};

DosingPumpDeviceConfigV1 makeDosingPumpConfig() {
    DosingPumpDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "Calcium");
    config.speedMilliMlPerSec = 1000U; // 1 ml/s
    config.containerCapacityMl = 1000U;
    config.thresholdPercent = 10U;
    config.blockAutoWhenEmpty = 1U;
    config.scheduleMode = static_cast<uint8_t>(DosingScheduleMode::Daily);
    config.everyDays = 1U;
    config.daysOfWeekMask = 0x7F;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeDosingPumpPayload(const DosingPumpDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, config, buffer, dosingPumpDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dosingPumpDeviceConfigSize(config)));
    return payload;
}

// A standalone DosingPumpDevice's dependency slots are populated only by bindDeviceIdentity()
// (same story as test_devices/test_dosing_pump_device.cpp's helper).
void bindDosingPumpIdentity(DosingPumpDevice& device, DeviceId pumpId, DeviceId switchId) {
    const BoundedBlob<kMaxDeviceConfigBytes> configBlob = encodeDosingPumpPayload(device.config());
    DeviceRegistryEntry record{};
    record.header.deviceId = pumpId;
    record.header.typeId = kDosingPumpDeviceTypeId;
    record.header.configVersion = kDosingPumpDeviceConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.deps[0] = DeviceDependencyLink{DeviceRole::Switch, switchId, false};
    record.depCount = 1U;
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

const IHaEntityAdapter* adapterByTypeName(const std::vector<const IHaEntityAdapter*>& adapters, const char* typeName) {
    for (const auto* adapter : adapters) {
        if (adapter != nullptr && std::strcmp(adapter->typeName(), typeName) == 0) {
            return adapter;
        }
    }
    return nullptr;
}

std::vector<const IHaEntityAdapter*> adaptersFor(const HaEntityAdapterRegistry& registry, DeviceTypeId typeId) {
    std::vector<const IHaEntityAdapter*> adapters;
    registry.forEach(typeId, [&adapters](const IHaEntityAdapter& adapter) { adapters.push_back(&adapter); });
    return adapters;
}

// Publishes every dosing entity's state into one topic->payload list, so assertions can pick
// individual channels out by topic.
std::vector<std::pair<std::string, std::string>> publishAll(const std::vector<const IHaEntityAdapter*>& adapters,
                                                            const DosingPumpDevice& pump, DeviceId deviceId) {
    std::vector<std::pair<std::string, std::string>> published;
    const HaStatePublisher publish = [&published](const std::string& topic, const std::string& payload) {
        published.emplace_back(topic, payload);
    };
    for (const auto* adapter : adapters) {
        adapter->publishState(pump, topicForDevice(deviceId), publish);
    }
    return published;
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

DeviceCreateRequest makeDosingPumpCreateRequest(const char* name, DeviceId switchId) {
    DeviceCreateRequest request{};
    request.typeId = kDosingPumpDeviceTypeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.configVersion = kDosingPumpDeviceConfigVersion;
    request.configBlob = encodeDosingPumpPayload(makeDosingPumpConfig());
    request.depCount = 1;
    request.deps[0] = {DeviceRole::Switch, switchId};
    return request;
}

} // namespace

void test_ha_entity_adapter_registry_resolves_five_dosing_pump_entities() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const std::vector<const IHaEntityAdapter*> adapters = adaptersFor(registry, kDosingPumpDeviceTypeId);
    TEST_ASSERT_EQUAL_INT(5, static_cast<int>(adapters.size()));

    TEST_ASSERT_NOT_NULL(adapterByTypeName(adapters, "dosing_pump_state"));
    TEST_ASSERT_NOT_NULL(adapterByTypeName(adapters, "dosing_pump_today_dosed"));
    TEST_ASSERT_NOT_NULL(adapterByTypeName(adapters, "dosing_pump_container_level"));
    TEST_ASSERT_NOT_NULL(adapterByTypeName(adapters, "dosing_pump_container_empty"));
    TEST_ASSERT_NOT_NULL(adapterByTypeName(adapters, "dosing_pump_auto_mode"));

    TEST_ASSERT_EQUAL_STRING("sensor", adapterByTypeName(adapters, "dosing_pump_state")->haComponent());
    TEST_ASSERT_EQUAL_STRING("sensor", adapterByTypeName(adapters, "dosing_pump_today_dosed")->haComponent());
    TEST_ASSERT_EQUAL_STRING("sensor", adapterByTypeName(adapters, "dosing_pump_container_level")->haComponent());
    TEST_ASSERT_EQUAL_STRING("binary_sensor", adapterByTypeName(adapters, "dosing_pump_container_empty")->haComponent());
    TEST_ASSERT_EQUAL_STRING("switch", adapterByTypeName(adapters, "dosing_pump_auto_mode")->haComponent());
}

void test_dosing_pump_ha_entity_adapters_build_discovery_payloads() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const std::vector<const IHaEntityAdapter*> adapters = adaptersFor(registry, kDosingPumpDeviceTypeId);
    DosingPumpDevice pump(makeDosingPumpConfig());

    // Run state: enum sensor - options array present, unit/state_class must be absent (HA rejects
    // enum sensors carrying them).
    {
        DynamicJsonDocument doc(768);
        JsonObject output = doc.to<JsonObject>();
        adapterByTypeName(adapters, "dosing_pump_state")
            ->buildDiscoveryPayload(pump, "node1_dosing_pump_state_9", "Calcium", topicForDevice(9), output);
        TEST_ASSERT_EQUAL_STRING("Calcium State", output[ha::key::kName].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("node1/dosing_state/9/state", output[ha::key::kStateTopic].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("enum", output[ha::key::kDeviceClass].as<const char*>());
        JsonArray options = output[ha::key::kOptions].as<JsonArray>();
        TEST_ASSERT_EQUAL_INT(2, static_cast<int>(options.size()));
        TEST_ASSERT_EQUAL_STRING("idle", options[0].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("dosing", options[1].as<const char*>());
        TEST_ASSERT_FALSE(output.containsKey(ha::key::kUnitOfMeasurement));
        TEST_ASSERT_FALSE(output.containsKey(ha::key::kStateClass));
        TEST_ASSERT_FALSE(output.containsKey(ha::key::kCommandTopic));
    }

    // Today dosed: volume in mL, total_increasing (the midnight reset reads as a meter reset).
    {
        DynamicJsonDocument doc(768);
        JsonObject output = doc.to<JsonObject>();
        adapterByTypeName(adapters, "dosing_pump_today_dosed")
            ->buildDiscoveryPayload(pump, "node1_dosing_pump_today_dosed_9", "Calcium", topicForDevice(9), output);
        TEST_ASSERT_EQUAL_STRING("Calcium Today dosed", output[ha::key::kName].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("node1/dosing_today_dosed/9/state", output[ha::key::kStateTopic].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("volume", output[ha::key::kDeviceClass].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("mL", output[ha::key::kUnitOfMeasurement].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("total_increasing", output[ha::key::kStateClass].as<const char*>());
        TEST_ASSERT_FALSE(output.containsKey(ha::key::kCommandTopic));
    }

    // Container level: plain volume measurement.
    {
        DynamicJsonDocument doc(768);
        JsonObject output = doc.to<JsonObject>();
        adapterByTypeName(adapters, "dosing_pump_container_level")
            ->buildDiscoveryPayload(pump, "node1_dosing_pump_container_level_9", "Calcium", topicForDevice(9), output);
        TEST_ASSERT_EQUAL_STRING("Calcium Container level", output[ha::key::kName].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("node1/dosing_container_level/9/state", output[ha::key::kStateTopic].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("volume", output[ha::key::kDeviceClass].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("mL", output[ha::key::kUnitOfMeasurement].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("measurement", output[ha::key::kStateClass].as<const char*>());
        TEST_ASSERT_FALSE(output.containsKey(ha::key::kCommandTopic));
    }

    // Container empty: problem-class binary sensor (ON = empty = problem).
    {
        DynamicJsonDocument doc(768);
        JsonObject output = doc.to<JsonObject>();
        adapterByTypeName(adapters, "dosing_pump_container_empty")
            ->buildDiscoveryPayload(pump, "node1_dosing_pump_container_empty_9", "Calcium", topicForDevice(9), output);
        TEST_ASSERT_EQUAL_STRING("Calcium Container empty", output[ha::key::kName].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("node1/dosing_container_empty/9/state", output[ha::key::kStateTopic].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("problem", output[ha::key::kDeviceClass].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("ON", output[ha::key::kPayloadOn].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("OFF", output[ha::key::kPayloadOff].as<const char*>());
        TEST_ASSERT_FALSE(output.containsKey(ha::key::kCommandTopic));
    }

    // Auto mode: the only commandable entity, grouped under HA's Configuration category.
    {
        DynamicJsonDocument doc(768);
        JsonObject output = doc.to<JsonObject>();
        adapterByTypeName(adapters, "dosing_pump_auto_mode")
            ->buildDiscoveryPayload(pump, "node1_dosing_pump_auto_mode_9", "Calcium", topicForDevice(9), output);
        TEST_ASSERT_EQUAL_STRING("Calcium Auto mode", output[ha::key::kName].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("node1/dosing_auto_mode/9/state", output[ha::key::kStateTopic].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("node1/dosing_auto_mode/9/set", output[ha::key::kCommandTopic].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("ON", output[ha::key::kPayloadOn].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("OFF", output[ha::key::kPayloadOff].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("ON", output[ha::key::kStateOn].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("OFF", output[ha::key::kStateOff].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("config", output[ha::key::kEntityCategory].as<const char*>());
    }
}

void test_dosing_pump_ha_entity_adapters_publish_monitoring_states() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const std::vector<const IHaEntityAdapter*> adapters = adaptersFor(registry, kDosingPumpDeviceTypeId);

    DosingPumpDevice pump(makeDosingPumpConfig());
    FakeDosingPumpSwitch pumpSwitch;
    bindDosingPumpIdentity(pump, 9U, 10U);
    pump.setDependencyRuntime(DeviceRole::Switch, &pumpSwitch);
    pump.begin(10);
    pump.tick100ms(11);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(pump.status()));

    // Fresh pump: idle, nothing dosed, container seeded to full capacity, manual mode.
    auto published = publishAll(adapters, pump, 9U);
    TEST_ASSERT_EQUAL_STRING("idle", publishedFor(published, "node1/dosing_state/9/state").c_str());
    TEST_ASSERT_EQUAL_STRING("0.00", publishedFor(published, "node1/dosing_today_dosed/9/state").c_str());
    TEST_ASSERT_EQUAL_STRING("1000.00", publishedFor(published, "node1/dosing_container_level/9/state").c_str());
    TEST_ASSERT_EQUAL_STRING("OFF", publishedFor(published, "node1/dosing_container_empty/9/state").c_str());
    TEST_ASSERT_EQUAL_STRING("OFF", publishedFor(published, "node1/dosing_auto_mode/9/state").c_str());

    // A drained container flips the problem entity ON.
    TEST_ASSERT_TRUE(pump.handleCommand(DeviceCommand{DeviceCommandType::Custom, 9U, "setVolume:0"}));
    published = publishAll(adapters, pump, 9U);
    TEST_ASSERT_EQUAL_STRING("0.00", publishedFor(published, "node1/dosing_container_level/9/state").c_str());
    TEST_ASSERT_EQUAL_STRING("ON", publishedFor(published, "node1/dosing_container_empty/9/state").c_str());

    // Refill (250.50 ml) and enable auto mode through the same Custom-command grammar HA uses.
    TEST_ASSERT_TRUE(pump.handleCommand(DeviceCommand{DeviceCommandType::Custom, 9U, "setVolume:25050"}));
    TEST_ASSERT_TRUE(pump.handleCommand(DeviceCommand{DeviceCommandType::Custom, 9U, "auto"}));
    published = publishAll(adapters, pump, 9U);
    TEST_ASSERT_EQUAL_STRING("250.50", publishedFor(published, "node1/dosing_container_level/9/state").c_str());
    TEST_ASSERT_EQUAL_STRING("OFF", publishedFor(published, "node1/dosing_container_empty/9/state").c_str());
    TEST_ASSERT_EQUAL_STRING("ON", publishedFor(published, "node1/dosing_auto_mode/9/state").c_str());

    // A manual run flips the state sensor to "dosing".
    TEST_ASSERT_TRUE(pump.startRun(DosingRunType::Manual, 500U, 1000U));
    published = publishAll(adapters, pump, 9U);
    TEST_ASSERT_EQUAL_STRING("dosing", publishedFor(published, "node1/dosing_state/9/state").c_str());
}

void test_dosing_pump_ha_entity_adapter_applies_auto_mode_commands() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(970);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult switchDevice = registry.create(makeDummyCreateRequest("switch-placeholder"), 10);
    TEST_ASSERT_TRUE_MESSAGE(switchDevice.ok(), switchDevice.validation.message);
    DeviceCreateResult pump = registry.create(makeDosingPumpCreateRequest("Calcium", switchDevice.deviceId), 11);
    TEST_ASSERT_TRUE_MESSAGE(pump.ok(), pump.validation.message);

    const HaEntityAdapterRegistry adapters = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* autoMode = adapterByTypeName(adaptersFor(adapters, kDosingPumpDeviceTypeId), "dosing_pump_auto_mode");
    TEST_ASSERT_NOT_NULL(autoMode);

    IDeviceRuntime* runtime = registry.runtime(pump.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_FALSE(static_cast<const DosingPumpDevice*>(runtime)->autoMode());

    TEST_ASSERT_TRUE(autoMode->applyCommand(registry, *runtime, pump.deviceId, "dosing_auto_mode", "ON", 20));
    TEST_ASSERT_TRUE(static_cast<const DosingPumpDevice*>(registry.runtime(pump.deviceId))->autoMode());

    // Case-insensitive, like the GPIO switch adapter.
    TEST_ASSERT_TRUE(autoMode->applyCommand(registry, *runtime, pump.deviceId, "dosing_auto_mode", "off", 21));
    TEST_ASSERT_FALSE(static_cast<const DosingPumpDevice*>(registry.runtime(pump.deviceId))->autoMode());

    TEST_ASSERT_FALSE(autoMode->applyCommand(registry, *runtime, pump.deviceId, "dosing_auto_mode", "bogus", 22));
    TEST_ASSERT_FALSE(autoMode->applyCommand(registry, *runtime, pump.deviceId, "dosing_state", "ON", 23));

    // The monitoring entities never accept commands, whatever the channel.
    const IHaEntityAdapter* stateSensor = adapterByTypeName(adaptersFor(adapters, kDosingPumpDeviceTypeId), "dosing_pump_state");
    TEST_ASSERT_NOT_NULL(stateSensor);
    TEST_ASSERT_FALSE(stateSensor->applyCommand(registry, *runtime, pump.deviceId, "dosing_state", "ON", 24));
}
