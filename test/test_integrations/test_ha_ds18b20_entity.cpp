#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "integrations/mqtt/ds18b20/Ds18b20HaEntityAdapter.h"

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

// Minimal fake covering only what GpioSwitchHaEntityAdapter's counterpart needs to exercise -
// the real hardware/state-machine path is already covered by test_ds18b20_temperature_sensor.cpp.
class FakeTemperatureRuntime final : public IDeviceRuntime, public ITemperatureReadingRuntime {
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
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override {
        return this;
    }

    bool latestTemperatureReading(TemperatureReading& reading) const override {
        reading = reading_;
        return true;
    }
    const char* latestTemperatureStatus() const override {
        return reading_.valid ? "ok" : "not_ready";
    }

    DeviceId deviceId_{0};
    DeviceTypeId typeId_{0};
    const char* name_{"Aquarium"};
    TemperatureReading reading_{};
};

} // namespace

void test_ha_entity_adapter_registry_resolves_ds18b20() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* adapter = registry.find(Ds18b20TemperatureSensorDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("ds18b20_temperature_sensor", adapter->typeName());
    TEST_ASSERT_EQUAL_STRING("sensor", adapter->haComponent());
}

void test_ds18b20_ha_entity_adapter_builds_discovery_payload() {
    FakeTemperatureRuntime runtime;
    runtime.deviceId_ = 77;
    runtime.typeId_ = Ds18b20TemperatureSensorDevice::descriptor().typeId;

    const Ds18b20HaEntityAdapter& adapter = Ds18b20HaEntityAdapter::instance();
    const std::string uniqueId = "node1_ds18b20_temperature_sensor_77";
    const std::string stateTopic = "node1/sensor/77/state";

    DynamicJsonDocument doc(512);
    JsonObject output = doc.to<JsonObject>();
    adapter.buildDiscoveryPayload(runtime, uniqueId, "Aquarium", topicForDevice(77), output);

    TEST_ASSERT_EQUAL_STRING(uniqueId.c_str(), output["unique_id"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Aquarium", output["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(stateTopic.c_str(), output["state_topic"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("temperature", output["device_class"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("measurement", output["state_class"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("°C", output["unit_of_measurement"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("mdi:thermometer", output["icon"].as<const char*>());
    TEST_ASSERT_FALSE(output.containsKey("command_topic"));
}

void test_ds18b20_ha_entity_adapter_builds_state_payload_for_valid_reading_and_skips_invalid() {
    FakeTemperatureRuntime runtime;
    const Ds18b20HaEntityAdapter& adapter = Ds18b20HaEntityAdapter::instance();
    std::vector<std::pair<std::string, std::string>> published;
    const HaStatePublisher publish = [&published](const std::string& topic, const std::string& payload) {
        published.emplace_back(topic, payload);
    };

    runtime.reading_ = TemperatureReading{23456, 1000, true};
    adapter.publishState(runtime, topicForDevice(runtime.deviceId_), publish);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(published.size()));
    TEST_ASSERT_EQUAL_STRING("23.46", published.back().second.c_str());

    published.clear();
    runtime.reading_ = TemperatureReading{0, 0, false};
    adapter.publishState(runtime, topicForDevice(runtime.deviceId_), publish);
    TEST_ASSERT_TRUE(published.empty());
}

void test_ds18b20_ha_entity_adapter_rejects_all_commands() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(800);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    FakeTemperatureRuntime runtime;
    const Ds18b20HaEntityAdapter& adapter = Ds18b20HaEntityAdapter::instance();
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, runtime, 1, "sensor", "23.5", 0));
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, runtime, 1, "sensor", "", 0));
}
