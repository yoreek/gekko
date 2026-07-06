#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "integrations/mqtt/gpio_switch/GpioSwitchHaEntityAdapter.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

BoundedBlob<kMaxDeviceConfigBytes> encodeGpioPayload(const GpioSwitchDevicePersistedConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeGpioSwitchDeviceConfig(config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return payload;
}

DeviceCreateRequest makeGpioSwitchCreateRequest(const char* name) {
    GpioSwitchDevicePersistedConfigV1 config{};
    config.switchConfig.enabled = true;
    std::snprintf(config.switchConfig.name, sizeof(config.switchConfig.name), "%s", name);
    config.switchConfig.restorePreviousState = false;
    config.switchConfig.startupState = OutputState::Off;
    config.switchConfig.safeState = OutputState::Disabled;
    config.switchConfig.inverted = false;
    config.gpioConfig.gpioPin = 13;

    DeviceCreateRequest request{};
    request.typeId = GpioSwitchDevice::descriptor().typeId;
    request.name = name;
    request.configBlob = encodeGpioPayload(config);
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    request.enabled = true;
    return request;
}

} // namespace

void test_ha_entity_adapter_registry_resolves_gpio_switch_and_rejects_unknown() {
    const HaEntityAdapterRegistry registry = HaEntityAdapterRegistry::withDefaults();
    const IHaEntityAdapter* adapter = registry.find(GpioSwitchDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("gpio_switch", adapter->typeName());
    TEST_ASSERT_EQUAL_STRING("switch", adapter->haComponent());

    TEST_ASSERT_NULL(registry.find(static_cast<DeviceTypeId>(0xBEEF)));
}

void test_gpio_switch_ha_entity_adapter_builds_discovery_payload() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(500);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult created = registry.create(makeGpioSwitchCreateRequest("Pump"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    registry.tickFastLoop(11);

    const IDeviceRuntime* runtime = registry.runtime(created.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);

    const GpioSwitchHaEntityAdapter& adapter = GpioSwitchHaEntityAdapter::instance();
    const std::string uniqueId = "node1_gpio_switch_" + std::to_string(created.deviceId);
    const std::string stateTopic = "node1/switch/" + std::to_string(created.deviceId) + "/state";
    const std::string commandTopic = "node1/switch/" + std::to_string(created.deviceId) + "/set";

    DynamicJsonDocument doc(512);
    JsonObject output = doc.to<JsonObject>();
    adapter.buildDiscoveryPayload(*runtime, uniqueId, "Pump", stateTopic, commandTopic, output);

    TEST_ASSERT_EQUAL_STRING(uniqueId.c_str(), output["unique_id"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Pump", output["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(stateTopic.c_str(), output["state_topic"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(commandTopic.c_str(), output["command_topic"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ON", output["payload_on"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("OFF", output["payload_off"].as<const char*>());
}

void test_gpio_switch_ha_entity_adapter_builds_state_payload_for_on_off_and_skips_disabled() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(600);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult created = registry.create(makeGpioSwitchCreateRequest("Light"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    registry.tickFastLoop(11);

    const GpioSwitchHaEntityAdapter& adapter = GpioSwitchHaEntityAdapter::instance();
    std::string payload;

    IDeviceRuntime* runtime = registry.runtime(created.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_TRUE(adapter.buildStatePayload(*runtime, payload));
    TEST_ASSERT_EQUAL_STRING("OFF", payload.c_str());

    DeviceMutationResult on = registry.command(DeviceCommand{DeviceCommandType::SetOutput, created.deviceId, "on"}, 20);
    TEST_ASSERT_TRUE_MESSAGE(on.ok(), on.validation.message);
    TEST_ASSERT_TRUE(adapter.buildStatePayload(*registry.runtime(created.deviceId), payload));
    TEST_ASSERT_EQUAL_STRING("ON", payload.c_str());

    DeviceMutationResult disabled = registry.command(DeviceCommand{DeviceCommandType::SetOutput, created.deviceId, "disabled"}, 30);
    TEST_ASSERT_TRUE_MESSAGE(disabled.ok(), disabled.validation.message);
    TEST_ASSERT_FALSE(adapter.buildStatePayload(*registry.runtime(created.deviceId), payload));
}

void test_gpio_switch_ha_entity_adapter_parses_on_off_commands_case_insensitively() {
    const GpioSwitchHaEntityAdapter& adapter = GpioSwitchHaEntityAdapter::instance();
    DeviceCommand command;

    TEST_ASSERT_TRUE(adapter.parseCommand("ON", 7, command));
    TEST_ASSERT_TRUE(command.type == DeviceCommandType::SetOutput);
    TEST_ASSERT_EQUAL_UINT32(7, command.deviceId);
    TEST_ASSERT_TRUE(command.payload.equals("on"));

    TEST_ASSERT_TRUE(adapter.parseCommand("off", 8, command));
    TEST_ASSERT_TRUE(command.payload.equals("off"));

    TEST_ASSERT_FALSE(adapter.parseCommand("toggle", 9, command));
}
