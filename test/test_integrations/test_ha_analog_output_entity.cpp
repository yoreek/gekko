#include "config/MemoryConfigStorage.h"
#include "devices/analog/composer/AnalogOutputComposerDevice.h"
#include "devices/analog/fade/FadeAnalogOutputDevice.h"
#include "devices/analog/ledc/LedcAnalogOutputDevice.h"
#include "devices/analog/ledc/LedcAnalogOutputDeviceConfig.h"
#include "devices/analog/scheduled/ScheduledAnalogOutputDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "integrations/mqtt/analog_output/AnalogOutputHaEntityAdapter.h"

#include <cstdio>
#include <string>
#include <unity.h>
#include <utility>
#include <vector>

using namespace ewfm;

namespace {

BoundedBlob<kMaxDeviceConfigBytes> encodeAnalogOutputPayload(const LedcAnalogOutputDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(LedcAnalogOutputDeviceConfigV1::kMagic, config, buffer, ledcAnalogOutputDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, ledcAnalogOutputDeviceConfigSize(config)));
    return payload;
}

DeviceCreateRequest makeAnalogOutputCreateRequest(const char* name) {
    LedcAnalogOutputDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    config.pin = 4;
    config.ledcChannel = 0;
    config.frequencyHz = 5000;
    config.dutyBits = 12;
    config.inverted = false;
    config.startupState = percentToAnalogOutputState(25);

    DeviceCreateRequest request{};
    request.typeId = LedcAnalogOutputDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.configBlob = encodeAnalogOutputPayload(config);
    request.configVersion = LedcAnalogOutputDevice::descriptor().currentConfigVersion;
    request.setEnabled(true);
    return request;
}

HaTopicBuilder topicForDevice(DeviceId deviceId) {
    return [deviceId](const char* channel, const char* suffix) {
        return "node1/" + std::string(channel) + "/" + std::to_string(deviceId) + "/" + suffix;
    };
}

} // namespace

void test_ha_entity_adapter_registry_resolves_analog_output() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* adapter = registry.find(LedcAnalogOutputDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("analog_output", adapter->typeName());
    TEST_ASSERT_EQUAL_STRING("light", adapter->haComponent());
}

void test_ha_entity_adapter_registry_exposes_composable_analog_outputs() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    TEST_ASSERT_EQUAL_UINT32(1U, registry.forEach(kFadeAnalogOutputDeviceTypeId, [](const IHaEntityAdapter&) {}));
    TEST_ASSERT_EQUAL_UINT32(2U, registry.forEach(kScheduledAnalogOutputDeviceTypeId, [](const IHaEntityAdapter&) {}));
    TEST_ASSERT_EQUAL_UINT32(1U, registry.forEach(kAnalogOutputComposerDeviceTypeId, [](const IHaEntityAdapter&) {}));
}

void test_analog_output_ha_entity_adapter_builds_discovery_payload() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(800);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult created = registry.create(makeAnalogOutputCreateRequest("Lamp"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    registry.tickFastLoop(11);

    const IDeviceRuntime* runtime = registry.runtime(created.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);

    const AnalogOutputHaEntityAdapter adapter({LedcAnalogOutputDevice::descriptor().typeId, "analog_output", "device"});
    DynamicJsonDocument doc(1024);
    JsonObject output = doc.to<JsonObject>();
    adapter.buildDiscoveryPayload(*runtime, "node1_analog_output_1", "Lamp", topicForDevice(created.deviceId), output);

    const std::string stateTopic = "node1/light/" + std::to_string(created.deviceId) + "/state";
    const std::string commandTopic = "node1/light/" + std::to_string(created.deviceId) + "/set";
    const std::string brightnessStateTopic = "node1/light_brightness/" + std::to_string(created.deviceId) + "/state";
    const std::string brightnessCommandTopic = "node1/light_brightness/" + std::to_string(created.deviceId) + "/set";

    TEST_ASSERT_EQUAL_STRING("node1_analog_output_1", output[ha::key::kUniqueId].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Lamp", output[ha::key::kName].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(stateTopic.c_str(), output[ha::key::kStateTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(commandTopic.c_str(), output[ha::key::kCommandTopic].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(brightnessStateTopic.c_str(), output["brightness_state_topic"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(brightnessCommandTopic.c_str(), output["brightness_command_topic"].as<const char*>());
    TEST_ASSERT_EQUAL_INT(255, output["brightness_scale"].as<int>());
    TEST_ASSERT_EQUAL_STRING("brightness", output["on_command_type"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("device", output[ha::key::kIcon].as<const char*>());
}

void test_analog_output_ha_entity_adapter_publishes_state_and_accepts_brightness_commands() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(900);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult created = registry.create(makeAnalogOutputCreateRequest("Lamp"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    registry.tickFastLoop(11);

    const AnalogOutputHaEntityAdapter adapter({LedcAnalogOutputDevice::descriptor().typeId, "analog_output", "device"});
    std::vector<std::pair<std::string, std::string>> published;
    const HaStatePublisher publish = [&published](const std::string& topic, const std::string& payload) {
        published.emplace_back(topic, payload);
    };

    const IDeviceRuntime* runtime = registry.runtime(created.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);
    adapter.publishState(*runtime, topicForDevice(created.deviceId), publish);
    TEST_ASSERT_EQUAL_INT(2, static_cast<int>(published.size()));
    TEST_ASSERT_EQUAL_STRING("ON", published[0].second.c_str());
    TEST_ASSERT_EQUAL_STRING("64", published[1].second.c_str());

    TEST_ASSERT_TRUE(adapter.applyCommand(registry, *runtime, created.deviceId, "light_brightness", "128", 20));
    published.clear();
    adapter.publishState(*registry.runtime(created.deviceId), topicForDevice(created.deviceId), publish);
    TEST_ASSERT_EQUAL_INT(2, static_cast<int>(published.size()));
    TEST_ASSERT_TRUE_MESSAGE(published[0].second == "ON", published[0].second.c_str());
    TEST_ASSERT_EQUAL_STRING("128", published[1].second.c_str());

    TEST_ASSERT_TRUE(adapter.applyCommand(registry, *registry.runtime(created.deviceId), created.deviceId, "light", "OFF", 30));
    published.clear();
    adapter.publishState(*registry.runtime(created.deviceId), topicForDevice(created.deviceId), publish);
    TEST_ASSERT_EQUAL_STRING("OFF", published[0].second.c_str());
    TEST_ASSERT_EQUAL_STRING("0", published[1].second.c_str());
}
