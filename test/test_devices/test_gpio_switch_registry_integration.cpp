#include "config/MemoryConfigStorage.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

BoundedBlob<kMaxDeviceConfigBytes> encodeGpioPayload(const GpioSwitchDeviceConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeLegacyGpioPayload(const GpioSwitchDevicePersistedConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    size_t pos = 0U;
    TEST_ASSERT_TRUE(appendFixedConfigSegment(SwitchDeviceConfigV1::kMagic, config.switchConfig, buffer, sizeof(buffer), pos));
    TEST_ASSERT_TRUE(appendFixedConfigSegment(GpioSwitchDeviceConfigV1::kMagic, config.gpioConfig, buffer, sizeof(buffer), pos));
    TEST_ASSERT_TRUE(payload.assign(buffer, pos));
    return payload;
}

DeviceCreateRequest makeGpioSwitchCreateRequest() {
    GpioSwitchDeviceConfigV3 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "relay");
    config.restorePreviousState = true;
    config.startupState = kSwitchOutputOff;
    config.safeState = kSwitchOutputOff;
    config.inverted = false;
    config.gpioPin = 13;

    DeviceCreateRequest request{};
    request.typeId = GpioSwitchDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName("relay"));
    request.configBlob = encodeGpioPayload(config);
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    request.setEnabled(true);
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
    DeviceRetainedDataStore retainedStore(storage);
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
        registry.command(DeviceCommand{DeviceCommandType::SetOutput, created.deviceId, "true", DevicePersistencePolicy::Delayed}, 20);
    TEST_ASSERT_TRUE_MESSAGE(command.ok(), command.validation.message);
    TEST_ASSERT_TRUE(command.pendingPersistence);
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyRetainedStateIds().size());
    TEST_ASSERT_TRUE(gpioRuntime(registry, created.deviceId)->currentOutputState() == kSwitchOutputOn);

    DeviceValidationResult flushResult = registry.flushNow();
    TEST_ASSERT_TRUE_MESSAGE(flushResult.ok(), flushResult.message);

    SequentialDeviceIdSource reloadIds(300);
    DeviceRegistry reloaded(store, types, reloadIds, &retainedStore);
    TEST_ASSERT_TRUE_MESSAGE(reloaded.begin(30).ok(), "reload failed");
    reloaded.tickFastLoop(31);
    TEST_ASSERT_NOT_NULL(reloaded.runtime(created.deviceId));
    TEST_ASSERT_TRUE(gpioRuntime(reloaded, created.deviceId)->currentOutputState() == kSwitchOutputOn);

    DeviceMutationResult removed = reloaded.remove(created.deviceId, 40, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(removed.ok(), removed.validation.message);
    TEST_ASSERT_NULL(reloaded.runtime(created.deviceId));
}

void test_gpio_switch_registry_migrates_v1_blob_on_begin() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    GpioSwitchDeviceConfigV3 current{};
    current.enabled = true;
    std::snprintf(current.name, sizeof(current.name), "%s", "legacy-relay");
    current.restorePreviousState = true;
    current.startupState = kSwitchOutputOn;
    current.safeState = kSwitchOutputOff;
    current.inverted = true;
    current.gpioPin = 21U;

    GpioSwitchDevicePersistedConfigV1 legacy{};
    legacy.switchConfig.enabled = current.enabled;
    std::snprintf(legacy.switchConfig.name, sizeof(legacy.switchConfig.name), "%s", current.name);
    legacy.switchConfig.restorePreviousState = current.restorePreviousState;
    legacy.switchConfig.startupState = 1U;
    legacy.switchConfig.safeState = 2U;
    legacy.switchConfig.inverted = current.inverted;
    legacy.gpioConfig.gpioPin = current.gpioPin;

    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = 5U;
    record.header.typeId = GpioSwitchDevice::descriptor().typeId;
    record.header.configVersion = 1U;
    record.header.configRevision = 3U;
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    record.status = DeviceStatus::Ready;

    DeviceRegistrySnapshot snapshot{};
    snapshot.records.push_back(record);
    snapshot.indexEntries.push_back({record.header.deviceId, record.header.typeId});
    DeviceConfigBlobMap configBlobs{};
    configBlobs[record.header.deviceId] = encodeLegacyGpioPayload(legacy);
    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());

    SequentialDeviceIdSource ids(10U);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0U).ok());

    const GpioSwitchDevice* runtime = static_cast<const GpioSwitchDevice*>(registry.runtime(record.header.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_UINT32(GpioSwitchDevice::descriptor().currentConfigVersion, runtime->configVersion());
    TEST_ASSERT_EQUAL_UINT8(current.gpioPin, runtime->gpioPin());
    TEST_ASSERT_EQUAL_STRING(current.name, runtime->config().name);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
}
