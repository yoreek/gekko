#include "devices/display/lcd1602/Lcd1602Device.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kLcd1602DeviceTypeId = 28;
constexpr uint32_t kLcd1602DeviceConfigVersion = 1;
constexpr uint8_t kLcd1602Columns = 16U;
constexpr uint8_t kLcd1602Rows = 2U;
} // namespace

static_assert(std::is_trivially_copyable<Lcd1602DeviceConfigV1>::value, "Lcd1602DeviceConfigV1 must be POD");

Lcd1602Device::Lcd1602Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Lcd1602Device([&configBlob]() {
          Lcd1602DeviceConfigV1 config{};
          (void)decodeLcd1602DeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Lcd1602Device::Lcd1602Device(const Lcd1602DeviceConfigV1& config)
    : Hd44780CharacterDisplayDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd1602Columns, kLcd1602Rows), config_(config) {
}

const Lcd1602DeviceConfigV1& Lcd1602Device::config() const {
    return config_;
}

const DeviceBaseConfigV1& Lcd1602Device::baseConfig() const {
    return config_;
}

const Hd44780ChannelConfigV1& Lcd1602Device::channelConfig() const {
    return config_.channels;
}

const char* Lcd1602Device::lineTemplate(uint8_t row) const {
    return row == 0U ? config_.line1 : config_.line2;
}

bool Lcd1602Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = lcd1602DeviceConfigSize(config_);
    return encodeFixedConfigBlob(Lcd1602DeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Lcd1602Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Lcd1602DeviceConfigV1 config{};
    if (!decodeLcd1602DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig =
        config.channels.rsChannel != config_.channels.rsChannel || config.channels.eChannel != config_.channels.eChannel ||
        config.channels.d4Channel != config_.channels.d4Channel || config.channels.d5Channel != config_.channels.d5Channel ||
        config.channels.d6Channel != config_.channels.d6Channel || config.channels.d7Channel != config_.channels.d7Channel ||
        config.channels.backlightChannel != config_.channels.backlightChannel;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Lcd1602Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Lcd1602DeviceConfigV1 config{};
    if (!decodeLcd1602DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    resetRenderedLines();
    return true;
}

DeviceTypeDescriptor Lcd1602Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kLcd1602DeviceTypeId;
    descriptor.name = "Lcd1602Device";
    descriptor.currentConfigVersion = kLcd1602DeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        {DeviceRole::PortExpander, true},
        {DeviceRole::MetricSource, false},
    };
    descriptor.createRuntime = &Lcd1602Device::createRuntime;
    descriptor.validateConfig = &Lcd1602Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Lcd1602Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Lcd1602Device(record, configBlob));
}

DeviceValidationResult Lcd1602Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::PortExpander) == 0U) {
        return {DeviceError::InvalidRelationship, "lcd1602 requires a port expander dependency"};
    }
    Lcd1602DeviceConfigV1 config{};
    if (!decodeLcd1602DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::CorruptRecord, "lcd1602 config is invalid"};
    }
    return config.validate();
}

} // namespace ewfm
