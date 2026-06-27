#include "devices/display/st7735/St7735Device.h"

#include "devices/bus/spi/SpiBusDevice.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kSt7735DeviceTypeId = 9;
constexpr uint32_t kSt7735DeviceConfigVersion = 1;
} // namespace

static_assert(std::is_trivially_copyable<St7735DeviceConfigV1>::value, "St7735DeviceConfigV1 must be POD");
static_assert(sizeof(St7735DeviceConfigV1::kMagic) - 1U + sizeof(St7735DeviceConfigV1) <= kMaxDeviceConfigBytes,
              "St7735DeviceConfigV1 exceeds device config bound");

St7735Device::St7735Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : DisplayDeviceBase(DisplayDeviceBase::initialState()) {
    bindDeviceIdentity(record, configBlob);
    (void)decodeSt7735DeviceConfig(configBlob.data(), configBlob.size(), config_);
}

const St7735DeviceConfigV1& St7735Device::config() const {
    return config_;
}

bool St7735Device::enabled() const {
    return config_.enabled != 0U;
}

const char* St7735Device::name() const {
    return config_.name;
}

bool St7735Device::spiChipSelectPin(uint8_t& pin) const {
    pin = config_.chipSelectPin;
    return true;
}

bool St7735Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = st7735DeviceConfigSize(config_);
    return encodeSt7735DeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool St7735Device::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    St7735DeviceConfigV1 config = config_;
    config.enabled = baseConfig.enabled;
    std::memcpy(config.name, baseConfig.name, sizeof(config.name));
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = st7735DeviceConfigSize(config);
    return encodeSt7735DeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan St7735Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    St7735DeviceConfigV1 config{};
    if (!decodeSt7735DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.spiBusDeviceId != config_.spiBusDeviceId || config.chipSelectPin != config_.chipSelectPin ||
                        config.layoutWidth != config_.layoutWidth || config.layoutHeight != config_.layoutHeight;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool St7735Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    St7735DeviceConfigV1 config{};
    if (!decodeSt7735DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

void St7735Device::writeDisplayConfigJson(JsonObject output) const {
    config_.writeJson(output);
}

DeviceTypeDescriptor St7735Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kSt7735DeviceTypeId;
    descriptor.name = "St7735Device";
    descriptor.currentConfigVersion = kSt7735DeviceConfigVersion;
    descriptor.maxDependents = 16;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        {DeviceDependencyRole::SpiBus, true, {SpiBusDevice::descriptor().typeId}},
    };
    descriptor.createRuntime = &St7735Device::createRuntime;
    descriptor.validateConfig = &St7735Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> St7735Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new St7735Device(record, configBlob));
}

DeviceValidationResult St7735Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "st7735 config exceeds supported size"};
    }
    St7735DeviceConfigV1 config{};
    if (!decodeSt7735DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "st7735 config is invalid"};
    }
    return {};
}

} // namespace ewfm
