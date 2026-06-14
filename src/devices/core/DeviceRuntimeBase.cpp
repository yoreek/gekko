#include "devices/core/DeviceRuntimeBase.h"

#include <algorithm>

namespace ewfm {

DeviceRuntimeBase::DeviceRuntimeBase(PState initialState) : StateMachine(initialState) {}

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

} // namespace ewfm
