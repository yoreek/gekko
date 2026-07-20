#include "config/MemoryConfigStorage.h"
#include "devices/analog/input/ads1115/Ads1115HubDevice.h"
#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDevice.h"
#include "devices/analog/input/channel/AnalogInputChannelDevice.h"
#include "devices/analog/input/port/AnalogPortInputDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "integrations/mqtt/analog_input/AnalogInputHaEntityAdapter.h"

#include <ArduinoJson.h>
#include <unity.h>
#include <utility>
#include <vector>

using namespace ewfm;

namespace {

HaTopicBuilder topicForDevice(DeviceId deviceId) {
    return [deviceId](const char* channel, const char* suffix) {
        return "node1/" + std::string(channel) + "/" + std::to_string(deviceId) + "/" + suffix;
    };
}

// Minimal fake covering only what the analog input HA adapter needs to exercise - the real
// hardware/state-machine path is already covered by test_analog_port_input.cpp and
// test_analog_input_channel.cpp.
class FakeAnalogInputRuntime final : public IDeviceRuntime, public IAnalogInputRuntime {
public:
    void begin(uint32_t) override {}
    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    void requestReconfigure() override {}
    void requestDisable() override {}
    void requestDelete() override {}
    DeviceStatus status() const override {
        return DeviceStatus::Ready;
    }
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }

    DeviceId deviceId() const override {
        return deviceId_;
    }
    DeviceTypeId typeId() const override {
        return typeId_;
    }
    const char* name() const override {
        return name_;
    }
    const IAnalogInputRuntime* analogInputRuntime() const override {
        return this;
    }

    bool latestAnalogInputReading(AnalogInputReading& reading) const override {
        reading = reading_;
        return true;
    }
    const char* latestAnalogInputStatus() const override {
        return reading_.valid ? "ok" : "not_ready";
    }

    DeviceId deviceId_{0};
    DeviceTypeId typeId_{0};
    const char* name_{"Sump level probe"};
    AnalogInputReading reading_{};
};

AnalogInputHaEntityAdapter makePortAdapter() {
    return AnalogInputHaEntityAdapter({AnalogPortInputDevice::descriptor().typeId, "analog_port_input", "mdi:flash-outline"});
}

} // namespace

void test_ha_entity_adapter_registry_resolves_analog_port_input() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* adapter = registry.find(AnalogPortInputDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("analog_port_input", adapter->typeName());
    TEST_ASSERT_EQUAL_STRING("sensor", adapter->haComponent());
}

void test_ha_entity_adapter_registry_resolves_analog_input_channel() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* adapter = registry.find(AnalogInputChannelDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("analog_input_channel", adapter->typeName());
}

void test_ha_entity_adapter_registry_does_not_resolve_analog_input_hubs() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    // Hubs provide channels, not a reading of their own -- they must not get a HA entity.
    TEST_ASSERT_NULL(registry.find(Ads1115HubDevice::descriptor().typeId));
    TEST_ASSERT_NULL(registry.find(Cd74hc4067HubDevice::descriptor().typeId));
}

void test_analog_input_ha_entity_adapter_builds_discovery_payload() {
    FakeAnalogInputRuntime runtime;
    runtime.deviceId_ = 88;
    runtime.typeId_ = AnalogPortInputDevice::descriptor().typeId;

    const AnalogInputHaEntityAdapter adapter = makePortAdapter();
    const std::string uniqueId = "node1_analog_port_input_88";
    const std::string stateTopic = "node1/sensor/88/state";

    DynamicJsonDocument doc(512);
    JsonObject output = doc.to<JsonObject>();
    adapter.buildDiscoveryPayload(runtime, uniqueId, "Sump level probe", topicForDevice(88), output);

    TEST_ASSERT_EQUAL_STRING(uniqueId.c_str(), output[ha::key::kUniqueId].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Sump level probe", output[ha::key::kName].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(stateTopic.c_str(), output[ha::key::kStateTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("voltage", output[ha::key::kDeviceClass].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("measurement", output[ha::key::kStateClass].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("V", output[ha::key::kUnitOfMeasurement].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("mdi:flash-outline", output[ha::key::kIcon].as<const char*>());
    TEST_ASSERT_FALSE(output.containsKey(ha::key::kCommandTopic));
}

void test_analog_input_ha_entity_adapter_builds_state_payload_for_valid_reading_and_skips_invalid() {
    FakeAnalogInputRuntime runtime;
    const AnalogInputHaEntityAdapter adapter = makePortAdapter();
    std::vector<std::pair<std::string, std::string>> published;
    const HaStatePublisher publish = [&published](const std::string& topic, const std::string& payload) {
        published.emplace_back(topic, payload);
    };

    runtime.reading_ = AnalogInputReading{2048, 1650, 1000, true};
    adapter.publishState(runtime, topicForDevice(runtime.deviceId_), publish);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(published.size()));
    TEST_ASSERT_EQUAL_STRING("1.650", published.back().second.c_str());

    published.clear();
    runtime.reading_ = AnalogInputReading{0, 0, 0, false};
    adapter.publishState(runtime, topicForDevice(runtime.deviceId_), publish);
    TEST_ASSERT_TRUE(published.empty());
}

void test_analog_input_ha_entity_adapter_rejects_all_commands() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(900);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    FakeAnalogInputRuntime runtime;
    const AnalogInputHaEntityAdapter adapter = makePortAdapter();
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, runtime, 1, "sensor", "1.65", 0));
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, runtime, 1, "sensor", "", 0));
}
