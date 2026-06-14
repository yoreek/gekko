#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/switch/TriStateSwitchDeviceBase.h"

#include <unity.h>

using namespace ewfm;

namespace {

constexpr DeviceTypeId kFakeSwitchTypeId = 77;

class RegistrySwitch final : public TriStateSwitchDeviceBase {
public:
    explicit RegistrySwitch(const SwitchDeviceConfigV1& config) : TriStateSwitchDeviceBase(config) {}

private:
    DeviceValidationResult configureHardware(uint32_t now) override {
        (void)now;
        return {};
    }

    DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) override {
        (void)state;
        (void)physicalLevel;
        (void)now;
        return {};
    }

    void releaseHardware(uint32_t now) override {
        (void)now;
    }
};

std::unique_ptr<IDeviceRuntime> createRegistrySwitchRuntime(const DeviceRecord& record) {
    SwitchDeviceConfigV1 config{};
    (void)decodeSwitchDeviceConfig(record.configPayload, config);
    return std::unique_ptr<IDeviceRuntime>(new RegistrySwitch(config));
}

DeviceValidationResult validateRegistrySwitchConfig(const DeviceRecord& record) {
    SwitchDeviceConfigV1 config{};
    if (!decodeSwitchDeviceConfig(record.configPayload, config)) {
        return {DeviceError::InvalidConfig, "switch config is invalid"};
    }
    return {};
}

DeviceTypeRegistry makeRegistry() {
    DeviceTypeRegistry registry;
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kFakeSwitchTypeId;
    descriptor.name = "RegistrySwitch";
    descriptor.currentConfigVersion = 1;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.createRuntime = &createRegistrySwitchRuntime;
    descriptor.validateConfig = &validateRegistrySwitchConfig;
    TEST_ASSERT_TRUE(registry.registerDescriptor(descriptor));
    return registry;
}

DeviceCreateRequest makeCreateRequest() {
    SwitchDeviceConfigV1 config{};
    config.enabled = true;
    config.restorePreviousState = true;
    config.startupState = static_cast<uint8_t>(OutputState::Off);
    config.safeState = static_cast<uint8_t>(OutputState::Disabled);
    config.inverted = false;

    DeviceCreateRequest request{};
    request.typeId = kFakeSwitchTypeId;
    request.name = "switch";
    request.configPayload = encodeSwitchDeviceConfig(config);
    request.configVersion = 1;
    request.enabled = true;
    request.persistencePolicy = DevicePersistencePolicy::Delayed;
    return request;
}

} // namespace

void test_registry_custom_switch_command_marks_retained_state_without_config_revision_change() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(true));
    RetainedStateStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(true));
    SequentialDeviceIdSource ids(100);
    DeviceTypeRegistry types = makeRegistry();
    DeviceRegistry registry(store, types, ids, &retainedStore);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult created = registry.create(makeCreateRequest(), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    registry.tickFastLoop(11);

    const DeviceRecord* before = registry.find(created.deviceId);
    TEST_ASSERT_NOT_NULL(before);
    const DeviceRevision revisionBefore = before->header.configRevision;
    const size_t dirtyConfigCountBefore = registry.dirtyConfigRecordIds().size();

    DeviceMutationResult result =
        registry.command(DeviceCommand{DeviceCommandType::Custom, created.deviceId, "state=on", DevicePersistencePolicy::Immediate}, 20);

    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_TRUE(result.pendingPersistence);
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyRetainedStateIds().size());
    TEST_ASSERT_EQUAL_UINT32(created.deviceId, registry.dirtyRetainedStateIds()[0]);
    TEST_ASSERT_EQUAL_UINT32(dirtyConfigCountBefore, registry.dirtyConfigRecordIds().size());

    const DeviceRecord* after = registry.find(created.deviceId);
    TEST_ASSERT_NOT_NULL(after);
    TEST_ASSERT_EQUAL_UINT32(revisionBefore, after->header.configRevision);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_custom_switch_command_marks_retained_state_without_config_revision_change);
    return UNITY_END();
}
