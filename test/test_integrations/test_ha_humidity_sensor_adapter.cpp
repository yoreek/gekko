#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/htu21/Htu21SensorDevice.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "integrations/mqtt/humidity/HumiditySensorHaEntityAdapter.h"
#include "integrations/mqtt/temperature/TemperatureSensorHaEntityAdapter.h"

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

// Minimal fake covering only what HumiditySensorHaEntityAdapter needs to exercise - the real
// hardware/state-machine path is already covered by test_htu21_sensor.cpp.
class FakeHumidityRuntime final : public IDeviceRuntime, public IHumidityReadingRuntime {
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
    const IHumidityReadingRuntime* humidityReadingRuntime() const override {
        return this;
    }

    bool latestHumidityReading(HumidityReading& reading) const override {
        reading = reading_;
        return true;
    }
    const char* latestHumidityStatus() const override {
        return reading_.valid ? "ok" : "not_ready";
    }

    DeviceId deviceId_{0};
    DeviceTypeId typeId_{0};
    const char* name_{"Greenhouse"};
    HumidityReading reading_{};
};

HumiditySensorHaEntityAdapter makeHtu21HumidityAdapter() {
    return HumiditySensorHaEntityAdapter({Htu21SensorDevice::descriptor().typeId, "htu21_humidity", "mdi:water-percent"});
}

std::vector<const IHaEntityAdapter*> adaptersFor(const HaEntityAdapterRegistry& registry, DeviceTypeId typeId) {
    std::vector<const IHaEntityAdapter*> adapters;
    registry.forEach(typeId, [&adapters](const IHaEntityAdapter& adapter) { adapters.push_back(&adapter); });
    return adapters;
}

} // namespace

void test_ha_entity_adapter_registry_resolves_htu21_humidity() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* adapter = registry.find(Htu21SensorDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("sensor", adapter->haComponent());

    const std::vector<const IHaEntityAdapter*> matches = adaptersFor(registry, Htu21SensorDevice::descriptor().typeId);
    TEST_ASSERT_EQUAL_INT(2, static_cast<int>(matches.size()));
    bool foundTemperature = false;
    bool foundHumidity = false;
    for (const auto* match : matches) {
        if (std::string(match->typeName()) == "htu21") {
            foundTemperature = true;
        } else if (std::string(match->typeName()) == "htu21_humidity") {
            foundHumidity = true;
        }
    }
    TEST_ASSERT_TRUE(foundTemperature);
    TEST_ASSERT_TRUE(foundHumidity);
}

void test_humidity_sensor_ha_entity_adapter_builds_discovery_payload() {
    FakeHumidityRuntime runtime;
    runtime.deviceId_ = 77;
    runtime.typeId_ = Htu21SensorDevice::descriptor().typeId;

    const HumiditySensorHaEntityAdapter adapter = makeHtu21HumidityAdapter();
    const std::string uniqueId = "node1_htu21_humidity_77";
    const std::string stateTopic = "node1/humidity_sensor/77/state";

    DynamicJsonDocument doc(512);
    JsonObject output = doc.to<JsonObject>();
    adapter.buildDiscoveryPayload(runtime, uniqueId, "Greenhouse", topicForDevice(77), output);

    TEST_ASSERT_EQUAL_STRING(uniqueId.c_str(), output[ha::key::kUniqueId].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Greenhouse", output[ha::key::kName].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(stateTopic.c_str(), output[ha::key::kStateTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("humidity", output[ha::key::kDeviceClass].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("measurement", output[ha::key::kStateClass].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("%", output[ha::key::kUnitOfMeasurement].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("mdi:water-percent", output[ha::key::kIcon].as<const char*>());
    TEST_ASSERT_FALSE(output.containsKey(ha::key::kCommandTopic));
}

void test_humidity_sensor_ha_entity_adapter_builds_state_payload_for_valid_reading_and_skips_invalid() {
    FakeHumidityRuntime runtime;
    const HumiditySensorHaEntityAdapter adapter = makeHtu21HumidityAdapter();
    std::vector<std::pair<std::string, std::string>> published;
    const HaStatePublisher publish = [&published](const std::string& topic, const std::string& payload) {
        published.emplace_back(topic, payload);
    };

    runtime.reading_ = HumidityReading{54321, 1000, true};
    adapter.publishState(runtime, topicForDevice(runtime.deviceId_), publish);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(published.size()));
    TEST_ASSERT_EQUAL_STRING("54.32", published.back().second.c_str());

    published.clear();
    runtime.reading_ = HumidityReading{0, 0, false};
    adapter.publishState(runtime, topicForDevice(runtime.deviceId_), publish);
    TEST_ASSERT_TRUE(published.empty());
}

void test_humidity_sensor_ha_entity_adapter_rejects_all_commands() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(800);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    FakeHumidityRuntime runtime;
    const HumiditySensorHaEntityAdapter adapter = makeHtu21HumidityAdapter();
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, runtime, 1, "humidity_sensor", "54.3", 0));
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, runtime, 1, "humidity_sensor", "", 0));
}

void test_ha_entity_adapter_registry_rejects_exact_duplicate_but_allows_second_entity_for_same_type() {
    HaEntityAdapterRegistry registry;
    const TemperatureSensorHaEntityAdapter temperatureAdapter =
        TemperatureSensorHaEntityAdapter({2001, "fake_combo_sensor", "mdi:thermometer"});
    const TemperatureSensorHaEntityAdapter duplicateAdapter =
        TemperatureSensorHaEntityAdapter({2001, "fake_combo_sensor", "mdi:thermometer"});
    const HumiditySensorHaEntityAdapter humidityAdapter =
        HumiditySensorHaEntityAdapter({2001, "fake_combo_sensor_humidity", "mdi:water-percent"});

    TEST_ASSERT_TRUE(registry.registerAdapter(temperatureAdapter));
    TEST_ASSERT_FALSE(registry.registerAdapter(duplicateAdapter));
    TEST_ASSERT_TRUE(registry.registerAdapter(humidityAdapter));

    const std::vector<const IHaEntityAdapter*> matches = adaptersFor(registry, 2001);
    TEST_ASSERT_EQUAL_INT(2, static_cast<int>(matches.size()));
}
