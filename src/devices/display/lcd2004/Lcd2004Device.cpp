#include "devices/display/lcd2004/Lcd2004Device.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kLcd2004DeviceTypeId = 30; // 29 is taken by kAht10SensorTypeId.
constexpr uint32_t kLcd2004DeviceConfigVersion = 1;
constexpr uint8_t kLcd2004Columns = 20U;
constexpr uint8_t kLcd2004Rows = 4U;
} // namespace

static_assert(std::is_trivially_copyable<Lcd2004DeviceConfigV1>::value, "Lcd2004DeviceConfigV1 must be POD");

Lcd2004Device::Lcd2004Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Lcd2004Device([&configBlob]() {
          Lcd2004DeviceConfigV1 config{};
          (void)decodeLcd2004DeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Lcd2004Device::Lcd2004Device(const Lcd2004DeviceConfigV1& config)
    : Hd44780CharacterDisplayDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd2004Columns, kLcd2004Rows), config_(config) {
}

const Lcd2004DeviceConfigV1& Lcd2004Device::config() const {
    return config_;
}

const DeviceBaseConfigV1& Lcd2004Device::baseConfig() const {
    return config_;
}

const Hd44780ChannelConfigV1& Lcd2004Device::channelConfig() const {
    return config_.channels;
}

const char* Lcd2004Device::lineTemplate(uint8_t row) const {
    switch (row) {
    case 0U:
        return config_.line1;
    case 1U:
        return config_.line2;
    case 2U:
        return config_.line3;
    default:
        return config_.line4;
    }
}

bool Lcd2004Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = lcd2004DeviceConfigSize(config_);
    return encodeFixedConfigBlob(Lcd2004DeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Lcd2004Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Lcd2004DeviceConfigV1 config{};
    if (!decodeLcd2004DeviceConfig(configBlob.data(), configBlob.size(), config)) {
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

bool Lcd2004Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Lcd2004DeviceConfigV1 config{};
    if (!decodeLcd2004DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    resetRenderedLines();
    return true;
}

DeviceTypeDescriptor Lcd2004Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kLcd2004DeviceTypeId;
    descriptor.name = "Lcd2004Device";
    descriptor.currentConfigVersion = kLcd2004DeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        {DeviceRole::PortExpander, true},
        {DeviceRole::MetricSource, false},
    };
    descriptor.createRuntime = &Lcd2004Device::createRuntime;
    descriptor.validateConfig = &Lcd2004Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Lcd2004Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Lcd2004Device(record, configBlob));
}

DeviceValidationResult Lcd2004Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::PortExpander) == 0U) {
        return {DeviceError::InvalidRelationship, "lcd2004 requires a port expander dependency"};
    }
    Lcd2004DeviceConfigV1 config{};
    if (!decodeLcd2004DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::CorruptRecord, "lcd2004 config is invalid"};
    }
    return config.validate();
}

} // namespace ewfm
