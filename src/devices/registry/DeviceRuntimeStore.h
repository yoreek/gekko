#pragma once

#include "devices/core/DeviceTypes.h"

#include <map>
#include <memory>

namespace ewfm {

struct DeviceRuntimeSlot {
    const DeviceTypeDescriptor* descriptor{nullptr};
    std::unique_ptr<IDeviceRuntime> runtime{};
};

using DeviceRuntimeMap = std::map<DeviceId, DeviceRuntimeSlot>;

} // namespace ewfm
