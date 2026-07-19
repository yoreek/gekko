#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/binary/BinarySensorDevice.h"
#include "devices/sensors/binary/BinarySensorDeviceConfig.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "integrations/mqtt/binary/BinarySensorHaEntityAdapter.h"

#include <cstdio>
#include <string>
#include <unity.h>
#include <utility>
#include <vector>

using namespace ewfm;

namespace {

class FakeBinarySensorGpioDriver final : public IGpioInputDriver {
public:
    bool configureInput(uint8_t, GpioInputPullMode) override {
        return true;
    }
    bool read(uint8_t, bool& outLevel) override {
        outLevel = level;
        return true;
    }
    void release(uint8_t) override {}

    bool level{false};
};

BinarySensorDeviceConfigV1 makeBinarySensorConfig() {
    BinarySensorDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "Leak probe");
    config.gpioPin = 4U;
    config.pullMode = static_cast<uint8_t>(GpioInputPullMode::PullUp);
    config.inverted = 0U;
    config.debounceMs = 50U;
    return config;
}

void tickUntilReady(BinarySensorDevice& device, uint32_t now) {
    device.begin(now);
    for (int i = 0; i < 4 && device.status() != DeviceStatus::Ready; ++i) {
        device.tickFastLoop(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
}

HaTopicBuilder topicForDevice(DeviceId deviceId) {
    return [deviceId](const char* channel, const char* suffix) {
        return "node1/" + std::string(channel) + "/" + std::to_string(deviceId) + "/" + suffix;
    };
}

} // namespace

void test_ha_entity_adapter_registry_resolves_binary_sensor() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* adapter = registry.find(kBinarySensorDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("binary_sensor", adapter->typeName());
    TEST_ASSERT_EQUAL_STRING("binary_sensor", adapter->haComponent());
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(registry.forEach(kBinarySensorDeviceTypeId, [](const IHaEntityAdapter&) {})));
}

void test_binary_sensor_ha_entity_adapter_builds_discovery_payload() {
    FakeBinarySensorGpioDriver driver;
    BinarySensorDevice sensor(makeBinarySensorConfig(), driver);
    const BinarySensorHaEntityAdapter& adapter = BinarySensorHaEntityAdapter::instance();
    const std::string uniqueId = "node1_binary_sensor_7";

    DynamicJsonDocument doc(512);
    JsonObject output = doc.to<JsonObject>();
    adapter.buildDiscoveryPayload(sensor, uniqueId, "Leak probe", topicForDevice(7), output);

    TEST_ASSERT_EQUAL_STRING(uniqueId.c_str(), output[ha::key::kUniqueId].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(uniqueId.c_str(), output[ha::key::kObjectId].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Leak probe", output[ha::key::kName].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("node1/binary_sensor/7/state", output[ha::key::kStateTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ON", output[ha::key::kPayloadOn].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("OFF", output[ha::key::kPayloadOff].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("mdi:electric-switch", output[ha::key::kIcon].as<const char*>());
    // The firmware config carries no semantic hint, so the entity must stay class-less.
    TEST_ASSERT_FALSE(output.containsKey(ha::key::kDeviceClass));
    TEST_ASSERT_FALSE(output.containsKey(ha::key::kCommandTopic));
}

void test_binary_sensor_ha_entity_adapter_publishes_active_state_and_skips_before_first_reading() {
    FakeBinarySensorGpioDriver driver;
    BinarySensorDevice sensor(makeBinarySensorConfig(), driver);
    const BinarySensorHaEntityAdapter& adapter = BinarySensorHaEntityAdapter::instance();

    std::vector<std::pair<std::string, std::string>> published;
    const HaStatePublisher publish = [&published](const std::string& topic, const std::string& payload) {
        published.emplace_back(topic, payload);
    };

    // No debounced level has been read yet - nothing may be published.
    adapter.publishState(sensor, topicForDevice(7), publish);
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(published.size()));

    // The first successful read seeds the stable level immediately.
    driver.level = true;
    tickUntilReady(sensor, 100);
    sensor.tickFastLoop(101);
    TEST_ASSERT_TRUE(sensor.hasReading());

    adapter.publishState(sensor, topicForDevice(7), publish);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(published.size()));
    TEST_ASSERT_EQUAL_STRING("node1/binary_sensor/7/state", published[0].first.c_str());
    TEST_ASSERT_EQUAL_STRING("ON", published[0].second.c_str());

    // A debounced flip to low must publish OFF.
    driver.level = false;
    sensor.tickFastLoop(200);
    sensor.tickFastLoop(300); // past debounceMs=50
    published.clear();
    adapter.publishState(sensor, topicForDevice(7), publish);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(published.size()));
    TEST_ASSERT_EQUAL_STRING("OFF", published[0].second.c_str());
}

void test_binary_sensor_ha_entity_adapter_rejects_all_commands() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(900);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    FakeBinarySensorGpioDriver driver;
    BinarySensorDevice sensor(makeBinarySensorConfig(), driver);
    const BinarySensorHaEntityAdapter& adapter = BinarySensorHaEntityAdapter::instance();
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, sensor, 1, "binary_sensor", "ON", 0));
    TEST_ASSERT_FALSE(adapter.applyCommand(registry, sensor, 1, "binary_sensor", "", 0));
}
