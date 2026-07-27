#pragma once

#include "devices/core/DeviceTypes.h"

namespace ewfm {

inline bool dependencyLinksHaveDuplicateDeviceIds(const DeviceDependencyLink* links, uint8_t depCount) {
    if (links == nullptr || depCount < 2U) {
        return false;
    }
    for (uint8_t left = 0; left < depCount; ++left) {
        const DeviceId leftDeviceId = links[left].deviceId;
        if (leftDeviceId == 0U) {
            continue;
        }
        for (uint8_t right = static_cast<uint8_t>(left + 1U); right < depCount; ++right) {
            if (leftDeviceId == links[right].deviceId) {
                return true;
            }
        }
    }
    return false;
}

inline bool dependencyLinksHaveDuplicateDeviceIds(const DeviceDependencyLink* links, const uint8_t* indices, uint8_t indexCount) {
    if (links == nullptr || indices == nullptr || indexCount < 2U) {
        return false;
    }
    for (uint8_t left = 0; left < indexCount; ++left) {
        const DeviceId leftDeviceId = links[indices[left]].deviceId;
        if (leftDeviceId == 0U) {
            continue;
        }
        for (uint8_t right = static_cast<uint8_t>(left + 1U); right < indexCount; ++right) {
            if (leftDeviceId == links[indices[right]].deviceId) {
                return true;
            }
        }
    }
    return false;
}

} // namespace ewfm
