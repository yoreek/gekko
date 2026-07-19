#pragma once

#include "integrations/common/DeviceEventBus.h"
#include "integrations/common/DeviceEventQueue.h"

#include <cstddef>

namespace ewfm {

class DeviceEventDispatcher {
public:
    DeviceEventDispatcher() = default;

    bool registerSink(IDeviceEventSink& sink);
    bool unregisterSink(IDeviceEventSink& sink);
    bool hasSink(const IDeviceEventSink& sink) const;
    size_t sinkCount() const;

    bool enqueue(const DeviceEvent& event);
    size_t queuedEventCount() const;
    size_t droppedEventCount() const;
    void clear();

    void tickFastLoop(uint32_t now);
    void tick100ms(uint32_t now);
    void tick1s(uint32_t now);

private:
    void drain();

    DeviceEventBus bus_{};
    DeviceEventQueue queue_{};
    size_t droppedEventCount_{0};
};

} // namespace ewfm
