#include "devices/core/DeviceRuntimeBase.h"

#include <algorithm>

namespace ewfm {

DeviceRuntimeBase::DeviceRuntimeBase(PState initialState) : StateMachine(initialState) {}

void DeviceRuntimeBase::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    (void)config;
    deviceId_ = record.header.deviceId;
    typeId_ = record.header.typeId;
    configVersion_ = record.header.configVersion;
    configRevision_ = record.header.configRevision;
    dependencyCount_ = record.depCount;
    for (uint8_t index = 0; index < dependencyCount_ && index < kMaxDeviceDependencies; ++index) {
        dependencyLinks_[index] = record.deps[index];
        dependencyRuntimes_[index] = nullptr;
    }
    persistencePolicy_ = record.persistencePolicy;
}

void DeviceRuntimeBase::begin(uint32_t now) {
    startRequested_ = true;
    tickRuntime(now);
}

void DeviceRuntimeBase::tickFastLoop(uint32_t now) {
    tickRuntime(now);
}

void DeviceRuntimeBase::tick100ms(uint32_t now) {
    tickRuntime(now);
}

void DeviceRuntimeBase::tick1s(uint32_t now) {
    tickRuntime(now);
}

void DeviceRuntimeBase::end(uint32_t now) {
    (void)now;
}

void DeviceRuntimeBase::resetStateMachine(uint32_t now) {
    StateMachine::reset(now);
    startRequested_ = true;
    clearReconfigureRequested();
    clearFaultRequested();
}

void DeviceRuntimeBase::setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) {
    for (uint8_t index = 0; index < dependencyCount_ && index < kMaxDeviceDependencies; ++index) {
        if (dependencyLinks_[index].role == role) {
            dependencyRuntimes_[index] = dependencyRuntime;
            return;
        }
    }
}

void DeviceRuntimeBase::setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) {
    if (index >= dependencyCount_ || index >= kMaxDeviceDependencies) {
        return;
    }
    dependencyRuntimes_[index] = dependencyRuntime;
}

IDeviceRuntime* DeviceRuntimeBase::dependencyRuntime(DeviceRole role) const {
    for (uint8_t index = 0; index < dependencyCount_ && index < kMaxDeviceDependencies; ++index) {
        if (dependencyLinks_[index].role == role) {
            return dependencyRuntimes_[index];
        }
    }
    return nullptr;
}

IDeviceRuntime* DeviceRuntimeBase::dependencyRuntimeAt(uint8_t index) const {
    if (index >= dependencyCount_ || index >= kMaxDeviceDependencies) {
        return nullptr;
    }
    return dependencyRuntimes_[index];
}

uint8_t DeviceRuntimeBase::dependencyCount() const {
    return dependencyCount_;
}

const DeviceDependencyLink* DeviceRuntimeBase::dependencyLinks() const {
    return dependencyLinks_.data();
}

void DeviceRuntimeBase::attachDependentRuntime(IDeviceRuntime* dependentRuntime) {
    if (dependentRuntime == nullptr || hasDependentRuntime(dependentRuntime)) {
        return;
    }
    dependentRuntimes_.push_back(dependentRuntime);
}

void DeviceRuntimeBase::detachDependentRuntime(IDeviceRuntime* dependentRuntime) {
    if (dependentRuntime == nullptr) {
        return;
    }
    const auto it = std::remove(dependentRuntimes_.begin(), dependentRuntimes_.end(), dependentRuntime);
    if (it != dependentRuntimes_.end()) {
        dependentRuntimes_.erase(it, dependentRuntimes_.end());
    }
}

const std::vector<IDeviceRuntime*>& DeviceRuntimeBase::dependentRuntimes() const {
    return dependentRuntimes_;
}

void DeviceRuntimeBase::requestReconfigure() {
    reconfigureRequested_ = true;
    disableRequested_ = false;
    status_ = DeviceStatus::Reconfiguring;
}

void DeviceRuntimeBase::requestDisable() {
    disableRequested_ = true;
    status_ = DeviceStatus::Disabled;
}

void DeviceRuntimeBase::requestDelete() {
    deleteRequested_ = true;
    status_ = DeviceStatus::Deleting;
}

void DeviceRuntimeBase::clearLifecycleRequests() {
    clearReconfigureRequested();
    clearDisableRequested();
    clearFaultRequested();
}

DeviceStatus DeviceRuntimeBase::status() const {
    return status_;
}

DeviceId DeviceRuntimeBase::deviceId() const {
    return deviceId_;
}

DeviceTypeId DeviceRuntimeBase::typeId() const {
    return typeId_;
}

uint32_t DeviceRuntimeBase::configVersion() const {
    return configVersion_;
}

uint32_t DeviceRuntimeBase::configRevision() const {
    return configRevision_;
}

bool DeviceRuntimeBase::hasDependencies() const {
    return dependencyCount_ > 0;
}

DeviceId DeviceRuntimeBase::dependencyDeviceId(DeviceRole role) const {
    for (uint8_t index = 0; index < dependencyCount_ && index < kMaxDeviceDependencies; ++index) {
        if (dependencyLinks_[index].role == role) {
            return dependencyLinks_[index].deviceId;
        }
    }
    return 0;
}

const DeviceBaseConfigV1& DeviceRuntimeBase::baseConfig() const {
    static const DeviceBaseConfigV1 empty{0U, {}};
    return empty;
}

bool DeviceRuntimeBase::enabled() const {
    return baseConfig().enabled != 0U;
}

const char* DeviceRuntimeBase::name() const {
    const char* value = baseConfig().name;
    return value[0] != '\0' ? value : nullptr;
}

DevicePersistencePolicy DeviceRuntimeBase::persistencePolicy() const {
    return persistencePolicy_;
}

bool DeviceRuntimeBase::handleCommand(const DeviceCommand& command) {
    (void)command;
    return false;
}

void DeviceRuntimeBase::tickRuntime(uint32_t now) {
    StateMachine::tick(now);
}

void DeviceRuntimeBase::setStatus(DeviceStatus status) {
    status_ = status;
}

bool DeviceRuntimeBase::dependencyReady(DeviceRole role) const {
    const IDeviceRuntime* dependency = dependencyRuntime(role);
    if (dependency == nullptr) {
        return false;
    }
    return dependency->status() == DeviceStatus::Ready;
}

bool DeviceRuntimeBase::dependenciesReady() const {
    for (uint8_t index = 0; index < dependencyCount_ && index < kMaxDeviceDependencies; ++index) {
        if (isGloballyOptionalDependencyRole(dependencyLinks_[index].role)) {
            continue;
        }
        const IDeviceRuntime* dependency = dependencyRuntimes_[index];
        if (dependency == nullptr || dependency->status() != DeviceStatus::Ready) {
            return false;
        }
    }
    return true;
}

bool DeviceRuntimeBase::hasDependentRuntime(const IDeviceRuntime* dependentRuntime) const {
    return std::find(dependentRuntimes_.begin(), dependentRuntimes_.end(), dependentRuntime) != dependentRuntimes_.end();
}

bool DeviceRuntimeBase::startRequested() const {
    return startRequested_;
}

bool DeviceRuntimeBase::reconfigureRequested() const {
    return reconfigureRequested_;
}

bool DeviceRuntimeBase::disableRequested() const {
    return disableRequested_;
}

bool DeviceRuntimeBase::deleteRequested() const {
    return deleteRequested_;
}

bool DeviceRuntimeBase::faultRequested() const {
    return faultRequested_;
}

void DeviceRuntimeBase::requestFault() {
    faultRequested_ = true;
}

void DeviceRuntimeBase::clearFaultRequested() {
    faultRequested_ = false;
}

void DeviceRuntimeBase::clearStartRequested() {
    startRequested_ = false;
}

void DeviceRuntimeBase::clearReconfigureRequested() {
    reconfigureRequested_ = false;
}

void DeviceRuntimeBase::clearDisableRequested() {
    disableRequested_ = false;
}

void DeviceRuntimeBase::clearDeleteRequested() {
    deleteRequested_ = false;
}

void DeviceRuntimeBase::setDeleted() {
    deleted_ = true;
}

void DeviceRuntimeBase::markRuntimeStateDirty() {
    runtimeStateDirty_ = true;
}

bool DeviceRuntimeBase::runtimeStateDirty() const {
    return runtimeStateDirty_;
}

void DeviceRuntimeBase::clearRuntimeStateDirty() {
    runtimeStateDirty_ = false;
}

} // namespace ewfm
