#include "devices/dummy/DummyDevice.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS DummyDevice

namespace {
constexpr DeviceTypeId kDummyDeviceTypeId = 1;
constexpr uint32_t kDummyDeviceConfigVersion = 1;
} // namespace

static_assert(std::is_trivially_copyable<DummyDeviceConfigV1>::value, "DummyDeviceConfigV1 must be POD");
static_assert(sizeof(DummyDeviceConfigV1) == 34, "DummyDeviceConfigV1 layout changed");

bool encodeDummyDeviceConfig(const DummyDeviceConfigV1& config, uint8_t* blob, size_t capacity) {
    return encodeDeviceBaseConfig(config, blob, capacity);
}

bool decodeDummyDeviceConfig(const uint8_t* blob, size_t size, DummyDeviceConfigV1& config) {
    return decodeDeviceBaseConfig(blob, size, config);
}

bool parseDummyDeviceConfigJson(const JsonObjectConst& input, uint32_t configVersion, DummyDeviceConfigV1& config, const char*& error) {
    if (configVersion == 0U) {
        configVersion = DummyDevice::descriptor().currentConfigVersion;
    }

    if (configVersion != 1U) {
        error = "unsupported DummyDevice config version";
        return false;
    }
    return true;
}

void writeDummyDeviceConfigJson(const DummyDeviceConfigV1& config, JsonObject output) {
    writeDeviceBaseConfigJson(config, output);
}

DummyDevice::DummyDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : DeviceRuntimeBase((PState)&DummyDevice::Idle) {
    bindDeviceIdentity(record, configBlob);
    (void)decodeDummyDeviceConfig(configBlob.data(), configBlob.size(), config_);
    restorePreviousState_ = false;
    currentOutput_ = false;
}

bool DummyDevice::handleCommand(const DeviceCommand& command) {
    if (command.type == DeviceCommandType::SetStatus) {
        if (command.payload.equals("fault")) {
            faultRequested_ = true;
            return true;
        }
        if (command.payload.equals("ready")) {
            faultRequested_ = false;
            reconfigureRequested_ = false;
            disableRequested_ = false;
            return true;
        }
    }

    if (command.type == DeviceCommandType::SetOutput) {
        if (command.payload.equals("on")) {
            currentOutput_ = true;
            return true;
        }
        if (command.payload.equals("off") || command.payload.equals("disabled")) {
            currentOutput_ = false;
            return true;
        }
    }

    if (command.type == DeviceCommandType::Custom) {
        if (command.payload.equals("output=1")) {
            currentOutput_ = true;
            return true;
        }
        if (command.payload.equals("output=0")) {
            currentOutput_ = false;
            return true;
        }
    }

    return false;
}

void DummyDevice::applyRetainedState(bool output) {
    retainedStateAvailable_ = true;
    retainedOutput_ = output;
    if (restorePreviousState_ && is((PState)&DummyDevice::Ready)) {
        currentOutput_ = retainedOutput_;
    }
}

bool DummyDevice::outputState() const {
    return currentOutput_;
}

bool DummyDevice::restorePreviousState() const {
    return restorePreviousState_;
}

const DummyDeviceConfigV1& DummyDevice::config() const {
    return config_;
}

bool DummyDevice::deleted() const {
    return deleted_;
}

bool DummyDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dummyDeviceConfigSize(config_);
    return encodeDummyDeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool DummyDevice::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dummyDeviceConfigSize(baseConfig);
    return encodeDummyDeviceConfig(baseConfig, buffer, size) && configBlob.assign(buffer, size);
}

void DummyDevice::writeDeviceJson(JsonObject output) const {
    output["name"] = config_.name;
    output["enabled"] = config_.enabled != 0U;
    JsonObject configObject = output.createNestedObject("config");
    writeDeviceBaseConfigJson(config_, configObject);
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

std::unique_ptr<IDeviceRuntime> DummyDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new DummyDevice(record, configBlob));
}

DeviceValidationResult DummyDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "dummy device config exceeds supported size"};
    }
    DummyDeviceConfigV1 config{};
    if (!decodeDummyDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "dummy device config is invalid"};
    }
    return {};
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

    if (restorePreviousState_ && retainedStateAvailable_) {
        currentOutput_ = retainedOutput_;
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
