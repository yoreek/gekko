#pragma once

#include "devices/core/DeviceTypes.h"

#include <array>
#include <cstddef>

namespace ewfm {

class IDeviceEventSink {
public:
    IDeviceEventSink() = default;
    IDeviceEventSink(const IDeviceEventSink&) = delete;
    IDeviceEventSink& operator=(const IDeviceEventSink&) = delete;
    IDeviceEventSink(IDeviceEventSink&&) = delete;
    IDeviceEventSink& operator=(IDeviceEventSink&&) = delete;
    virtual ~IDeviceEventSink() = default;

    virtual void onDeviceEvent(const DeviceEvent& event) = 0;
    virtual void tickFastLoop(uint32_t now) = 0;
    virtual void tick100ms(uint32_t now) = 0;
    virtual void tick1s(uint32_t now) = 0;
};

class DeviceEventBus {
public:
    static constexpr size_t kMaxSinks = 8;

    bool registerSink(IDeviceEventSink& sink);
    bool unregisterSink(IDeviceEventSink& sink);
    bool hasSink(const IDeviceEventSink& sink) const;
    size_t sinkCount() const;

    void publish(const DeviceEvent& event) const;
    void tickFastLoop(uint32_t now) const;
    void tick100ms(uint32_t now) const;
    void tick1s(uint32_t now) const;

private:
    std::array<IDeviceEventSink*, kMaxSinks> sinks_{};
    size_t sinkCount_{0};
};

} // namespace ewfm
