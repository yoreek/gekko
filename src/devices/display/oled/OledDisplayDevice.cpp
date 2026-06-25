#include "devices/display/oled/OledDisplayDevice.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS OledDisplayDevice

namespace {
constexpr DeviceTypeId kOledDisplayDeviceTypeId = 7;
constexpr uint32_t kOledDisplayDeviceConfigVersion = 1;
} // namespace

static_assert(std::is_trivially_copyable<OledDisplayDeviceConfigV1>::value, "OledDisplayDeviceConfigV1 must be POD");
static_assert(sizeof(OledDisplayDeviceConfigV1::kMagic) - 1U + sizeof(OledDisplayDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "OledDisplayDeviceConfigV1 exceeds device config bound");

OledDisplayDevice::OledDisplayDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : DeviceRuntimeBase((PState)&OledDisplayDevice::Idle) {
    bindDeviceIdentity(record, configBlob);
    (void)decodeOledDisplayDeviceConfig(configBlob.data(), configBlob.size(), config_);
}

const OledDisplayDeviceConfigV1& OledDisplayDevice::config() const {
    return config_;
}

bool OledDisplayDevice::enabled() const {
    return config_.enabled != 0U;
}

const char* OledDisplayDevice::name() const {
    return config_.name;
}

bool OledDisplayDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oledDisplayDeviceConfigSize(config_);
    return encodeOledDisplayDeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool OledDisplayDevice::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    OledDisplayDeviceConfigV1 config = config_;
    config.enabled = baseConfig.enabled;
    std::memcpy(config.name, baseConfig.name, sizeof(config.name));
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oledDisplayDeviceConfigSize(config);
    return encodeOledDisplayDeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan OledDisplayDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    OledDisplayDeviceConfigV1 config{};
    if (!decodeOledDisplayDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.i2cBusDeviceId != config_.i2cBusDeviceId || config.i2cAddress != config_.i2cAddress ||
                        config.layoutWidth != config_.layoutWidth || config.layoutHeight != config_.layoutHeight;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool OledDisplayDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    OledDisplayDeviceConfigV1 config{};
    if (!decodeOledDisplayDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

DeviceValidationResult OledDisplayDevice::loadPersistedState(DeviceScopedDataStore& store) {
    OledDisplayLayoutStore layoutStore(store);
    OledDisplayLayoutRecordV1 layout{};
    const DeviceValidationResult result = layoutStore.load(deviceId(), layout);
    if (!result.ok()) {
        if (result.error == DeviceError::MissingRecord) {
            layout_ = {};
        }
        return result;
    }
    layout_ = layout;
    return {};
}

DeviceValidationResult OledDisplayDevice::savePersistedState(DeviceScopedDataStore& store) const {
    OledDisplayLayoutStore layoutStore(store);
    if (layout_.pages.empty()) {
        (void)layoutStore.remove(deviceId());
        return {};
    }
    return layoutStore.save(layout_);
}

DeviceValidationResult OledDisplayDevice::clearPersistedState(DeviceScopedDataStore& store) {
    OledDisplayLayoutStore layoutStore(store);
    layout_ = {};
    (void)layoutStore.clearDevice(deviceId());
    return {};
}

DeviceValidationResult OledDisplayDevice::applyPersistedStateUpdate(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0U) {
        layout_ = {};
        return {};
    }

    OledDisplayLayoutRecordV1 layout{};
    if (!decodeOledDisplayLayoutBinary(data, size, layout)) {
        return {DeviceError::InvalidConfig, "oled display layout is invalid"};
    }
    if (layout.deviceId != 0U && layout.deviceId != deviceId()) {
        return {DeviceError::InvalidDeviceId, "device scoped data device id is invalid"};
    }
    layout.deviceId = deviceId();
    layout_ = layout;
    return {};
}

void OledDisplayDevice::setLayout(const OledDisplayLayoutRecordV1& layout) {
    layout_ = layout;
}

void OledDisplayDevice::writeDeviceJson(JsonObject output) const {
    writeCommonDeviceJson(output);
    config_.writeJson(output);
}

const OledDisplayLayoutRecordV1& OledDisplayDevice::layout() const {
    return layout_;
}

DeviceTypeDescriptor OledDisplayDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kOledDisplayDeviceTypeId;
    descriptor.name = "OledDisplayDevice";
    descriptor.currentConfigVersion = kOledDisplayDeviceConfigVersion;
    descriptor.maxDependents = 16;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.createRuntime = &OledDisplayDevice::createRuntime;
    descriptor.validateConfig = &OledDisplayDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> OledDisplayDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new OledDisplayDevice(record, configBlob));
}

DeviceValidationResult OledDisplayDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "oled display config exceeds supported size"};
    }
    OledDisplayDeviceConfigV1 config{};
    if (!decodeOledDisplayDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "oled display config is invalid"};
    }
    return {};
}

SM_STATE(OledDisplayDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(OledDisplayDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(Disabled);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
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
    startRequested_ = false;
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(OledDisplayDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
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
}

SM_STATE(OledDisplayDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    status_ = DeviceStatus::Starting;
    SM_GOTO(Starting);
}

SM_STATE(OledDisplayDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(OledDisplayDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(OledDisplayDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
