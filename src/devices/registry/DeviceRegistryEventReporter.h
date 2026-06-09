#pragma once

#include "devices/core/DeviceTypes.h"

#include <map>

namespace ewfm {

class DeviceEventDispatcher;

class DeviceRegistryEventReporter {
public:
    explicit DeviceRegistryEventReporter(DeviceEventDispatcher* dispatcher = nullptr) : dispatcher_(dispatcher) {}

    void setDispatcher(DeviceEventDispatcher* dispatcher);
    void reset();
    void emit(const DeviceEvent& event);
    void trackRuntimeStatus(DeviceId deviceId, DeviceStatus status);
    void clearRuntimeStatus(DeviceId deviceId);
    void emitRuntimeStatusChangeIfNeeded(DeviceId deviceId, DeviceTypeId typeId, DeviceStatus currentStatus, uint32_t registryRevision,
                                         bool pendingPersistence, const char* detail);

    static void setEventDetail(DeviceEvent& event, const char* detail);

private:
    DeviceEventDispatcher* dispatcher_{nullptr};
    std::map<DeviceId, DeviceStatus> lastRuntimeStatuses_{};
};

} // namespace ewfm
