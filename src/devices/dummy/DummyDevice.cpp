#include "devices/dummy/DummyDevice.h"

#include <algorithm>
#include <type_traits>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS DummyDevice

namespace {
constexpr DeviceTypeId kDummyDeviceTypeId = 1;
constexpr uint32_t kDummyDeviceConfigVersion = 2;

template <typename T>
void appendLE(std::string& out, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    const Unsigned v = static_cast<Unsigned>(value);
    for (size_t index = 0; index < sizeof(T); ++index) {
        out.push_back(static_cast<char>((v >> (index * 8)) & 0xFFU));
    }
}

template <typename T>
bool readLE(const std::string& blob, size_t& pos, T& value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    if (pos + sizeof(T) > blob.size()) {
        return false;
    }
    Unsigned v{0};
    for (size_t index = 0; index < sizeof(T); ++index) {
        v |= static_cast<Unsigned>(static_cast<unsigned char>(blob[pos + index])) << (index * 8);
    }
    value = static_cast<T>(v);
    pos += sizeof(T);
    return true;
}

bool decodeConfig(const std::string& blob, DummyDeviceConfigV2& config) {
    size_t pos = 0;
    uint32_t magic{0};
    uint8_t enabled{0};
    uint8_t restore{0};
    uint8_t defaultOutput{0};
    uint8_t currentOutput{0};
    uint8_t inverted{0};
    uint8_t reserved{0};
    uint8_t reserved2{0};
    uint8_t reserved3{0};
    if (!readLE(blob, pos, magic)) {
        return false;
    }

    if (magic == DummyDeviceConfigV1::magicKey) {
        if (!readLE(blob, pos, enabled) || !readLE(blob, pos, restore) || !readLE(blob, pos, defaultOutput) || !readLE(blob, pos, currentOutput)) {
            return false;
        }
        config.enabled = enabled != 0;
        config.restorePreviousState = restore != 0;
        config.defaultOutput = defaultOutput != 0;
        config.currentOutput = currentOutput != 0;
        config.inverted = false;
        return true;
    }

    if (magic != DummyDeviceConfigV2::magicKey) {
        return false;
    }

    if (!readLE(blob, pos, enabled) || !readLE(blob, pos, restore) || !readLE(blob, pos, defaultOutput) ||
        !readLE(blob, pos, currentOutput) || !readLE(blob, pos, inverted) || !readLE(blob, pos, reserved) ||
        !readLE(blob, pos, reserved2) || !readLE(blob, pos, reserved3)) {
        return false;
    }

    config.enabled = enabled != 0;
    config.restorePreviousState = restore != 0;
    config.defaultOutput = defaultOutput != 0;
    config.currentOutput = currentOutput != 0;
    config.inverted = inverted != 0;
    return true;
}

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
    (void)decodeConfig(record.configPayload, config_);
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
    if (!decodeConfig(record.configPayload, config)) {
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
