#include "devices/registry/DeviceRegistryEventReporter.h"

#include "integrations/common/DeviceEventDispatcher.h"

namespace ewfm {

void DeviceRegistryEventReporter::setDispatcher(DeviceEventDispatcher* dispatcher) {
    dispatcher_ = dispatcher;
}

void DeviceRegistryEventReporter::reset() {
    lastRuntimeStatuses_.clear();
}

void DeviceRegistryEventReporter::emit(const DeviceEvent& event) {
    if (dispatcher_ == nullptr) {
        return;
    }
    (void)dispatcher_->enqueue(event);
}

void DeviceRegistryEventReporter::trackRuntimeStatus(DeviceId deviceId, DeviceStatus status) {
    lastRuntimeStatuses_[deviceId] = status;
}

void DeviceRegistryEventReporter::clearRuntimeStatus(DeviceId deviceId) {
    lastRuntimeStatuses_.erase(deviceId);
}

void DeviceRegistryEventReporter::emitRuntimeStatusChangeIfNeeded(DeviceId deviceId, DeviceTypeId typeId, DeviceStatus currentStatus,
                                                                  uint32_t registryRevision, bool pendingPersistence, const char* detail) {
    const auto previousIt = lastRuntimeStatuses_.find(deviceId);
    const DeviceStatus previous = previousIt == lastRuntimeStatuses_.end() ? DeviceStatus::Unknown : previousIt->second;
    if (previous == currentStatus) {
        return;
    }

    DeviceEvent event{};
    event.kind = DeviceEventKind::StatusChanged;
    event.registryRevision = registryRevision;
    event.deviceId = deviceId;
    event.typeId = typeId;
    event.previousStatus = previous;
    event.status = currentStatus;
    event.pendingPersistence = pendingPersistence;
    setEventDetail(event, detail);
    emit(event);
    trackRuntimeStatus(deviceId, currentStatus);
}

void DeviceRegistryEventReporter::setEventDetail(DeviceEvent& event, const char* detail) {
    if (detail == nullptr) {
        event.detail.clear();
        return;
    }
    (void)event.detail.assign(detail);
}

} // namespace ewfm
