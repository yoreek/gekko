#include "integrations/common/DeviceEventBus.h"

namespace ewfm {

bool DeviceEventBus::registerSink(IDeviceEventSink& sink) {
    if (hasSink(sink) || sinkCount_ >= kMaxSinks) {
        return false;
    }
    sinks_[sinkCount_++] = &sink;
    return true;
}

bool DeviceEventBus::unregisterSink(IDeviceEventSink& sink) {
    for (size_t index = 0; index < sinkCount_; ++index) {
        if (sinks_[index] != &sink) {
            continue;
        }
        for (size_t moveIndex = index + 1; moveIndex < sinkCount_; ++moveIndex) {
            sinks_[moveIndex - 1] = sinks_[moveIndex];
        }
        sinks_[sinkCount_ - 1] = nullptr;
        --sinkCount_;
        return true;
    }
    return false;
}

bool DeviceEventBus::hasSink(const IDeviceEventSink& sink) const {
    for (size_t index = 0; index < sinkCount_; ++index) {
        if (sinks_[index] == &sink) {
            return true;
        }
    }
    return false;
}

size_t DeviceEventBus::sinkCount() const {
    return sinkCount_;
}

void DeviceEventBus::publish(const DeviceEvent& event) const {
    for (size_t index = 0; index < sinkCount_; ++index) {
        auto* sink = sinks_[index];
        if (sink != nullptr) {
            sink->onDeviceEvent(event);
        }
    }
}

void DeviceEventBus::tickFastLoop(uint32_t now) const {
    for (size_t index = 0; index < sinkCount_; ++index) {
        auto* sink = sinks_[index];
        if (sink != nullptr) {
            sink->tickFastLoop(now);
        }
    }
}

void DeviceEventBus::tick100ms(uint32_t now) const {
    for (size_t index = 0; index < sinkCount_; ++index) {
        auto* sink = sinks_[index];
        if (sink != nullptr) {
            sink->tick100ms(now);
        }
    }
}

void DeviceEventBus::tick1s(uint32_t now) const {
    for (size_t index = 0; index < sinkCount_; ++index) {
        auto* sink = sinks_[index];
        if (sink != nullptr) {
            sink->tick1s(now);
        }
    }
}

} // namespace ewfm
