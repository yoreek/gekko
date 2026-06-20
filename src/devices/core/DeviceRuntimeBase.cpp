#include "devices/core/DeviceRuntimeBase.h"

#include "devices/core/DeviceBaseConfig.h"

#include <algorithm>
#include <cstring>

namespace ewfm {

DeviceRuntimeBase::DeviceRuntimeBase(PState initialState) : StateMachine(initialState) {}

void DeviceRuntimeBase::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceBaseConfigV1 base{};
    if (readDeviceBaseConfig(config, base)) {
        enabled_ = base.enabled != 0U;
        std::memcpy(name_, base.name, sizeof(name_));
        name_[sizeof(name_) - 1U] = '\0';
    }
    deviceId_ = record.header.deviceId;
    typeId_ = record.header.typeId;
    configVersion_ = record.header.configVersion;
    configRevision_ = record.header.configRevision;
    hasParent_ = record.hasParent;
    parentDeviceId_ = record.parentDeviceId;
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

void DeviceRuntimeBase::setParentRuntime(IDeviceRuntime* parentRuntime) {
    parentRuntime_ = parentRuntime;
}

IDeviceRuntime* DeviceRuntimeBase::parentRuntime() const {
    return parentRuntime_;
}

void DeviceRuntimeBase::attachChildRuntime(IDeviceRuntime* childRuntime) {
    if (childRuntime == nullptr || hasChildRuntime(childRuntime)) {
        return;
    }
    childRuntimes_.push_back(childRuntime);
}

void DeviceRuntimeBase::detachChildRuntime(IDeviceRuntime* childRuntime) {
    if (childRuntime == nullptr) {
        return;
    }
    const auto it = std::remove(childRuntimes_.begin(), childRuntimes_.end(), childRuntime);
    if (it != childRuntimes_.end()) {
        childRuntimes_.erase(it, childRuntimes_.end());
    }
}

const std::vector<IDeviceRuntime*>& DeviceRuntimeBase::childRuntimes() const {
    return childRuntimes_;
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

bool DeviceRuntimeBase::hasParent() const {
    return hasParent_;
}

DeviceId DeviceRuntimeBase::parentDeviceId() const {
    return parentDeviceId_;
}

bool DeviceRuntimeBase::enabled() const {
    return enabled_;
}

const char* DeviceRuntimeBase::name() const {
    return name_;
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

bool DeviceRuntimeBase::parentReady() const {
    if (parentRuntime_ == nullptr) {
        return true;
    }
    return parentRuntime_->status() == DeviceStatus::Ready;
}

bool DeviceRuntimeBase::hasChildRuntime(const IDeviceRuntime* childRuntime) const {
    return std::find(childRuntimes_.begin(), childRuntimes_.end(), childRuntime) != childRuntimes_.end();
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
