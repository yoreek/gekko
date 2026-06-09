#include "devices/dummy/DummyDevice.h"

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS DummyDevice

namespace {
constexpr DeviceTypeId kDummyDeviceTypeId = 1;
constexpr uint32_t kDummyDeviceConfigVersion = 2;

} // namespace

static_assert(std::is_trivially_copyable<DummyDeviceConfigV1>::value, "DummyDeviceConfigV1 must be POD");
static_assert(std::is_trivially_copyable<DummyDeviceConfigV2>::value, "DummyDeviceConfigV2 must be POD");
static_assert(sizeof(DummyDeviceConfigV1) == 4, "DummyDeviceConfigV1 layout changed");
static_assert(sizeof(DummyDeviceConfigV2) == 5, "DummyDeviceConfigV2 layout changed");

void DummyDeviceConfigV2::migrateFrom(const DummyDeviceConfigV1& orig) {
    enabled = orig.enabled;
    restorePreviousState = orig.restorePreviousState;
    defaultOutput = orig.defaultOutput;
    currentOutput = orig.currentOutput;
    inverted = false;
}

void DummyDeviceConfigV2::migrateFrom(const DummyDeviceConfigV2& orig) {
    enabled = orig.enabled;
    restorePreviousState = orig.restorePreviousState;
    defaultOutput = orig.defaultOutput;
    currentOutput = orig.currentOutput;
    inverted = orig.inverted;
}

namespace {
template <typename T> std::string encodeDeviceConfigBlob(uint32_t magicKey, const T& config) {
    std::string blob;
    blob.resize(sizeof(magicKey) + sizeof(T));
    std::memcpy(blob.data(), &magicKey, sizeof(magicKey));
    std::memcpy(blob.data() + sizeof(magicKey), &config, sizeof(T));
    return blob;
}

template <typename T> bool decodeDeviceConfigBlob(const std::string& blob, uint32_t expectedMagicKey, T& config, size_t legacySize = 0) {
    const size_t compactSize = sizeof(expectedMagicKey) + sizeof(T);
    if (blob.size() != compactSize && blob.size() != legacySize) {
        return false;
    }

    uint32_t magicKey{0};
    std::memcpy(&magicKey, blob.data(), sizeof(magicKey));
    if (magicKey != expectedMagicKey) {
        return false;
    }

    std::memcpy(&config, blob.data() + sizeof(magicKey), sizeof(T));
    return true;
}
} // namespace

std::string encodeDummyDeviceConfig(const DummyDeviceConfigV1& config) {
    return encodeDeviceConfigBlob(DummyDeviceConfigV1::kMagicKey, config);
}

std::string encodeDummyDeviceConfig(const DummyDeviceConfigV2& config) {
    return encodeDeviceConfigBlob(DummyDeviceConfigV2::kMagicKey, config);
}

bool decodeDummyDeviceConfig(const std::string& blob, DummyDeviceConfigV2& config) {
    if (blob.size() < sizeof(uint32_t)) {
        return false;
    }

    uint32_t magicKey{0};
    std::memcpy(&magicKey, blob.data(), sizeof(magicKey));
    if (magicKey == DummyDeviceConfigV1::kMagicKey) {
        DummyDeviceConfigV1 legacy{};
        if (!decodeDeviceConfigBlob(blob, DummyDeviceConfigV1::kMagicKey, legacy)) {
            return false;
        }
        config.migrateFrom(legacy);
        return true;
    }

    if (magicKey == DummyDeviceConfigV2::kMagicKey) {
        return decodeDeviceConfigBlob(blob, DummyDeviceConfigV2::kMagicKey, config, sizeof(uint32_t) + sizeof(DummyDeviceConfigV2) + 3U);
    }

    return false;
}

bool parseDummyDeviceConfigJson(const JsonObjectConst& input, uint32_t configVersion, DummyDeviceConfigV2& config, std::string& error) {
    if (configVersion == 0U) {
        configVersion = DummyDevice::descriptor().currentConfigVersion;
    }

    if (configVersion != 1U && configVersion != 2U) {
        error = "unsupported DummyDevice config version";
        return false;
    }

    config.enabled = (input["enabled"] | true) ? 1U : 0U;
    config.restorePreviousState = (input["restore_previous_state"] | false) ? 1U : 0U;
    config.defaultOutput = (input["default_output"] | false) ? 1U : 0U;
    config.currentOutput = (input["current_output"] | (config.defaultOutput != 0U)) ? 1U : 0U;
    config.inverted = (input["inverted"] | false) ? 1U : 0U;
    if (configVersion == 1U) {
        config.inverted = false;
    }
    return true;
}

void writeDummyDeviceConfigJson(const DummyDeviceConfigV2& config, JsonObject output) {
    output["enabled"] = config.enabled != 0U;
    output["restore_previous_state"] = config.restorePreviousState != 0U;
    output["default_output"] = config.defaultOutput != 0U;
    output["current_output"] = config.currentOutput != 0U;
    output["inverted"] = config.inverted != 0U;
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

void DummyDevice::setParentRuntime(IDeviceRuntime* parentRuntime) {
    parentRuntime_ = parentRuntime;
}

IDeviceRuntime* DummyDevice::parentRuntime() const {
    return parentRuntime_;
}

void DummyDevice::attachChildRuntime(IDeviceRuntime* childRuntime) {
    if (childRuntime == nullptr || hasChildRuntime(childRuntime)) {
        return;
    }
    childRuntimes_.push_back(childRuntime);
}

void DummyDevice::detachChildRuntime(IDeviceRuntime* childRuntime) {
    if (childRuntime == nullptr) {
        return;
    }
    const auto it = std::remove(childRuntimes_.begin(), childRuntimes_.end(), childRuntime);
    if (it != childRuntimes_.end()) {
        childRuntimes_.erase(it, childRuntimes_.end());
    }
}

const std::vector<IDeviceRuntime*>& DummyDevice::childRuntimes() const {
    return childRuntimes_;
}

void DummyDevice::requestReconfigure() {
    reconfigureRequested_ = true;
    disableRequested_ = false;
    status_ = DeviceStatus::Reconfiguring;
}

void DummyDevice::requestDisable() {
    disableRequested_ = true;
    status_ = DeviceStatus::Disabled;
}

void DummyDevice::requestDelete() {
    deleteRequested_ = true;
    status_ = DeviceStatus::Deleting;
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
            disableRequested_ = false;
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

bool DummyDevice::parentReady() const {
    if (parentRuntime_ == nullptr) {
        return true;
    }
    return parentRuntime_->status() == DeviceStatus::Ready;
}

bool DummyDevice::hasChildRuntime(const IDeviceRuntime* childRuntime) const {
    return std::find(childRuntimes_.begin(), childRuntimes_.end(), childRuntime) != childRuntimes_.end();
}

SM_STATE(DummyDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(DummyDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (!parentReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
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
    if (!parentReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
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

SM_STATE(DummyDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        deleted_ = true;
        SM_GOTO(Deleting);
    }
    if (disableRequested_) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_ || startRequested_) {
        if (parentReady()) {
            status_ = DeviceStatus::Reconfiguring;
            SM_GOTO(Reconfiguring);
        }
    }
}

SM_STATE(DummyDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    if (!parentReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
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
