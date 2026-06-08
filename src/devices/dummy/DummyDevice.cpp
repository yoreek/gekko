#include "devices/dummy/DummyDevice.h"

#include "devices/dummy/DummyDeviceConfigCodec.h"

#include <algorithm>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS DummyDevice

namespace {
constexpr DeviceTypeId kDummyDeviceTypeId = 1;
constexpr uint32_t kDummyDeviceConfigVersion = 2;

} // namespace

void DummyDeviceConfigV2::migrateFrom(const DummyDeviceConfigV1& orig) {
    enabled = orig.enabled;
    restorePreviousState = orig.restorePreviousState;
    defaultOutput = orig.defaultOutput;
    currentOutput = orig.currentOutput;
    inverted = false;
}

DummyDevice::DummyDevice(const DeviceRecord& record) : StateMachine((PState)&DummyDevice::Idle) {
    config_.enabled = record.enabled;
    config_.currentOutput = false;
    (void)decodeDummyDeviceConfig(record.configPayload, config_);
    config_.enabled = record.enabled;
    if (config_.inverted) {
        config_.currentOutput = !config_.currentOutput;
    }
}

void DummyDevice::begin(uint32_t now) {
    startRequested_ = true;
    StateMachine::tick(now);
}

void DummyDevice::tickFastLoop(uint32_t now) {
    tickCadence(now);
}

void DummyDevice::tick100ms(uint32_t now) {
    tickCadence(now);
}

void DummyDevice::tick1s(uint32_t now) {
    tickCadence(now);
}

void DummyDevice::requestReconfigure() {
    reconfigureRequested_ = true;
}

void DummyDevice::requestDisable() {
    disableRequested_ = true;
}

void DummyDevice::requestDelete() {
    deleteRequested_ = true;
}

DeviceStatus DummyDevice::status() const {
    return status_;
}

bool DummyDevice::handleCommand(const DeviceCommand& command) {
    if (command.type == DeviceCommandType::SetStatus) {
        if (command.payload == "fault") {
            faultRequested_ = true;
            return true;
        }
        if (command.payload == "ready") {
            faultRequested_ = false;
            reconfigureRequested_ = false;
            return true;
        }
    }

    if (command.type == DeviceCommandType::Custom) {
        if (command.payload == "output=1") {
            config_.currentOutput = true;
            return true;
        }
        if (command.payload == "output=0") {
            config_.currentOutput = false;
            return true;
        }
    }

    return false;
}

void DummyDevice::applyRetainedState(bool output) {
    retainedStateAvailable_ = true;
    retainedOutput_ = output;
    if (config_.restorePreviousState && is((PState)&DummyDevice::Ready)) {
        config_.currentOutput = retainedOutput_;
    }
}

bool DummyDevice::outputState() const {
    return config_.currentOutput;
}

bool DummyDevice::restorePreviousState() const {
    return config_.restorePreviousState;
}

const DummyDeviceConfigV2& DummyDevice::config() const {
    return config_;
}

bool DummyDevice::deleted() const {
    return deleted_;
}

DeviceTypeDescriptor DummyDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kDummyDeviceTypeId;
    descriptor.name = "DummyDevice";
    descriptor.currentConfigVersion = kDummyDeviceConfigVersion;
    descriptor.canHaveChildren = true;
    descriptor.maxChildren = 16;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.ticks100ms = true;
    descriptor.ticks1s = true;
    descriptor.createRuntime = &DummyDevice::createRuntime;
    descriptor.validateConfig = &DummyDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> DummyDevice::createRuntime(const DeviceRecord& record) {
    return std::unique_ptr<IDeviceRuntime>(new DummyDevice(record));
}

DeviceValidationResult DummyDevice::validateConfig(const DeviceRecord& record) {
    if (record.configPayload.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "dummy device config exceeds supported size"};
    }
    DummyDeviceConfigV2 config;
    if (!decodeDummyDeviceConfig(record.configPayload, config)) {
        return {DeviceError::InvalidConfig, "dummy device config is invalid"};
    }
    return {};
}

void DummyDevice::tickCadence(uint32_t now) {
    StateMachine::tick(now);
}

SM_STATE(DummyDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(DummyDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        deleted_ = true;
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.enabled) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }

    if (config_.restorePreviousState && retainedStateAvailable_) {
        config_.currentOutput = retainedOutput_;
    }

    startRequested_ = false;
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(DummyDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        deleted_ = true;
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.enabled) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (faultRequested_) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
}

SM_STATE(DummyDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    status_ = DeviceStatus::Starting;
    SM_GOTO(Starting);
}

SM_STATE(DummyDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        deleted_ = true;
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(DummyDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        deleted_ = true;
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        faultRequested_ = false;
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(DummyDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    deleted_ = true;
}

} // namespace ewfm
