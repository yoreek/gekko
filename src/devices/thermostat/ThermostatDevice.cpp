#include "devices/thermostat/ThermostatDevice.h"

#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS ThermostatDevice

namespace {
constexpr const char* kControlOk = "ok";
constexpr const char* kControlNotReady = "not_ready";
constexpr const char* kControlDependencyBlocked = "dependency_blocked";
constexpr const char* kControlDisabled = "disabled";
constexpr const char* kControlSensorTimeout = "sensor_timeout";
constexpr const char* kControlOutOfRange = "out_of_range";
constexpr const char* kControlSwitchError = "switch_error";
constexpr const char* kControlRetryBackoff = "retry_backoff";
constexpr const char* kControlSafeOff = "safe_off";

bool configBlobToThermostatConfig(const DeviceConfigBlob& configBlob, ThermostatDeviceConfigV1& config) {
    return decodeThermostatDeviceConfig(configBlob.data(), configBlob.size(), config);
}

int32_t halfBandMilliCelsius(const ThermostatDeviceConfigV1& config) {
    return static_cast<int32_t>(config.hysteresisCentiCelsius) * 5;
}
} // namespace

ThermostatDevice::ThermostatDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : ThermostatDevice([&configBlob]() {
          ThermostatDeviceConfigV1 config{};
          (void)configBlobToThermostatConfig(configBlob, config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

ThermostatDevice::ThermostatDevice(const ThermostatDeviceConfigV1& config)
    : DeviceRuntimeBase((PState)&ThermostatDevice::Idle), config_(config) {}

const ThermostatDeviceConfigV1& ThermostatDevice::config() const {
    return config_;
}

bool ThermostatDevice::enabled() const {
    return config_.base.enabled != 0U;
}

const char* ThermostatDevice::name() const {
    return config_.base.name;
}

const TemperatureReading& ThermostatDevice::latestTemperature() const {
    return latestTemperature_;
}

OutputState ThermostatDevice::desiredOutputState() const {
    return desiredOutputState_;
}

OutputState ThermostatDevice::actualOutputState() const {
    return actualOutputState_;
}

const char* ThermostatDevice::controlStatus() const {
    return controlStatus_;
}

uint32_t ThermostatDevice::lastCheckAtMs() const {
    return lastCheckAtMs_;
}

void ThermostatDevice::setDependencyRuntime(DeviceDependencyRole role, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntime(role, dependencyRuntime);
    refreshCapabilityCache();
}

void ThermostatDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
    refreshCapabilityCache();
}

bool ThermostatDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = thermostatDeviceConfigSize(config_);
    return encodeThermostatDeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool ThermostatDevice::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    ThermostatDeviceConfigV1 config = config_;
    config.base = baseConfig;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = thermostatDeviceConfigSize(config);
    return encodeThermostatDeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan ThermostatDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    ThermostatDeviceConfigV1 config{};
    if (!decodeThermostatDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    return {};
}

bool ThermostatDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    ThermostatDeviceConfigV1 config{};
    if (!decodeThermostatDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    const bool checkIntervalChanged = config.checkIntervalMs != config_.checkIntervalMs;
    const bool retryAfterErrorChanged = config.retryAfterErrorMs != config_.retryAfterErrorMs;
    const bool minSwitchIntervalChanged = config.minSwitchIntervalMs != config_.minSwitchIntervalMs;
    config_ = config;
    if (checkIntervalChanged) {
        nextCheckAtMs_ = now + config_.checkIntervalMs;
    }
    if (retryAfterErrorChanged && status_ == DeviceStatus::Faulted) {
        retryDeadlineMs_ = now + config_.retryAfterErrorMs;
    }
    if (minSwitchIntervalChanged && pendingOutputState_ != desiredOutputState_) {
        nextCheckAtMs_ = lastOrdinarySwitchChangeAtMs_ + config_.minSwitchIntervalMs;
    }
    return true;
}

void ThermostatDevice::writeDeviceJson(JsonObject output) const {
    writeCommonDeviceJson(output);
    JsonObject configObject = output.createNestedObject("config");
    writeThermostatDeviceConfigJson(config_, configObject);
    JsonObject outputObject = output.createNestedObject("output");
    JsonObject temperatureObject = outputObject.createNestedObject("temperature");
    writeTemperatureOutputJson(latestTemperature_, TemperatureUnit::Celsius, controlStatus_, temperatureObject);
    outputObject["desired_switch_state"] = outputStateName(desiredOutputState_);
    outputObject["actual_switch_state"] = outputStateName(actualOutputState_);
    outputObject["last_check_at_ms"] = lastCheckAtMs_;
    outputObject["control_status"] = controlStatus_;
}

DeviceTypeDescriptor ThermostatDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kThermostatDeviceTypeId;
    descriptor.name = "ThermostatDevice";
    descriptor.currentConfigVersion = kThermostatDeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        DeviceDependencyRequirement{DeviceDependencyRole::TemperatureSensor, true, {Ds18b20TemperatureSensorDevice::descriptor().typeId}},
        DeviceDependencyRequirement{DeviceDependencyRole::Switch, true, {GpioSwitchDevice::descriptor().typeId}}};
    descriptor.createRuntime = &ThermostatDevice::createRuntime;
    descriptor.validateConfig = &ThermostatDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> ThermostatDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new ThermostatDevice(record, configBlob));
}

DeviceValidationResult ThermostatDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceDependencyRole::TemperatureSensor) == 0U ||
        record.dependencyDeviceId(DeviceDependencyRole::Switch) == 0U) {
        return {DeviceError::InvalidRelationship, "thermostat requires temperature_sensor and switch dependencies"};
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "thermostat config exceeds supported size"};
    }
    ThermostatDeviceConfigV1 config{};
    if (!decodeThermostatDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "thermostat config is invalid"};
    }
    return validateThermostatDeviceConfig(config);
}

void ThermostatDevice::refreshCapabilityCache() {
    temperatureSensor_ = nullptr;
    switchOutput_ = nullptr;
    if (const IDeviceRuntime* temperatureRuntime = dependencyRuntime(DeviceDependencyRole::TemperatureSensor);
        temperatureRuntime != nullptr) {
        temperatureSensor_ = temperatureRuntime->temperatureReadingRuntime();
    }
    if (const IDeviceRuntime* switchRuntime = dependencyRuntime(DeviceDependencyRole::Switch); switchRuntime != nullptr) {
        switchOutput_ = const_cast<ISwitchOutputRuntime*>(switchRuntime->switchOutputRuntime());
    }
}

bool ThermostatDevice::dependenciesAvailable() const {
    return dependencyRuntime(DeviceDependencyRole::TemperatureSensor) != nullptr &&
           dependencyRuntime(DeviceDependencyRole::Switch) != nullptr && temperatureSensor_ != nullptr && switchOutput_ != nullptr;
}

bool ThermostatDevice::dependenciesReady() const {
    return dependencyReady(DeviceDependencyRole::TemperatureSensor) && dependencyReady(DeviceDependencyRole::Switch) &&
           dependenciesAvailable();
}

bool ThermostatDevice::dependencyIsDisabled() const {
    return (dependencyRuntime(DeviceDependencyRole::TemperatureSensor) != nullptr &&
            dependencyRuntime(DeviceDependencyRole::TemperatureSensor)->status() == DeviceStatus::Disabled) ||
           (dependencyRuntime(DeviceDependencyRole::Switch) != nullptr &&
            dependencyRuntime(DeviceDependencyRole::Switch)->status() == DeviceStatus::Disabled);
}

bool ThermostatDevice::dependencyBlocked() const {
    if (!dependenciesAvailable()) {
        return true;
    }
    const IDeviceRuntime* temperatureRuntime = dependencyRuntime(DeviceDependencyRole::TemperatureSensor);
    const IDeviceRuntime* switchRuntime = dependencyRuntime(DeviceDependencyRole::Switch);
    return temperatureRuntime == nullptr || switchRuntime == nullptr || temperatureRuntime->status() != DeviceStatus::Ready ||
           switchRuntime->status() != DeviceStatus::Ready;
}

bool ThermostatDevice::sensorReadingTimedOut(uint32_t now) const {
    return sensorInvalidSinceMs_ != 0U && static_cast<uint32_t>(now - sensorInvalidSinceMs_) >= config_.sensorTimeoutMs;
}

bool ThermostatDevice::sensorReadingInRange(const TemperatureReading& reading) const {
    return reading.milliCelsius >= config_.minSafeMilliCelsius && reading.milliCelsius <= config_.maxSafeMilliCelsius;
}

OutputState ThermostatDevice::desiredStateForTemperature(const TemperatureReading& reading) const {
    if (static_cast<ThermostatMode>(config_.mode) == ThermostatMode::Off) {
        return OutputState::Off;
    }

    const int32_t halfBand = halfBandMilliCelsius(config_);
    const int32_t lower = config_.targetMilliCelsius - halfBand;
    const int32_t upper = config_.targetMilliCelsius + halfBand;

    if (static_cast<ThermostatMode>(config_.mode) == ThermostatMode::Heat) {
        if (reading.milliCelsius <= lower) {
            return OutputState::On;
        }
        if (reading.milliCelsius >= upper) {
            return OutputState::Off;
        }
        return desiredOutputState_;
    }

    if (reading.milliCelsius >= upper) {
        return OutputState::On;
    }
    if (reading.milliCelsius <= lower) {
        return OutputState::Off;
    }
    return desiredOutputState_;
}

bool ThermostatDevice::switchSupports(OutputState state) const {
    return switchOutput_ != nullptr && outputStateIsSupported(state, switchOutput_->supportedOutputStateMask());
}

bool ThermostatDevice::switchCapable() const {
    return switchOutput_ != nullptr;
}

bool ThermostatDevice::minSwitchIntervalElapsed(uint32_t now) const {
    if (config_.minSwitchIntervalMs == 0U || lastOrdinarySwitchChangeAtMs_ == 0U) {
        return true;
    }
    return static_cast<uint32_t>(now - lastOrdinarySwitchChangeAtMs_) >= config_.minSwitchIntervalMs;
}

void ThermostatDevice::publishTemperature(const TemperatureReading& reading, const char* status) {
    const bool changed = latestTemperature_.valid != reading.valid || latestTemperature_.measuredAtMs != reading.measuredAtMs ||
                         latestTemperature_.milliCelsius != reading.milliCelsius || std::strcmp(controlStatus_, status) != 0;
    latestTemperature_ = reading;
    controlStatus_ = status;
    if (changed) {
        markRuntimeStateDirty();
    }
}

void ThermostatDevice::invalidateTemperature(const char* status) {
    TemperatureReading invalid{};
    invalid.valid = false;
    publishTemperature(invalid, status);
}

void ThermostatDevice::scheduleNextCheck(uint32_t now) {
    nextCheckAtMs_ = now + config_.checkIntervalMs;
}

bool ThermostatDevice::requestSwitchOutput(OutputState state, uint32_t now, bool safetyOff) {
    if (switchOutput_ == nullptr) {
        return true;
    }
    if (!switchSupports(state)) {
        return false;
    }
    if (!switchOutput_->requestOutputState(state, now)) {
        return false;
    }
    desiredOutputState_ = state;
    actualOutputState_ = switchOutput_->currentOutputState();
    if (!safetyOff) {
        lastOrdinarySwitchChangeAtMs_ = now;
    }
    markRuntimeStateDirty();
    return true;
}

bool ThermostatDevice::requestSafeOff(uint32_t now) {
    pendingSafetyOff_ = true;
    pendingOutputState_ = OutputState::Off;
    if (switchOutput_ == nullptr) {
        return true;
    }
    if (!switchSupports(OutputState::Off)) {
        return false;
    }
    if (!switchOutput_->requestOutputState(OutputState::Off, now)) {
        return false;
    }
    desiredOutputState_ = OutputState::Off;
    actualOutputState_ = switchOutput_->currentOutputState();
    markRuntimeStateDirty();
    return true;
}

SM_STATE(ThermostatDevice::Idle) {
    status_ = DeviceStatus::Creating;
    const uint32_t now = uptime();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        (void)requestSafeOff(now);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.base.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(ThermostatDevice::Starting) {
    status_ = DeviceStatus::Starting;
    const uint32_t now = uptime();
    refreshCapabilityCache();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        (void)requestSafeOff(now);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.base.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateTemperature(kControlDependencyBlocked);
        (void)requestSafeOff(now);
        SM_GOTO(DependencyBlocked);
    }

    startRequested_ = false;
    reconfigureRequested_ = false;
    faultRequested_ = false;
    lastCheckAtMs_ = now;
    nextCheckAtMs_ = now;
    retryDeadlineMs_ = 0;
    pendingSafetyOff_ = false;
    pendingOutputState_ = OutputState::Off;
    desiredOutputState_ = switchCapable() ? switchOutput_->currentOutputState() : OutputState::Off;
    actualOutputState_ = desiredOutputState_;
    invalidateTemperature(kControlNotReady);
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(ThermostatDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    const uint32_t now = uptime();
    refreshCapabilityCache();
    reconfigureRequested_ = false;
    faultRequested_ = false;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        (void)requestSafeOff(now);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.base.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateTemperature(kControlDependencyBlocked);
        (void)requestSafeOff(now);
        SM_GOTO(DependencyBlocked);
    }

    lastCheckAtMs_ = now;
    nextCheckAtMs_ = now;
    retryDeadlineMs_ = 0;
    controlStatus_ = kControlNotReady;
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(ThermostatDevice::Ready) {
    status_ = DeviceStatus::Ready;
    const uint32_t now = uptime();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        (void)requestSafeOff(now);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.base.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateTemperature(kControlDependencyBlocked);
        (void)requestSafeOff(now);
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        SM_GOTO(Reconfiguring);
    }
    if (faultRequested_) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
    if (!EWFM_SM_TIME_REACHED(now, nextCheckAtMs_)) {
        return;
    }
    SM_GOTO(CheckTemperature);
}

SM_STATE(ThermostatDevice::CheckTemperature) {
    status_ = DeviceStatus::Ready;
    const uint32_t now = uptime();
    TemperatureReading reading{};
    if (temperatureSensor_ == nullptr || !temperatureSensor_->latestTemperatureReading(reading) || !reading.valid) {
        if (sensorInvalidSinceMs_ == 0U) {
            sensorInvalidSinceMs_ = now;
        }
        invalidateTemperature(kControlSensorTimeout);
        lastCheckAtMs_ = now;
        if (sensorReadingTimedOut(now)) {
            retryDeadlineMs_ = now + config_.retryAfterErrorMs;
            status_ = DeviceStatus::Faulted;
            SM_GOTO(Faulted);
        }
        scheduleNextCheck(now);
        SM_GOTO(Ready);
    }
    sensorInvalidSinceMs_ = 0U;
    if (!sensorReadingInRange(reading)) {
        invalidateTemperature(kControlOutOfRange);
        retryDeadlineMs_ = now + config_.retryAfterErrorMs;
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }

    publishTemperature(reading, kControlOk);
    lastCheckAtMs_ = now;
    sensorInvalidSinceMs_ = 0U;
    pendingOutputState_ = desiredStateForTemperature(reading);
    pendingSafetyOff_ = false;

    if (pendingOutputState_ == desiredOutputState_) {
        if (actualOutputState_ != desiredOutputState_ && switchCapable()) {
            SM_GOTO(ApplyOutput);
        }
        scheduleNextCheck(now);
        SM_GOTO(Ready);
    }

    if (!minSwitchIntervalElapsed(now)) {
        nextCheckAtMs_ = lastOrdinarySwitchChangeAtMs_ + config_.minSwitchIntervalMs;
        SM_GOTO(Ready);
    }

    SM_GOTO(ApplyOutput);
}

SM_STATE(ThermostatDevice::ApplyOutput) {
    status_ = DeviceStatus::Ready;
    const uint32_t now = uptime();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        (void)requestSafeOff(now);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.base.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        (void)requestSafeOff(now);
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateTemperature(kControlDependencyBlocked);
        (void)requestSafeOff(now);
        SM_GOTO(DependencyBlocked);
    }

    if (!pendingSafetyOff_ && pendingOutputState_ != OutputState::Off && !switchSupports(pendingOutputState_)) {
        invalidateTemperature(kControlSwitchError);
        retryDeadlineMs_ = now + config_.retryAfterErrorMs;
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }

    if (!pendingSafetyOff_ && !minSwitchIntervalElapsed(now) && pendingOutputState_ != desiredOutputState_) {
        nextCheckAtMs_ = lastOrdinarySwitchChangeAtMs_ + config_.minSwitchIntervalMs;
        SM_GOTO(Ready);
    }

    if (!requestSwitchOutput(pendingOutputState_, now, pendingSafetyOff_)) {
        invalidateTemperature(kControlSwitchError);
        retryDeadlineMs_ = now + config_.retryAfterErrorMs;
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }

    pendingSafetyOff_ = false;
    scheduleNextCheck(now);
    SM_GOTO(Ready);
}

SM_STATE(ThermostatDevice::RetryBackoff) {
    status_ = DeviceStatus::Faulted;
    const uint32_t now = uptime();
    controlStatus_ = kControlRetryBackoff;
    (void)requestSafeOff(now);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.base.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (!EWFM_SM_TIME_REACHED(now, retryDeadlineMs_)) {
        return;
    }
    SM_GOTO(Reconfiguring);
}

SM_STATE(ThermostatDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    const uint32_t now = uptime();
    controlStatus_ = kControlDependencyBlocked;
    (void)requestSafeOff(now);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.base.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependenciesReady()) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(ThermostatDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    const uint32_t now = uptime();
    controlStatus_ = kControlDisabled;
    (void)requestSafeOff(now);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (!disableRequested_ && config_.base.enabled != 0U && dependenciesReady()) {
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(ThermostatDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    const uint32_t now = uptime();
    if (std::strcmp(controlStatus_, kControlNotReady) == 0) {
        controlStatus_ = kControlSwitchError;
    }
    (void)requestSafeOff(now);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.base.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    SM_GOTO(RetryBackoff);
}

SM_STATE(ThermostatDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    const uint32_t now = uptime();
    controlStatus_ = kControlSafeOff;
    (void)requestSafeOff(now);
    setDeleted();
}

} // namespace ewfm
