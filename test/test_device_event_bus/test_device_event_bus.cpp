#include "devices/core/DeviceTypes.h"
#include "integrations/common/DeviceEventBus.h"
#include "integrations/common/DeviceEventDispatcher.h"
#include "integrations/common/DeviceEventQueue.h"

#include <string>
#include <unity.h>

using namespace ewfm;

namespace {

struct RecordingSink final : public IDeviceEventSink {
    void onDeviceEvent(const DeviceEvent& event) override {
        events.push_back(event);
        onEventCount += 1;
    }
    void tickFastLoop(uint32_t now) override {
        fastLoopCount += 1;
        lastNow = now;
    }
    void tick100ms(uint32_t now) override {
        tick100msCount += 1;
        lastNow = now;
    }
    void tick1s(uint32_t now) override {
        tick1sCount += 1;
        lastNow = now;
    }

    uint32_t onEventCount{0};
    uint32_t fastLoopCount{0};
    uint32_t tick100msCount{0};
    uint32_t tick1sCount{0};
    uint32_t lastNow{0};
    std::vector<DeviceEvent> events{};
};

} // namespace

void test_device_event_payload_is_bounded() {
    DeviceEvent event{};
    TEST_ASSERT_TRUE(event.detail.assign("ok"));
    TEST_ASSERT_TRUE(event.detail.valid());
    TEST_ASSERT_EQUAL_STRING("ok", event.detail.c_str());

    std::string oversized(kMaxDeviceEventBytes + 1U, 'x');
    TEST_ASSERT_FALSE(event.detail.assign(oversized));
    TEST_ASSERT_FALSE(event.detail.valid());
    TEST_ASSERT_TRUE(event.detail.empty());
}

void test_device_command_payload_is_bounded() {
    DeviceCommand command{DeviceCommandType::Custom, 77, "output=1", DevicePersistencePolicy::Delayed};
    TEST_ASSERT_TRUE(command.valid());
    TEST_ASSERT_TRUE(command.payload.equals("output=1"));

    std::string oversized(kMaxDeviceEventBytes + 1U, 'y');
    DeviceCommand invalid{DeviceCommandType::Custom, 78, oversized, DevicePersistencePolicy::Delayed};
    TEST_ASSERT_FALSE(invalid.valid());
    TEST_ASSERT_TRUE(invalid.payload.empty());
}

void test_device_event_bus_registers_fans_out_and_ticks() {
    DeviceEventBus bus;
    RecordingSink first{};
    RecordingSink second{};

    TEST_ASSERT_TRUE(bus.registerSink(first));
    TEST_ASSERT_TRUE(bus.registerSink(second));
    TEST_ASSERT_FALSE(bus.registerSink(first));
    TEST_ASSERT_TRUE(bus.hasSink(first));
    TEST_ASSERT_TRUE(bus.hasSink(second));
    TEST_ASSERT_EQUAL_UINT32(2, bus.sinkCount());

    DeviceEvent event{};
    event.kind = DeviceEventKind::DeviceCreated;
    event.deviceId = 123;
    event.registryRevision = 7;
    event.status = DeviceStatus::Ready;
    event.detail.assign("created");
    bus.publish(event);

    TEST_ASSERT_EQUAL_UINT32(1, first.onEventCount);
    TEST_ASSERT_EQUAL_UINT32(1, second.onEventCount);
    TEST_ASSERT_EQUAL_UINT32(123, first.events[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(DeviceEventKind::DeviceCreated), static_cast<uint32_t>(first.events[0].kind));
    TEST_ASSERT_EQUAL_STRING("created", first.events[0].detail.c_str());

    bus.tickFastLoop(11);
    bus.tick100ms(22);
    bus.tick1s(33);

    TEST_ASSERT_EQUAL_UINT32(1, first.fastLoopCount);
    TEST_ASSERT_EQUAL_UINT32(1, first.tick100msCount);
    TEST_ASSERT_EQUAL_UINT32(1, first.tick1sCount);
    TEST_ASSERT_EQUAL_UINT32(33, first.lastNow);
    TEST_ASSERT_EQUAL_UINT32(1, second.fastLoopCount);
    TEST_ASSERT_EQUAL_UINT32(1, second.tick100msCount);
    TEST_ASSERT_EQUAL_UINT32(1, second.tick1sCount);
    TEST_ASSERT_EQUAL_UINT32(33, second.lastNow);

    TEST_ASSERT_TRUE(bus.unregisterSink(first));
    TEST_ASSERT_FALSE(bus.hasSink(first));
    TEST_ASSERT_EQUAL_UINT32(1, bus.sinkCount());
}

void test_device_event_bus_caps_sink_count() {
    DeviceEventBus bus;
    RecordingSink sinks[DeviceEventBus::kMaxSinks + 1U]{};

    for (size_t i = 0; i < DeviceEventBus::kMaxSinks; ++i) {
        TEST_ASSERT_TRUE(bus.registerSink(sinks[i]));
    }
    TEST_ASSERT_FALSE(bus.registerSink(sinks[DeviceEventBus::kMaxSinks]));
    TEST_ASSERT_EQUAL_UINT32(DeviceEventBus::kMaxSinks, bus.sinkCount());
}

void test_device_event_queue_preserves_fifo_and_bounds() {
    DeviceEventQueue queue;
    TEST_ASSERT_TRUE(queue.empty());
    TEST_ASSERT_EQUAL_UINT32(0, queue.size());
    TEST_ASSERT_EQUAL_UINT32(DeviceEventQueue::kMaxEvents, queue.capacity());

    DeviceEvent first{};
    first.kind = DeviceEventKind::DeviceCreated;
    first.deviceId = 1;
    TEST_ASSERT_TRUE(queue.push(first));

    DeviceEvent second{};
    second.kind = DeviceEventKind::DeviceDeleted;
    second.deviceId = 2;
    TEST_ASSERT_TRUE(queue.push(second));
    TEST_ASSERT_FALSE(queue.empty());
    TEST_ASSERT_EQUAL_UINT32(2, queue.size());

    DeviceEvent peeked{};
    TEST_ASSERT_TRUE(queue.peek(peeked));
    TEST_ASSERT_EQUAL_UINT32(1, peeked.deviceId);

    DeviceEvent popped{};
    TEST_ASSERT_TRUE(queue.pop(popped));
    TEST_ASSERT_EQUAL_UINT32(1, popped.deviceId);
    TEST_ASSERT_TRUE(queue.pop(popped));
    TEST_ASSERT_EQUAL_UINT32(2, popped.deviceId);
    TEST_ASSERT_TRUE(queue.empty());
}

void test_device_event_dispatcher_drains_queue_in_order() {
    DeviceEventDispatcher dispatcher;
    RecordingSink sink{};

    TEST_ASSERT_TRUE(dispatcher.registerSink(sink));

    DeviceEvent first{};
    first.kind = DeviceEventKind::DeviceCreated;
    first.deviceId = 11;
    first.detail.assign("first");
    DeviceEvent second{};
    second.kind = DeviceEventKind::StatusChanged;
    second.deviceId = 12;
    second.detail.assign("second");

    TEST_ASSERT_TRUE(dispatcher.enqueue(first));
    TEST_ASSERT_TRUE(dispatcher.enqueue(second));
    TEST_ASSERT_EQUAL_UINT32(2, dispatcher.queuedEventCount());

    dispatcher.tickFastLoop(44);

    TEST_ASSERT_EQUAL_UINT32(2, sink.onEventCount);
    TEST_ASSERT_EQUAL_UINT32(0, dispatcher.queuedEventCount());
    TEST_ASSERT_EQUAL_UINT32(11, sink.events[0].deviceId);
    TEST_ASSERT_EQUAL_STRING("first", sink.events[0].detail.c_str());
    TEST_ASSERT_EQUAL_UINT32(12, sink.events[1].deviceId);
    TEST_ASSERT_EQUAL_STRING("second", sink.events[1].detail.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, sink.fastLoopCount);
    TEST_ASSERT_EQUAL_UINT32(44, sink.lastNow);
}

void test_device_event_dispatcher_counts_dropped_events_when_full() {
    DeviceEventDispatcher dispatcher;
    RecordingSink sink{};
    TEST_ASSERT_TRUE(dispatcher.registerSink(sink));

    DeviceEvent event{};
    event.kind = DeviceEventKind::StatusChanged;
    event.deviceId = 99;

    for (size_t i = 0; i < DeviceEventQueue::kMaxEvents; ++i) {
        TEST_ASSERT_TRUE(dispatcher.enqueue(event));
    }
    TEST_ASSERT_FALSE(dispatcher.enqueue(event));
    TEST_ASSERT_EQUAL_UINT32(1, dispatcher.droppedEventCount());

    dispatcher.tick100ms(55);
    TEST_ASSERT_EQUAL_UINT32(DeviceEventQueue::kMaxEvents, sink.onEventCount);
    TEST_ASSERT_EQUAL_UINT32(55, sink.lastNow);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_device_event_payload_is_bounded);
    RUN_TEST(test_device_command_payload_is_bounded);
    RUN_TEST(test_device_event_bus_registers_fans_out_and_ticks);
    RUN_TEST(test_device_event_bus_caps_sink_count);
    RUN_TEST(test_device_event_queue_preserves_fifo_and_bounds);
    RUN_TEST(test_device_event_dispatcher_drains_queue_in_order);
    RUN_TEST(test_device_event_dispatcher_counts_dropped_events_when_full);
    return UNITY_END();
}
