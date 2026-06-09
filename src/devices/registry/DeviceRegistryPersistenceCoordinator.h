#pragma once

#include "devices/core/DeviceTypes.h"

#include <algorithm>
#include <map>
#include <vector>

namespace ewfm {

class DeviceRegistryPersistenceCoordinator {
public:
    static constexpr uint32_t kDirtyTimestampUnset = 0xFFFFFFFFUL;

    void reset(uint32_t now);

    bool hasPendingPersistence() const;
    bool dirtyIndex() const;
    std::vector<DeviceId> dirtyConfigRecordIds() const;
    std::vector<DeviceId> dirtyRetainedStateIds() const;
    uint32_t firstDirtyAt() const;
    uint32_t lastChangeAt() const;
    bool shouldFlush(uint32_t now, uint32_t debounceMs, uint32_t maxDelayMs) const;

    void markIndexDirty(uint32_t now);
    void markConfigDirty(DeviceId deviceId, uint32_t now);
    void markRetainedDirty(DeviceId deviceId, uint32_t now);
    void clearConfigDirtyAfterImmediateFlush();
    void clearRetainedTracking(DeviceId deviceId);

    bool hasConfigPersistenceWork() const;
    bool hasRetainedPersistenceWork() const;
    bool hasAnyPersistenceWork() const;

    const std::vector<DeviceId>& dirtyConfigRecordIdsRef() const;
    const std::vector<DeviceId>& dirtyRetainedStateIdsRef() const;
    const std::map<DeviceId, RetainedStateRecord>& pendingRetainedStateRecords() const;
    std::map<DeviceId, RetainedStateRecord>& pendingRetainedStateRecords();

    void clearConfigDirtyAfterPersisted();
    void clearRetainedDirtyAfterPersisted();
    void markRetainedPersisted(DeviceId deviceId);
    void markClean();

private:
    void markDirty(uint32_t now);
    bool eraseDirtyId(std::vector<DeviceId>& dirtyIds, DeviceId deviceId);

    bool dirty_{false};
    bool dirtyIndex_{false};
    std::vector<DeviceId> dirtyConfigRecordIds_{};
    std::vector<DeviceId> dirtyRetainedStateIds_{};
    std::map<DeviceId, RetainedStateRecord> pendingRetainedStateRecords_{};
    uint32_t firstDirtyAt_{kDirtyTimestampUnset};
    uint32_t lastChangeAt_{kDirtyTimestampUnset};
};

} // namespace ewfm
