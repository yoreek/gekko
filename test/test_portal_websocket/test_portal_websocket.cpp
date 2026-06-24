#include "config/MemoryConfigStorage.h"
#include "devices/dummy/DummyDevice.h"
#include "integrations/common/DeviceEventDispatcher.h"
#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"
#include "portal/ws/PortalWebSocketManager.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

struct FixedDeviceIdSource final : public IDeviceIdSource {
    FixedDeviceIdSource(DeviceId firstId, DeviceId secondId) : ids_{firstId, secondId} {}
    explicit FixedDeviceIdSource(DeviceId id) : ids_{id, 0} {}

    bool next(DeviceId& out) override {
        if (index_ >= 2 || ids_[index_] == 0) {
            return false;
        }
        out = ids_[index_++];
        return true;
    }

    DeviceId ids_[2]{};
    size_t index_{0};
};

DeviceEvent makeDeviceEvent(const DeviceEventKind kind, const uint32_t revision, const DeviceId deviceId = 42) {
    DeviceEvent event{};
    event.kind = kind;
    (void)event.eventKind.assign(deviceEventKindName(kind));
    event.registryRevision = revision;
    event.configRevision = 7;
    event.deviceId = deviceId;
    event.typeId = 99;
    (void)event.name.assign("Living Room Lamp");
    (void)event.typeName.assign("Dummy device");
    event.previousStatus = DeviceStatus::Starting;
    event.status = DeviceStatus::Ready;
    event.pendingPersistence = true;
    event.commandAccepted = kind == DeviceEventKind::CommandAccepted;
    (void)event.detail.assign("detail");
    return event;
}

DeviceRegistryEntry makeDeviceRecord() {
    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = 42;
    record.header.typeId = DummyDeviceApiAdapter::instance().typeId();
    record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 5;
    DummyDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", "Living Room Lamp");
    record.header.payloadLength = static_cast<uint32_t>(dummyDeviceConfigSize(config));
    record.depCount = 0;
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    record.status = DeviceStatus::Ready;
    return record;
}

DeviceConfigBlob makeDeviceConfigBlob(const char* name = "Living Room Lamp") {
    DummyDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(blob.assign(buffer, dummyDeviceConfigSize(config)));
    return blob;
}

DeviceCreateRequest makeCreateRequest(const char* name = "Living Room Lamp") {
    DummyDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", name);

    DeviceCreateRequest request{};
    request.typeId = DummyDevice::descriptor().typeId;
    request.name = config.name;
    request.enabled = true;
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.configBlob = makeDeviceConfigBlob(name);
    return request;
}

} // namespace

void test_ws_message_builders_create_compact_envelopes() {
    DeviceEvent upsertEvent = makeDeviceEvent(DeviceEventKind::DeviceUpdated, 12);
    const std::string upsert = PortalWebSocketMessages::buildDeviceUpsert(upsertEvent);
    DynamicJsonDocument upsertDoc(1536);
    TEST_ASSERT_FALSE(deserializeJson(upsertDoc, upsert));
    TEST_ASSERT_EQUAL_STRING("device.upsert", upsertDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(12, upsertDoc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(42, upsertDoc["payload"]["deviceId"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("device_updated", upsertDoc["payload"]["eventKind"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(99, upsertDoc["payload"]["typeId"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(7, upsertDoc["payload"]["configRevision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint8_t>(DeviceStatus::Starting), upsertDoc["payload"]["previousStatus"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint8_t>(DeviceStatus::Ready), upsertDoc["payload"]["status"].as<uint8_t>());
    TEST_ASSERT_EQUAL_STRING("Living Room Lamp", upsertDoc["payload"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Dummy device", upsertDoc["payload"]["type"].as<const char*>());
    TEST_ASSERT_TRUE(upsertDoc["payload"]["pendingPersistence"].as<bool>());
    TEST_ASSERT_FALSE(upsertDoc["payload"]["commandAccepted"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("detail", upsertDoc["payload"]["detail"].as<const char*>());

    const DeviceRegistryEntry record = makeDeviceRecord();
    const DeviceConfigBlob configBlob = makeDeviceConfigBlob();
    DummyDevice runtime(record, configBlob);
    runtime.begin(0);
    runtime.tickFastLoop(1);
    const std::string snapshot =
        PortalWebSocketMessages::buildDeviceUpsert(runtime, runtime.status(), 14, true, &DummyDeviceApiAdapter::instance(), "snapshot");
    DynamicJsonDocument snapshotDoc(1536);
    TEST_ASSERT_FALSE(deserializeJson(snapshotDoc, snapshot));
    TEST_ASSERT_EQUAL_STRING("device.upsert", snapshotDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(14, snapshotDoc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(42, snapshotDoc["payload"]["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("snapshot", snapshotDoc["payload"]["eventKind"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Living Room Lamp", snapshotDoc["payload"]["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(snapshotDoc["payload"]["config"]["deps"].is<JsonArrayConst>());
    TEST_ASSERT_EQUAL_STRING("ready", snapshotDoc["payload"]["runtime"]["effectiveStatus"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ready", snapshotDoc["payload"]["runtime"]["status"].as<const char*>());

    DeviceEvent removedEvent = makeDeviceEvent(DeviceEventKind::DeviceDeleted, 13);
    const std::string removed = PortalWebSocketMessages::buildDeviceRemove(removedEvent);
    DynamicJsonDocument removedDoc(1024);
    TEST_ASSERT_FALSE(deserializeJson(removedDoc, removed));
    TEST_ASSERT_EQUAL_STRING("device.remove", removedDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(13, removedDoc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(42, removedDoc["payload"]["deviceId"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("device_deleted", removedDoc["payload"]["eventKind"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(99, removedDoc["payload"]["typeId"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("Living Room Lamp", removedDoc["payload"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Dummy device", removedDoc["payload"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("detail", removedDoc["payload"]["detail"].as<const char*>());

    const std::string hello = PortalWebSocketMessages::buildHello(9, 8, 2);
    DynamicJsonDocument helloDoc(1024);
    TEST_ASSERT_FALSE(deserializeJson(helloDoc, hello));
    TEST_ASSERT_EQUAL_STRING("hello", helloDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(9, helloDoc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("connected", helloDoc["payload"]["state"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(2, helloDoc["payload"]["clients"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(8, helloDoc["payload"]["registryRevision"].as<uint32_t>());
}

void test_ws_manager_attaches_and_detaches_from_dispatcher() {
    DeviceEventDispatcher dispatcher;
    PortalWebSocketManager manager(&dispatcher);

    TEST_ASSERT_FALSE(dispatcher.hasSink(manager));
    manager.attachDispatcher();
    TEST_ASSERT_TRUE(dispatcher.hasSink(manager));
    manager.detachDispatcher();
    TEST_ASSERT_FALSE(dispatcher.hasSink(manager));
}

void test_ws_manager_receives_device_events_when_attached() {
    DeviceEventDispatcher dispatcher;
    PortalWebSocketManager manager(&dispatcher);
    manager.attachDispatcher();
    manager.setClientCountForTest(1);

    const DeviceEvent event = makeDeviceEvent(DeviceEventKind::DeviceUpdated, 33);
    TEST_ASSERT_TRUE(dispatcher.enqueue(event));
    dispatcher.tickFastLoop(0);

#if defined(UNIT_TEST)
    TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(manager.sentMessageCount()));
    const std::string& message = manager.sentMessages().front();
    DynamicJsonDocument doc(1536);
    TEST_ASSERT_FALSE(deserializeJson(doc, message));
    TEST_ASSERT_EQUAL_STRING("device.upsert", doc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(33, doc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(42, doc["payload"]["deviceId"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(99, doc["payload"]["typeId"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("device_updated", doc["payload"]["eventKind"].as<const char*>());
    TEST_ASSERT_FALSE(doc["payload"]["commandAccepted"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("detail", doc["payload"]["detail"].as<const char*>());
#endif
}

void test_ws_manager_stops_receiving_after_detach() {
    DeviceEventDispatcher dispatcher;
    PortalWebSocketManager manager(&dispatcher);
    manager.attachDispatcher();
    manager.detachDispatcher();
    manager.setClientCountForTest(1);

    const DeviceEvent event = makeDeviceEvent(DeviceEventKind::DeviceUpdated, 44);
    TEST_ASSERT_TRUE(dispatcher.enqueue(event));
    dispatcher.tickFastLoop(0);

#if defined(UNIT_TEST)
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(manager.sentMessageCount()));
#endif
}

void test_ws_manager_ignores_registry_persistence_cleared_events() {
    DeviceEventDispatcher dispatcher;
    PortalWebSocketManager manager(&dispatcher);
    manager.attachDispatcher();
    manager.setClientCountForTest(1);

    const DeviceEvent event = makeDeviceEvent(DeviceEventKind::PersistencePendingCleared, 45);
    TEST_ASSERT_TRUE(dispatcher.enqueue(event));
    dispatcher.tickFastLoop(0);

#if defined(UNIT_TEST)
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(manager.sentMessageCount()));
#endif
}

void test_ws_manager_broadcasts_snapshots_only_when_clients_are_connected() {
    PortalWebSocketManager manager;

    manager.publishSnapshotPayloadsForTest("{\"topic\":\"wifi.status\",\"revision\":1,\"payload\":{\"wifiStatus\":\"idle\"}}",
                                           "{\"topic\":\"ota.status\",\"revision\":1,\"payload\":{\"enabled\":true}}");
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(manager.sentMessageCount()));

    manager.setClientCountForTest(1);
    manager.publishSnapshotPayloadsForTest("{\"topic\":\"wifi.status\",\"revision\":1,\"payload\":{\"wifiStatus\":\"idle\"}}",
                                           "{\"topic\":\"ota.status\",\"revision\":1,\"payload\":{\"enabled\":true}}");
    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(manager.sentMessageCount()));

    manager.publishSnapshotPayloadsForTest("{\"topic\":\"wifi.status\",\"revision\":1,\"payload\":{\"wifiStatus\":\"idle\"}}",
                                           "{\"topic\":\"ota.status\",\"revision\":1,\"payload\":{\"enabled\":true}}");
    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(manager.sentMessageCount()));

    manager.setClientCountForTest(0);
    manager.publishSnapshotPayloadsForTest("{\"topic\":\"wifi.status\",\"revision\":2,\"payload\":{\"wifiStatus\":\"ap\"}}",
                                           "{\"topic\":\"ota.status\",\"revision\":2,\"payload\":{\"enabled\":false}}");
    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(manager.sentMessageCount()));

    manager.setClientCountForTest(1);
    manager.publishSnapshotPayloadsForTest("{\"topic\":\"wifi.status\",\"revision\":2,\"payload\":{\"wifiStatus\":\"ap\"}}",
                                           "{\"topic\":\"ota.status\",\"revision\":2,\"payload\":{\"enabled\":false}}");
    TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(manager.sentMessageCount()));
}

void test_ws_manager_resyncs_all_device_snapshots_for_new_clients() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    FixedDeviceIdSource idSource(42, 43);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeCreateRequest(), 10).ok());
    TEST_ASSERT_TRUE(registry.create(makeCreateRequest("Kitchen Lamp"), 20).ok());
    registry.tickFastLoop(21);

    PortalWebSocketManager manager(nullptr, &registry);
    manager.setClientCountForTest(1);
    manager.publishDeviceSnapshotsForTest();

#if defined(UNIT_TEST)
    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(manager.sentMessageCount()));
    bool sawFirst = false;
    bool sawSecond = false;
    for (const std::string& message : manager.sentMessages()) {
        DynamicJsonDocument doc(1536);
        TEST_ASSERT_FALSE(deserializeJson(doc, message));
        TEST_ASSERT_EQUAL_STRING("device.upsert", doc["topic"].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("snapshot", doc["payload"]["eventKind"].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("ready", doc["payload"]["runtime"]["effectiveStatus"].as<const char*>());
        const DeviceId deviceId = doc["payload"]["record"]["id"].as<DeviceId>();
        if (deviceId == 42) {
            sawFirst = true;
            TEST_ASSERT_EQUAL_STRING("Living Room Lamp", doc["payload"]["config"]["name"].as<const char*>());
        } else if (deviceId == 43) {
            sawSecond = true;
            TEST_ASSERT_EQUAL_STRING("Kitchen Lamp", doc["payload"]["config"]["name"].as<const char*>());
        } else {
            TEST_FAIL_MESSAGE("unexpected device snapshot");
        }
    }
    TEST_ASSERT_TRUE(sawFirst);
    TEST_ASSERT_TRUE(sawSecond);
#endif
}

void test_ws_status_messages_are_serializable() {
    const std::string ota = PortalWebSocketMessages::buildOtaStatus(true, false, 1234, 17);
    DynamicJsonDocument otaDoc(1024);
    TEST_ASSERT_FALSE(deserializeJson(otaDoc, ota));
    TEST_ASSERT_EQUAL_STRING("ota.status", otaDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(17, otaDoc["revision"].as<uint32_t>());
    TEST_ASSERT_TRUE(otaDoc["payload"]["enabled"].as<bool>());
    TEST_ASSERT_FALSE(otaDoc["payload"]["hasError"].as<bool>());
    TEST_ASSERT_EQUAL_UINT32(1234, otaDoc["payload"]["freeSketchSpace"].as<uint32_t>());

    const std::string system = PortalWebSocketMessages::buildSystemStatus("ok", false, 18);
    DynamicJsonDocument systemDoc(1024);
    TEST_ASSERT_FALSE(deserializeJson(systemDoc, system));
    TEST_ASSERT_EQUAL_STRING("system.status", systemDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(18, systemDoc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("ok", systemDoc["payload"]["status"].as<const char*>());
    TEST_ASSERT_FALSE(systemDoc["payload"]["rebooting"].as<bool>());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_ws_message_builders_create_compact_envelopes);
    RUN_TEST(test_ws_manager_attaches_and_detaches_from_dispatcher);
    RUN_TEST(test_ws_manager_receives_device_events_when_attached);
    RUN_TEST(test_ws_manager_stops_receiving_after_detach);
    RUN_TEST(test_ws_manager_ignores_registry_persistence_cleared_events);
    RUN_TEST(test_ws_manager_broadcasts_snapshots_only_when_clients_are_connected);
    RUN_TEST(test_ws_manager_resyncs_all_device_snapshots_for_new_clients);
    RUN_TEST(test_ws_status_messages_are_serializable);
    return UNITY_END();
}
