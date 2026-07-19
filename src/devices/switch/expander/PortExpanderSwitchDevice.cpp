#include "devices/switch/expander/PortExpanderSwitchDevice.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kPortExpanderSwitchDeviceTypeId = 14;
constexpr uint32_t kPortExpanderSwitchDeviceConfigVersion = 3;

} // namespace

static_assert(std::is_trivially_copyable<PortExpanderSwitchDeviceConfigV1>::value, "PortExpanderSwitchDeviceConfigV1 must be POD");
static_assert(std::is_trivially_copyable<PortExpanderSwitchDevicePersistedConfigV1>::value,
              "PortExpanderSwitchDevicePersistedConfigV1 must be POD");
static_assert(std::is_trivially_copyable<PortExpanderSwitchDeviceConfigV2>::value, "PortExpanderSwitchDeviceConfigV2 must be POD");
static_assert(std::is_trivially_copyable<PortExpanderSwitchDeviceConfigV3>::value, "PortExpanderSwitchDeviceConfigV3 must be POD");
static_assert(sizeof(SwitchDeviceConfigV1) == 38, "SwitchDeviceConfigV1 layout changed");
static_assert(sizeof(PortExpanderSwitchDeviceConfigV1) == 1, "PortExpanderSwitchDeviceConfigV1 layout changed");
static_assert(sizeof(PortExpanderSwitchDeviceConfigV2) == 39, "PortExpanderSwitchDeviceConfigV2 layout changed");
static_assert(sizeof(PortExpanderSwitchDeviceConfigV3) == 39, "PortExpanderSwitchDeviceConfigV3 layout changed");
static_assert(sizeof(PortExpanderSwitchDeviceConfigV3::kMagic) - 1U + sizeof(PortExpanderSwitchDeviceConfigV3) <= kMaxDeviceConfigBytes,
              "PortExpanderSwitchDeviceConfigV3 exceeds device config bound");

bool decodePortExpanderSwitchDeviceConfig(const uint8_t* blob, size_t size, PortExpanderSwitchDeviceConfigV3& config) {
    if (decodeValidatedFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, blob, size, config)) {
        return true;
    }

    PortExpanderSwitchDeviceConfigV2 legacyV2{};
    if (decodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV2::kMagic, blob, size, legacyV2)) {
        if (!legacyV2.SwitchDeviceConfigV1::validate().ok() || !portExpanderSwitchChannelIsValid(legacyV2.channel)) {
            return false;
        }
        config.migrateFrom(legacyV2);
        return config.validate().ok();
    }

    PortExpanderSwitchDevicePersistedConfigV1 legacy{};
    size_t pos = 0;
    if (!readFixedConfigSegment(SwitchDeviceConfigV1::kMagic, blob, size, pos, legacy.switchConfig) ||
        !readFixedConfigSegment(PortExpanderSwitchDeviceConfigV1::kMagic, blob, size, pos, legacy.expanderConfig) || pos != size) {
        return false;
    }
    if (!legacy.switchConfig.validate().ok() || !portExpanderSwitchChannelIsValid(legacy.expanderConfig.channel)) {
        return false;
    }
    legacyV2 = {};
    static_cast<SwitchDeviceConfigV1&>(legacyV2) = legacy.switchConfig;
    legacyV2.channel = legacy.expanderConfig.channel;
    config.migrateFrom(legacyV2);
    return config.validate().ok();
}

bool PortExpanderSwitchDeviceConfigV3::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!SwitchDeviceConfigV2::parseJson(input, error)) {
        return false;
    }

    channel = static_cast<uint8_t>(input["channel"] | static_cast<int>(channel));
    if (!portExpanderSwitchChannelIsValid(channel)) {
        error = "port expander switch channel is invalid";
        return false;
    }
    return true;
}

DeviceValidationResult PortExpanderSwitchDeviceConfigV3::validate() const {
    const DeviceValidationResult switchResult = SwitchDeviceConfigV2::validate();
    if (!switchResult.ok()) {
        return switchResult;
    }
    return portExpanderSwitchChannelIsValid(channel)
               ? DeviceValidationResult{}
               : DeviceValidationResult{DeviceError::InvalidConfig, "port expander switch channel is invalid"};
}

void PortExpanderSwitchDeviceConfigV3::writeJson(JsonObject output) const {
    SwitchDeviceConfigV2::writeJson(output);
    output["channel"] = channel;
}

void PortExpanderSwitchDeviceConfigV3::migrateFrom(const PortExpanderSwitchDeviceConfigV2& legacy) {
    SwitchDeviceConfigV2::migrateFrom(legacy);
    channel = legacy.channel;
}

bool portExpanderSwitchChannelIsValid(uint8_t channel) {
    return channel <= kMaxPortExpanderChannel;
}

PortExpanderSwitchDevice::PortExpanderSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : PortExpanderSwitchDevice([&configBlob]() {
          PortExpanderSwitchDeviceConfigV3 config{};
          (void)decodePortExpanderSwitchDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

PortExpanderSwitchDevice::PortExpanderSwitchDevice(const PortExpanderSwitchDeviceConfigV3& config)
    : SwitchDeviceBase(config), config_(config) {}

uint8_t PortExpanderSwitchDevice::channel() const {
    return config_.channel;
}

const PortExpanderSwitchDeviceConfigV3& PortExpanderSwitchDevice::config() const {
    return config_;
}

bool PortExpanderSwitchDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = portExpanderSwitchDeviceConfigSize(config_);
    return encodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan PortExpanderSwitchDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    PortExpanderSwitchDeviceConfigV3 config{};
    if (!decodePortExpanderSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool invertedChanged = config.inverted != config_.inverted;
    const bool channelChanged = config.channel != config_.channel;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = channelChanged || invertedChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool PortExpanderSwitchDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    PortExpanderSwitchDeviceConfigV3 config{};
    if (!decodePortExpanderSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

SwitchDeviceConfigV2& PortExpanderSwitchDevice::mutableConfig() {
    return config_;
}

bool PortExpanderSwitchDevice::expanderChannel(uint8_t& channel) const {
    channel = config_.channel;
    return true;
}
DeviceTypeDescriptor PortExpanderSwitchDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kPortExpanderSwitchDeviceTypeId;
    descriptor.name = "PortExpanderSwitchDevice";
    descriptor.currentConfigVersion = kPortExpanderSwitchDeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::PortExpander, true}};
    descriptor.providedRoles = ProvidedRoles::of({ISwitchOutputRuntime::kProvidedRole, IStatusRuntime::kProvidedRole});
    descriptor.createRuntime = &PortExpanderSwitchDevice::createRuntime;
    descriptor.validateConfig = &PortExpanderSwitchDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> PortExpanderSwitchDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                        const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new PortExpanderSwitchDevice(record, configBlob));
}

DeviceValidationResult PortExpanderSwitchDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::PortExpander) == 0U) {
        return {DeviceError::InvalidRelationship, "port expander switch requires a port expander dependency"};
    }
    return validateOutputConfig<PortExpanderSwitchDeviceConfigV3>(configBlob, decodePortExpanderSwitchDeviceConfig);
}

DeviceValidationResult PortExpanderSwitchDevice::configureHardware(uint32_t now) {
    (void)now;
    const IPortExpanderRuntime* expander = dependencyExpander();
    if (expander == nullptr) {
        return {DeviceError::InvalidRelationship, "port expander switch dependency is not ready"};
    }
    if (config_.channel >= expander->channelCount()) {
        return {DeviceError::InvalidConfig, "port expander switch channel is out of range"};
    }
    return {};
}

DeviceValidationResult PortExpanderSwitchDevice::applyHardwareOutput(const bool state, const uint32_t now) {
    IPortExpanderRuntime* expander = dependencyExpander();
    if (expander == nullptr) {
        return {DeviceError::InvalidRelationship, "port expander switch dependency is not ready"};
    }

    return expander->requestChannelState(config_.channel, state, now)
               ? DeviceValidationResult{}
               : DeviceValidationResult{DeviceError::StorageError, "port expander channel write failed"};
}

void PortExpanderSwitchDevice::releaseHardware(uint32_t now) {
    (void)now;
}

IPortExpanderRuntime* PortExpanderSwitchDevice::dependencyExpander() const {
    IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::PortExpander);
    if (dependency == nullptr) {
        return nullptr;
    }
    return const_cast<IPortExpanderRuntime*>(dependency->portExpanderRuntime());
}

} // namespace ewfm
