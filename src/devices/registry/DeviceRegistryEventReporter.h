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
                                         bool pendingPersistence, const char* detail, const char* name = nullptr,
                                         const char* typeName = nullptr);

    static void setEventDetail(DeviceEvent& event, const char* detail);
    static void setEventMetadata(DeviceEvent& event, const char* name, const char* typeName);

private:
    DeviceEventDispatcher* dispatcher_{nullptr};
    std::map<DeviceId, DeviceStatus> lastRuntimeStatuses_{};
};

} // namespace ewfm
