#include "devices/registry/DeviceRegistryEventReporter.h"
#include "integrations/common/DeviceEventDispatcher.h"

#include <unity.h>

using namespace ewfm;

namespace {

struct RecordingSink final : public IDeviceEventSink {
    void onDeviceEvent(const DeviceEvent& event) override {
        events.push_back(event);
    }
    void tickFastLoop(uint32_t now) override {
        lastNow = now;
    }
    void tick100ms(uint32_t now) override {
        lastNow = now;
    }
    void tick1s(uint32_t now) override {
        lastNow = now;
    }

    std::vector<DeviceEvent> events{};
    uint32_t lastNow{0};
};

} // namespace

void test_event_reporter_emits_and_tracks_runtime_status() {
    DeviceEventDispatcher dispatcher;
    RecordingSink sink{};
    TEST_ASSERT_TRUE(dispatcher.registerSink(sink));

    DeviceRegistryEventReporter reporter(&dispatcher);

    DeviceEvent event{};
    event.kind = DeviceEventKind::DeviceCreated;
    event.deviceId = 11;
    DeviceRegistryEventReporter::setEventDetail(event, "created");
    reporter.emit(event);
    dispatcher.tickFastLoop(10);

    TEST_ASSERT_EQUAL_UINT32(1, sink.events.size());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(DeviceEventKind::DeviceCreated), static_cast<uint32_t>(sink.events[0].kind));
    TEST_ASSERT_EQUAL_STRING("device_created", sink.events[0].eventKind.c_str());
    TEST_ASSERT_EQUAL_STRING("created", sink.events[0].detail.c_str());

    reporter.emitRuntimeStatusChangeIfNeeded(11, 1, DeviceStatus::Ready, 7, false, "status changed");
    dispatcher.tickFastLoop(20);
    TEST_ASSERT_EQUAL_UINT32(2, sink.events.size());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(DeviceEventKind::StatusChanged), static_cast<uint32_t>(sink.events[1].kind));
    TEST_ASSERT_EQUAL_STRING("status_changed", sink.events[1].eventKind.c_str());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(DeviceStatus::Unknown), static_cast<uint32_t>(sink.events[1].previousStatus));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(DeviceStatus::Ready), static_cast<uint32_t>(sink.events[1].status));

    reporter.emitRuntimeStatusChangeIfNeeded(11, 1, DeviceStatus::Ready, 8, false, "status changed");
    dispatcher.tickFastLoop(30);
    TEST_ASSERT_EQUAL_UINT32(2, sink.events.size());

    reporter.reset();
    reporter.emitRuntimeStatusChangeIfNeeded(11, 1, DeviceStatus::Ready, 9, true, "status changed");
    dispatcher.tickFastLoop(40);
    TEST_ASSERT_EQUAL_UINT32(3, sink.events.size());
    TEST_ASSERT_TRUE(sink.events[2].pendingPersistence);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_event_reporter_emits_and_tracks_runtime_status);
    return UNITY_END();
}
