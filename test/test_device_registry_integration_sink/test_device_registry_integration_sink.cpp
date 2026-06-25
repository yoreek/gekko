#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "integrations/common/DeviceEventDispatcher.h"
#include "integrations/common/DeviceIntegrationIdentity.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

struct FixedDeviceIdSource final : public IDeviceIdSource {
    explicit FixedDeviceIdSource(std::initializer_list<DeviceId> ids) : ids_(ids) {}

    bool next(DeviceId& out) override {
        if (index_ >= ids_.size()) {
            return false;
        }
        out = ids_[index_++];
        return true;
    }

    std::vector<DeviceId> ids_{};
    size_t index_{0};
};

struct RecordingSink final : public IDeviceEventSink {
    void onDeviceEvent(const DeviceEvent& event) override {
        events.push_back(event);
    }

    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}

    std::vector<DeviceEvent> events{};
};

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyPayload(const DummyDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

DeviceCreateRequest makeDummyCreateRequest(const std::string& name) {
    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", name.c_str());

    DeviceCreateRequest request{};
    request.typeId = 1;
    request.name = name;
    request.enabled = true;
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeDummyPayload(config);
    return request;
}

int findEventIndex(const std::vector<DeviceEvent>& events, DeviceEventKind kind, int startIndex = 0) {
    for (size_t index = static_cast<size_t>(startIndex); index < events.size(); ++index) {
        if (events[index].kind == kind) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

const DeviceEvent* findLastEvent(const std::vector<DeviceEvent>& events, DeviceEventKind kind) {
    for (auto it = events.rbegin(); it != events.rend(); ++it) {
        if (it->kind == kind) {
            return &(*it);
        }
    }
    return nullptr;
}

} // namespace

void test_integration_sink_observes_order_revisions_and_pending_flags() {
    MemoryConfigStorage storage;
    DeviceRegistryStore registryStore(storage);
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    DeviceEventDispatcher dispatcher;
    RecordingSink sink{};
    TEST_ASSERT_TRUE(dispatcher.registerSink(sink));

    FixedDeviceIdSource idSource({901, 902});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource, &retainedStore, &dispatcher);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult created = registry.command(makeDummyCreateRequest("sink-device"), 10);
    TEST_ASSERT_TRUE(created.ok());
    dispatcher.tickFastLoop(11);

    DeviceMutationResult renamed = registry.command(
        DeviceCommand{DeviceCommandType::Rename, created.deviceId, "sink-device-v2", DevicePersistencePolicy::Delayed}, 20);
    TEST_ASSERT_TRUE(renamed.ok());
    TEST_ASSERT_TRUE(renamed.pendingPersistence);
    dispatcher.tickFastLoop(21);

    TEST_ASSERT_TRUE(registry.flushNow().ok());
    dispatcher.tickFastLoop(23);

    DeviceMutationResult rejected =
        registry.command(DeviceCommand{DeviceCommandType::SetStatus, 9999, "fault", DevicePersistencePolicy::Immediate}, 24);
    TEST_ASSERT_FALSE(rejected.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::MissingRecord), static_cast<int>(rejected.validation.error));
    dispatcher.tickFastLoop(25);

    const int createdIndex = findEventIndex(sink.events, DeviceEventKind::DeviceCreated);
    const int updatedIndex = findEventIndex(sink.events, DeviceEventKind::DeviceUpdated, createdIndex + 1);
    const int configPersistedIndex = findEventIndex(sink.events, DeviceEventKind::ConfigPersisted, updatedIndex + 1);
    const int pendingClearedIndex = findEventIndex(sink.events, DeviceEventKind::PersistencePendingCleared, configPersistedIndex + 1);
    TEST_ASSERT_TRUE(createdIndex >= 0);
    TEST_ASSERT_TRUE(updatedIndex > createdIndex);
    TEST_ASSERT_TRUE(configPersistedIndex > updatedIndex);
    TEST_ASSERT_TRUE(pendingClearedIndex > configPersistedIndex);

    const DeviceEvent* createdEvent = findLastEvent(sink.events, DeviceEventKind::DeviceCreated);
    TEST_ASSERT_NOT_NULL(createdEvent);
    TEST_ASSERT_EQUAL_UINT32(1, createdEvent->registryRevision);
    TEST_ASSERT_EQUAL_UINT32(1, createdEvent->configRevision);

    const DeviceEvent* updatedEvent = findLastEvent(sink.events, DeviceEventKind::DeviceUpdated);
    TEST_ASSERT_NOT_NULL(updatedEvent);
    TEST_ASSERT_EQUAL_UINT32(2, updatedEvent->registryRevision);
    TEST_ASSERT_EQUAL_UINT32(1, updatedEvent->configRevision);
    TEST_ASSERT_TRUE(updatedEvent->pendingPersistence);

    const DeviceEvent* persistedEvent = findLastEvent(sink.events, DeviceEventKind::ConfigPersisted);
    TEST_ASSERT_NOT_NULL(persistedEvent);
    TEST_ASSERT_FALSE(persistedEvent->pendingPersistence);

    const DeviceEvent* rejectedEvent = findLastEvent(sink.events, DeviceEventKind::CommandRejected);
    TEST_ASSERT_NOT_NULL(rejectedEvent);
    TEST_ASSERT_FALSE(rejectedEvent->commandAccepted);
}

void test_integration_sink_handles_bounded_payload_and_unavailable_path() {
    MemoryConfigStorage storage;
    DeviceRegistryStore registryStore(storage);
    TEST_ASSERT_TRUE(registryStore.begin(false));

    DeviceEventDispatcher dispatcher;
    RecordingSink sink{};
    TEST_ASSERT_TRUE(dispatcher.registerSink(sink));

    FixedDeviceIdSource idSource({911, 912});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource, nullptr, &dispatcher);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult created = registry.command(makeDummyCreateRequest("bounded-device"), 10);
    TEST_ASSERT_TRUE(created.ok());
    dispatcher.tickFastLoop(11);

    std::string oversized(kMaxDeviceEventBytes + 1U, 'x');
    DeviceMutationResult oversizedResult =
        registry.command(DeviceCommand{DeviceCommandType::Custom, created.deviceId, oversized, DevicePersistencePolicy::Immediate}, 12);
    TEST_ASSERT_FALSE(oversizedResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::BoundsExceeded), static_cast<int>(oversizedResult.validation.error));
    dispatcher.tickFastLoop(13);

    for (size_t i = 0; i < DeviceEventQueue::kMaxEvents + 2U; ++i) {
        const std::string nextName = std::string("rename-") + std::to_string(i);
        DeviceMutationResult renameResult = registry.command(
            DeviceCommand{DeviceCommandType::Rename, created.deviceId, nextName, DevicePersistencePolicy::Delayed}, 20U + i);
        TEST_ASSERT_TRUE(renameResult.ok());
    }
    TEST_ASSERT_TRUE(dispatcher.droppedEventCount() > 0);

    DeviceMutationResult stillWorks =
        registry.command(DeviceCommand{DeviceCommandType::Enable, created.deviceId, "", DevicePersistencePolicy::Immediate}, 100);
    TEST_ASSERT_TRUE(stillWorks.ok());

    const std::string externalId = makeExternalDeviceId("Controller#A", created.deviceId);
    TEST_ASSERT_EQUAL_STRING("controller_a-dev-0000038f", externalId.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_integration_sink_observes_order_revisions_and_pending_flags);
    RUN_TEST(test_integration_sink_handles_bounded_payload_and_unavailable_path);
    return UNITY_END();
}
