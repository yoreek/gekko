#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <unity.h>

using namespace ewfm;

namespace {

DeviceCreateRequest makeGpioSwitchCreateRequest() {
    GpioSwitchDeviceConfigV1 config{};
    config.enabled = true;
    config.restorePreviousState = true;
    config.startupState = static_cast<uint8_t>(OutputState::Off);
    config.safeState = static_cast<uint8_t>(OutputState::Disabled);
    config.inverted = false;
    config.gpioPin = 13;

    DeviceCreateRequest request{};
    request.typeId = GpioSwitchDevice::descriptor().typeId;
    request.name = "relay";
    request.configPayload = encodeGpioSwitchDeviceConfig(config);
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    request.enabled = true;
    request.persistencePolicy = DevicePersistencePolicy::Delayed;
    return request;
}

GpioSwitchDevice* gpioRuntime(DeviceRegistry& registry, DeviceId deviceId) {
    return static_cast<GpioSwitchDevice*>(registry.runtime(deviceId));
}

} // namespace

void test_gpio_switch_registry_create_command_retain_reload_and_delete() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    RetainedStateStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    SequentialDeviceIdSource ids(200);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids, &retainedStore);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult created = registry.create(makeGpioSwitchCreateRequest(), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    registry.tickFastLoop(11);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(registry.effectiveStatus(created.deviceId)));

    DeviceMutationResult command =
        registry.command(DeviceCommand{DeviceCommandType::Custom, created.deviceId, "state=on", DevicePersistencePolicy::Delayed}, 20);
    TEST_ASSERT_TRUE_MESSAGE(command.ok(), command.validation.message);
    TEST_ASSERT_TRUE(command.pendingPersistence);
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyRetainedStateIds().size());
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(gpioRuntime(registry, created.deviceId)->outputState()));

    DeviceValidationResult flushResult = registry.flushNow();
    TEST_ASSERT_TRUE_MESSAGE(flushResult.ok(), flushResult.message);

    SequentialDeviceIdSource reloadIds(300);
    DeviceRegistry reloaded(store, types, reloadIds, &retainedStore);
    TEST_ASSERT_TRUE_MESSAGE(reloaded.begin(30).ok(), "reload failed");
    reloaded.tickFastLoop(31);
    TEST_ASSERT_NOT_NULL(reloaded.runtime(created.deviceId));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(gpioRuntime(reloaded, created.deviceId)->outputState()));

    DeviceMutationResult removed = reloaded.remove(created.deviceId, 40, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(removed.ok(), removed.validation.message);
    TEST_ASSERT_NULL(reloaded.runtime(created.deviceId));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_switch_registry_create_command_retain_reload_and_delete);
    return UNITY_END();
}
