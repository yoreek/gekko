#include "devices/registry/DeviceRegistryPersistenceCoordinator.h"

namespace ewfm {

void DeviceRegistryPersistenceCoordinator::reset(uint32_t now) {
    dirty_ = false;
    dirtyIndex_ = false;
    dirtyConfigRecordIds_.clear();
    dirtyRetainedStateIds_.clear();
    pendingRetainedStateRecords_.clear();
    firstDirtyAt_ = kDirtyTimestampUnset;
    lastChangeAt_ = now;
}

bool DeviceRegistryPersistenceCoordinator::hasPendingPersistence() const {
    return dirty_;
}

bool DeviceRegistryPersistenceCoordinator::dirtyIndex() const {
    return dirtyIndex_;
}

std::vector<DeviceId> DeviceRegistryPersistenceCoordinator::dirtyConfigRecordIds() const {
    return dirtyConfigRecordIds_;
}

std::vector<DeviceId> DeviceRegistryPersistenceCoordinator::dirtyRetainedStateIds() const {
    return dirtyRetainedStateIds_;
}

uint32_t DeviceRegistryPersistenceCoordinator::firstDirtyAt() const {
    return firstDirtyAt_ == kDirtyTimestampUnset ? 0 : firstDirtyAt_;
}

uint32_t DeviceRegistryPersistenceCoordinator::lastChangeAt() const {
    return lastChangeAt_ == kDirtyTimestampUnset ? 0 : lastChangeAt_;
}

bool DeviceRegistryPersistenceCoordinator::shouldFlush(uint32_t now, uint32_t debounceMs, uint32_t maxDelayMs) const {
    if (!dirty_ || firstDirtyAt_ == kDirtyTimestampUnset) {
        return false;
    }

    const uint32_t elapsed = static_cast<uint32_t>(now - lastChangeAt_);
    const uint32_t dirtyAge = static_cast<uint32_t>(now - firstDirtyAt_);
    return elapsed >= debounceMs || dirtyAge >= maxDelayMs;
}

void DeviceRegistryPersistenceCoordinator::markIndexDirty(uint32_t now) {
    markDirty(now);
    dirtyIndex_ = true;
}

void DeviceRegistryPersistenceCoordinator::markConfigDirty(DeviceId deviceId, uint32_t now) {
    markDirty(now);
    if (std::find(dirtyConfigRecordIds_.begin(), dirtyConfigRecordIds_.end(), deviceId) == dirtyConfigRecordIds_.end()) {
        dirtyConfigRecordIds_.push_back(deviceId);
    }
}

void DeviceRegistryPersistenceCoordinator::markRetainedDirty(DeviceId deviceId, uint32_t now) {
    markDirty(now);
    if (std::find(dirtyRetainedStateIds_.begin(), dirtyRetainedStateIds_.end(), deviceId) == dirtyRetainedStateIds_.end()) {
        dirtyRetainedStateIds_.push_back(deviceId);
    }
}

void DeviceRegistryPersistenceCoordinator::clearConfigDirtyAfterImmediateFlush() {
    dirtyIndex_ = false;
    dirtyConfigRecordIds_.clear();
    if (dirtyRetainedStateIds_.empty() && pendingRetainedStateRecords_.empty()) {
        markClean();
        return;
    }
    dirty_ = true;
}

void DeviceRegistryPersistenceCoordinator::clearRetainedTracking(DeviceId deviceId) {
    eraseDirtyId(dirtyRetainedStateIds_, deviceId);
    pendingRetainedStateRecords_.erase(deviceId);
}

bool DeviceRegistryPersistenceCoordinator::hasConfigPersistenceWork() const {
    return dirtyIndex_ || !dirtyConfigRecordIds_.empty();
}

bool DeviceRegistryPersistenceCoordinator::hasRetainedPersistenceWork() const {
    return !dirtyRetainedStateIds_.empty();
}

bool DeviceRegistryPersistenceCoordinator::hasAnyPersistenceWork() const {
    return dirtyIndex_ || !dirtyConfigRecordIds_.empty() || !dirtyRetainedStateIds_.empty() || !pendingRetainedStateRecords_.empty();
}

const std::vector<DeviceId>& DeviceRegistryPersistenceCoordinator::dirtyConfigRecordIdsRef() const {
    return dirtyConfigRecordIds_;
}

const std::vector<DeviceId>& DeviceRegistryPersistenceCoordinator::dirtyRetainedStateIdsRef() const {
    return dirtyRetainedStateIds_;
}

const std::map<DeviceId, RetainedStateRecord>& DeviceRegistryPersistenceCoordinator::pendingRetainedStateRecords() const {
    return pendingRetainedStateRecords_;
}

std::map<DeviceId, RetainedStateRecord>& DeviceRegistryPersistenceCoordinator::pendingRetainedStateRecords() {
    return pendingRetainedStateRecords_;
}

void DeviceRegistryPersistenceCoordinator::clearConfigDirtyAfterPersisted() {
    dirtyIndex_ = false;
    dirtyConfigRecordIds_.clear();
}

void DeviceRegistryPersistenceCoordinator::clearRetainedDirtyAfterPersisted() {
    dirtyRetainedStateIds_.clear();
}

void DeviceRegistryPersistenceCoordinator::markRetainedPersisted(DeviceId deviceId) {
    pendingRetainedStateRecords_.erase(deviceId);
}

void DeviceRegistryPersistenceCoordinator::markClean() {
    dirty_ = false;
    dirtyIndex_ = false;
    dirtyConfigRecordIds_.clear();
    dirtyRetainedStateIds_.clear();
    firstDirtyAt_ = kDirtyTimestampUnset;
    lastChangeAt_ = kDirtyTimestampUnset;
    pendingRetainedStateRecords_.clear();
}

void DeviceRegistryPersistenceCoordinator::markDirty(uint32_t now) {
    if (!dirty_) {
        firstDirtyAt_ = now;
    }
    dirty_ = true;
    lastChangeAt_ = now;
}

bool DeviceRegistryPersistenceCoordinator::eraseDirtyId(std::vector<DeviceId>& dirtyIds, DeviceId deviceId) {
    const auto it = std::remove(dirtyIds.begin(), dirtyIds.end(), deviceId);
    if (it == dirtyIds.end()) {
        return false;
    }
    dirtyIds.erase(it, dirtyIds.end());
    return true;
}

} // namespace ewfm
