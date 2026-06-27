#include "devices/display/ssd1306/Ssd1306Device.h"

#include "devices/bus/i2c/I2cBusDevice.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kSsd1306DeviceTypeId = 7;
constexpr uint32_t kSsd1306DeviceConfigVersion = 1;
} // namespace

static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV1>::value, "Ssd1306DeviceConfigV1 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV1::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Ssd1306DeviceConfigV1 exceeds device config bound");

Ssd1306Device::Ssd1306Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : DisplayDeviceBase(DisplayDeviceBase::initialState()) {
    bindDeviceIdentity(record, configBlob);
    (void)decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config_);
}

const Ssd1306DeviceConfigV1& Ssd1306Device::config() const {
    return config_;
}

bool Ssd1306Device::enabled() const {
    return config_.enabled != 0U;
}

const char* Ssd1306Device::name() const {
    return config_.name;
}

bool Ssd1306Device::i2cAddress(uint8_t& address) const {
    address = config_.i2cAddress;
    return true;
}

bool Ssd1306Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ssd1306DeviceConfigSize(config_);
    return encodeSsd1306DeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool Ssd1306Device::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    Ssd1306DeviceConfigV1 config = config_;
    config.enabled = baseConfig.enabled;
    std::memcpy(config.name, baseConfig.name, sizeof(config.name));
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ssd1306DeviceConfigSize(config);
    return encodeSsd1306DeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Ssd1306Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Ssd1306DeviceConfigV1 config{};
    if (!decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.i2cBusDeviceId != config_.i2cBusDeviceId || config.i2cAddress != config_.i2cAddress ||
                        config.layoutWidth != config_.layoutWidth || config.layoutHeight != config_.layoutHeight;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Ssd1306Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Ssd1306DeviceConfigV1 config{};
    if (!decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

void Ssd1306Device::writeDisplayConfigJson(JsonObject output) const {
    config_.writeJson(output);
}

DeviceTypeDescriptor Ssd1306Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kSsd1306DeviceTypeId;
    descriptor.name = "Ssd1306Device";
    descriptor.currentConfigVersion = kSsd1306DeviceConfigVersion;
    descriptor.maxDependents = 16;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        {DeviceDependencyRole::I2CBus, true, {I2cBusDevice::descriptor().typeId}},
    };
    descriptor.createRuntime = &Ssd1306Device::createRuntime;
    descriptor.validateConfig = &Ssd1306Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Ssd1306Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Ssd1306Device(record, configBlob));
}

DeviceValidationResult Ssd1306Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "ssd1306 config exceeds supported size"};
    }
    Ssd1306DeviceConfigV1 config{};
    if (!decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "ssd1306 config is invalid"};
    }
    return {};
}

} // namespace ewfm
