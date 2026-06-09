#include "integrations/common/DeviceEventDispatcher.h"

namespace ewfm {

bool DeviceEventDispatcher::registerSink(IDeviceEventSink& sink) {
    return bus_.registerSink(sink);
}

bool DeviceEventDispatcher::unregisterSink(IDeviceEventSink& sink) {
    return bus_.unregisterSink(sink);
}

bool DeviceEventDispatcher::hasSink(const IDeviceEventSink& sink) const {
    return bus_.hasSink(sink);
}

size_t DeviceEventDispatcher::sinkCount() const {
    return bus_.sinkCount();
}

bool DeviceEventDispatcher::enqueue(const DeviceEvent& event) {
    if (!queue_.push(event)) {
        ++droppedEventCount_;
        return false;
    }
    return true;
}

size_t DeviceEventDispatcher::queuedEventCount() const {
    return queue_.size();
}

size_t DeviceEventDispatcher::droppedEventCount() const {
    return droppedEventCount_;
}

void DeviceEventDispatcher::clear() {
    queue_.clear();
}

void DeviceEventDispatcher::tickFastLoop(uint32_t now) {
    drain();
    bus_.tickFastLoop(now);
}

void DeviceEventDispatcher::tick100ms(uint32_t now) {
    drain();
    bus_.tick100ms(now);
}

void DeviceEventDispatcher::tick1s(uint32_t now) {
    drain();
    bus_.tick1s(now);
}

void DeviceEventDispatcher::drain() {
    DeviceEvent event{};
    while (queue_.pop(event)) {
        bus_.publish(event);
    }
}

} // namespace ewfm
