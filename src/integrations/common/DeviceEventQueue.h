#pragma once

#include "devices/core/DeviceTypes.h"

#include <array>
#include <cstddef>

namespace ewfm {

class DeviceEventQueue {
public:
    static constexpr size_t kMaxEvents = 16;

    bool push(const DeviceEvent& event);
    bool pop(DeviceEvent& event);
    bool peek(DeviceEvent& event) const;
    void clear();

    bool empty() const;
    bool full() const;
    size_t size() const;
    constexpr size_t capacity() const {
        return kMaxEvents;
    }

private:
    std::array<DeviceEvent, kMaxEvents> events_{};
    size_t head_{0};
    size_t tail_{0};
    size_t count_{0};
};

} // namespace ewfm
