#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/switch/TriStateSwitchDeviceBase.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

constexpr DeviceTypeId kFakeSwitchTypeId = 77;

class RegistrySwitch final : public TriStateSwitchDeviceBase {
public:
    explicit RegistrySwitch(const SwitchDeviceConfigV1& config) : TriStateSwitchDeviceBase(config) {}

private:
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = switchDeviceConfigSize(switchConfig());
        return encodeSwitchDeviceConfig(switchConfig(), buffer, size) && configBlob.assign(buffer, size);
    }

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

BoundedBlob<kMaxDeviceConfigBytes> encodeSwitchPayload(const SwitchDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeSwitchDeviceConfig(config, buffer, switchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, switchDeviceConfigSize(config)));
    return payload;
}

std::unique_ptr<IDeviceRuntime> createRegistrySwitchRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    SwitchDeviceConfigV1 config{};
    (void)decodeSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(configBlob.data()), configBlob.size(), config);
    std::unique_ptr<IDeviceRuntime> runtime(new RegistrySwitch(config));
    runtime->bindDeviceIdentity(record, configBlob);
    return runtime;
}

DeviceValidationResult validateRegistrySwitchConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    SwitchDeviceConfigV1 config{};
    if (!decodeSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(configBlob.data()), configBlob.size(), config)) {
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
    config.base.enabled = true;
    std::snprintf(config.base.name, sizeof(config.base.name), "%s", "switch");
    config.restorePreviousState = true;
    config.startupState = OutputState::Off;
    config.safeState = OutputState::Disabled;
    config.inverted = false;

    DeviceCreateRequest request{};
    request.typeId = kFakeSwitchTypeId;
    request.name = "switch";
    request.configBlob = encodeSwitchPayload(config);
    request.configVersion = 1;
    request.enabled = true;
    SwitchDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeSwitchDeviceConfig(request.configBlob.data(), request.configBlob.size(), decoded));
    return request;
}

} // namespace

void test_registry_custom_switch_command_marks_retained_state_without_config_revision_change() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    RetainedStateStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    SequentialDeviceIdSource ids(100);
    DeviceTypeRegistry types = makeRegistry();
    DeviceRegistry registry(store, types, ids, &retainedStore);
    TEST_ASSERT_TRUE(registry.begin(1).ok());

    DeviceCreateResult created = registry.create(makeCreateRequest(), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    registry.tickFastLoop(11);

    const IDeviceRuntime* before = registry.runtime(created.deviceId);
    TEST_ASSERT_NOT_NULL(before);
    const DeviceRevision revisionBefore = before->configRevision();
    const size_t dirtyConfigCountBefore = registry.dirtyConfigRecordIds().size();

    DeviceMutationResult result =
        registry.command(DeviceCommand{DeviceCommandType::SetOutput, created.deviceId, "on", DevicePersistencePolicy::Immediate}, 20);

    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_TRUE(result.pendingPersistence);
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyRetainedStateIds().size());
    TEST_ASSERT_EQUAL_UINT32(created.deviceId, registry.dirtyRetainedStateIds()[0]);
    TEST_ASSERT_EQUAL_UINT32(dirtyConfigCountBefore, registry.dirtyConfigRecordIds().size());

    const IDeviceRuntime* after = registry.runtime(created.deviceId);
    TEST_ASSERT_NOT_NULL(after);
    TEST_ASSERT_EQUAL_UINT32(revisionBefore, after->configRevision());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_custom_switch_command_marks_retained_state_without_config_revision_change);
    return UNITY_END();
}
