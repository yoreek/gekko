#include "devices/rtc/ds1302/Ds1302RtcDevice.h"

#include "debug/Debug.h"
#include "devices/rtc/ds1302/Ds1302Protocol.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Ds1302RtcDevice

namespace {
Ds1302Pins pinsOf(const Ds1302RtcDeviceConfigV1& config) {
    return Ds1302Pins{config.clkPin, config.dataPin, config.rstPin};
}
} // namespace

Ds1302RtcDevice::Ds1302RtcDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Ds1302RtcDevice(
          [&configBlob]() {
              Ds1302RtcDeviceConfigV1 config{};
              (void)decodeDs1302RtcDeviceConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          defaultArduinoDs1302LineDriver()) {
    bindDeviceIdentity(record, configBlob);
}

Ds1302RtcDevice::Ds1302RtcDevice(const Ds1302RtcDeviceConfigV1& config, IDs1302LineDriver& lineDriver)
    : Ds1302RtcDeviceBase((PState)&Ds1302RtcDevice::Idle), config_(config), lineDriver_(lineDriver) {}

const Ds1302RtcDeviceConfigV1& Ds1302RtcDevice::config() const {
    return config_;
}

bool Ds1302RtcDevice::writeTime(uint32_t utcEpoch) {
    return ds1302WriteTime(lineDriver_, pinsOf(config_), utcEpoch);
}

void Ds1302RtcDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool Ds1302RtcDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ds1302RtcDeviceConfigSize(config_);
    return encodeFixedConfigBlob(Ds1302RtcDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Ds1302RtcDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Ds1302RtcDeviceConfigV1 config{};
    if (!decodeDs1302RtcDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    const bool pinsChanged = config.clkPin != config_.clkPin || config.dataPin != config_.dataPin || config.rstPin != config_.rstPin;
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = pinsChanged;
    plan.resetStateMachine = pinsChanged;
    return plan;
}

bool Ds1302RtcDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Ds1302RtcDeviceConfigV1 config{};
    if (!decodeDs1302RtcDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

DeviceTypeDescriptor Ds1302RtcDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kDs1302RtcTypeId;
    descriptor.name = "Ds1302RtcDevice";
    descriptor.currentConfigVersion = kDs1302RtcConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.providedRoles = ProvidedRoles::of({IRealTimeClockRuntime::kProvidedRole});
    descriptor.createRuntime = &Ds1302RtcDevice::createRuntime;
    descriptor.validateConfig = &Ds1302RtcDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Ds1302RtcDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Ds1302RtcDevice(record, configBlob));
}

DeviceValidationResult Ds1302RtcDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "ds1302 config exceeds supported size"};
    }
    Ds1302RtcDeviceConfigV1 config{};
    if (!decodeDs1302RtcDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "ds1302 config is invalid"};
    }
    return config.validate();
}

SM_STATE(Ds1302RtcDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(Ds1302RtcDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    // No bus/ack to probe over a bit-bang link - the first read in Reading() doubles as the probe.
    clearStartRequested();
    SM_GOTO(Reading);
}

SM_STATE(Ds1302RtcDevice::Reading) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        clearReconfigureRequested();
        readingState_.resetErrors();
        SM_GOTO(Starting);
    }

    uint32_t epoch = 0;
    if (!ds1302ReadTime(lineDriver_, pinsOf(config_), epoch)) {
        recordFailure();
        if (readingState_.consecutiveErrors() >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    // DS1302 has no reliable lost-power signal (its CH bit only reflects whether we last told it to
    // halt, not real backup-battery loss - see docs/rest-api-contract.md), so this always reports
    // lostPower=false, unlike Ds3231RtcDevice's oscillator-stop-flag reading.
    recordSuccess(epoch, false);
    SM_GOTO(Ready);
}

SM_STATE(Ds1302RtcDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        clearReconfigureRequested();
        readingState_.resetErrors();
        SM_GOTO(Starting);
    }
    if (!elapsed(uptime(), kPollMs)) {
        return;
    }
    SM_GOTO(Reading);
}

SM_STATE(Ds1302RtcDevice::RetryBackoff) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        clearReconfigureRequested();
        readingState_.resetErrors();
        SM_GOTO(Starting);
    }
    if (!elapsed(uptime(), kRetryBackoffMs)) {
        return;
    }
    SM_GOTO(Reading);
}

SM_STATE(Ds1302RtcDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_ && config_.enabled != 0U) {
        clearReconfigureRequested();
        SM_GOTO(Starting);
    }
}

SM_STATE(Ds1302RtcDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        clearFaultRequested();
        clearReconfigureRequested();
        readingState_.resetErrors();
        SM_GOTO(Starting);
    }
    if (!elapsed(uptime(), kFaultRetryBackoffMs)) {
        return;
    }
    SM_GOTO(Starting);
}

SM_STATE(Ds1302RtcDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
