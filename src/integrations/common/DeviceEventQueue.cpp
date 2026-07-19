#include "integrations/common/DeviceEventQueue.h"

namespace ewfm {

bool DeviceEventQueue::push(const DeviceEvent& event) {
    if (full()) {
        return false;
    }
    events_[tail_] = event;
    tail_ = (tail_ + 1U) % kMaxEvents;
    ++count_;
    return true;
}

bool DeviceEventQueue::pop(DeviceEvent& event) {
    if (empty()) {
        return false;
    }
    event = events_[head_];
    head_ = (head_ + 1U) % kMaxEvents;
    --count_;
    return true;
}

bool DeviceEventQueue::peek(DeviceEvent& event) const {
    if (empty()) {
        return false;
    }
    event = events_[head_];
    return true;
}

void DeviceEventQueue::clear() {
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

bool DeviceEventQueue::empty() const {
    return count_ == 0;
}

bool DeviceEventQueue::full() const {
    return count_ >= kMaxEvents;
}

size_t DeviceEventQueue::size() const {
    return count_;
}

} // namespace ewfm
