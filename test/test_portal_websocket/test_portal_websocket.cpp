#include "integrations/common/DeviceEventDispatcher.h"
#include "portal/ws/PortalWebSocketManager.h"

#include <ArduinoJson.h>
#include <unity.h>

using namespace ewfm;

namespace {

DeviceEvent makeDeviceEvent(const DeviceEventKind kind, const uint32_t revision, const DeviceId deviceId = 42) {
    DeviceEvent event{};
    event.kind = kind;
    event.registryRevision = revision;
    event.configRevision = 7;
    event.deviceId = deviceId;
    event.typeId = 99;
    event.previousStatus = DeviceStatus::Starting;
    event.status = DeviceStatus::Ready;
    event.pendingPersistence = true;
    event.commandAccepted = kind == DeviceEventKind::CommandAccepted;
    (void)event.detail.assign("detail");
    return event;
}

} // namespace

void test_ws_message_builders_create_compact_envelopes() {
    DeviceEvent upsertEvent = makeDeviceEvent(DeviceEventKind::DeviceUpdated, 12);
    const std::string upsert = PortalWebSocketMessages::buildDeviceUpsert(upsertEvent);
    DynamicJsonDocument upsertDoc(1536);
    TEST_ASSERT_FALSE(deserializeJson(upsertDoc, upsert));
    TEST_ASSERT_EQUAL_STRING("device.upsert", upsertDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(12, upsertDoc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(42, upsertDoc["payload"]["device_id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(99, upsertDoc["payload"]["type_id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(7, upsertDoc["payload"]["config_revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint8_t>(DeviceStatus::Starting), upsertDoc["payload"]["previous_status"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint8_t>(DeviceStatus::Ready), upsertDoc["payload"]["status"].as<uint8_t>());
    TEST_ASSERT_TRUE(upsertDoc["payload"]["pending_persistence"].as<bool>());
    TEST_ASSERT_FALSE(upsertDoc["payload"]["command_accepted"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("detail", upsertDoc["payload"]["detail"].as<const char*>());

    DeviceEvent removedEvent = makeDeviceEvent(DeviceEventKind::DeviceDeleted, 13);
    const std::string removed = PortalWebSocketMessages::buildDeviceRemove(removedEvent);
    DynamicJsonDocument removedDoc(1024);
    TEST_ASSERT_FALSE(deserializeJson(removedDoc, removed));
    TEST_ASSERT_EQUAL_STRING("device.remove", removedDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(13, removedDoc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(42, removedDoc["payload"]["device_id"].as<uint32_t>());
    TEST_ASSERT_TRUE(removedDoc["payload"]["pending_persistence"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("detail", removedDoc["payload"]["detail"].as<const char*>());

    const std::string hello = PortalWebSocketMessages::buildHello(9, 8, 2);
    DynamicJsonDocument helloDoc(1024);
    TEST_ASSERT_FALSE(deserializeJson(helloDoc, hello));
    TEST_ASSERT_EQUAL_STRING("hello", helloDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(9, helloDoc["revision"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("connected", helloDoc["payload"]["state"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(2, helloDoc["payload"]["clients"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(8, helloDoc["payload"]["registry_revision"].as<uint32_t>());
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
    TEST_ASSERT_EQUAL_UINT32(42, doc["payload"]["device_id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(99, doc["payload"]["type_id"].as<uint32_t>());
    TEST_ASSERT_FALSE(doc["payload"]["command_accepted"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("detail", doc["payload"]["detail"].as<const char*>());
#endif
}

void test_ws_manager_stops_receiving_after_detach() {
    DeviceEventDispatcher dispatcher;
    PortalWebSocketManager manager(&dispatcher);
    manager.attachDispatcher();
    manager.detachDispatcher();

    const DeviceEvent event = makeDeviceEvent(DeviceEventKind::DeviceUpdated, 44);
    TEST_ASSERT_TRUE(dispatcher.enqueue(event));
    dispatcher.tickFastLoop(0);

#if defined(UNIT_TEST)
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(manager.sentMessageCount()));
#endif
}

void test_ws_status_messages_are_serializable() {
    const std::string ota = PortalWebSocketMessages::buildOtaStatus(true, false, 1234, 17);
    DynamicJsonDocument otaDoc(1024);
    TEST_ASSERT_FALSE(deserializeJson(otaDoc, ota));
    TEST_ASSERT_EQUAL_STRING("ota.status", otaDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(17, otaDoc["revision"].as<uint32_t>());
    TEST_ASSERT_TRUE(otaDoc["payload"]["enabled"].as<bool>());
    TEST_ASSERT_FALSE(otaDoc["payload"]["has_error"].as<bool>());
    TEST_ASSERT_EQUAL_UINT32(1234, otaDoc["payload"]["free_sketch_space"].as<uint32_t>());

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
    RUN_TEST(test_ws_status_messages_are_serializable);
    return UNITY_END();
}
