#include "devices/bus/spi/SpiBusDevice.h"

#include <cstring>
#include <type_traits>
#include <utility>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS SpiBusDevice

namespace {
constexpr DeviceTypeId kSpiBusDeviceTypeId = 8;
constexpr uint32_t kSpiBusDeviceConfigVersion = 1;
constexpr uint32_t kSpiDiagnosticsDebounceMs = 1000U;

bool samePin(const uint8_t left, const uint8_t right) {
    return left == right;
}
} // namespace

static_assert(std::is_trivially_copyable<SpiBusDeviceConfigV1>::value, "SpiBusDeviceConfigV1 must be POD");
static_assert(sizeof(SpiBusDeviceConfigV1::kMagic) - 1U + sizeof(SpiBusDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "SpiBusDeviceConfigV1 exceeds device config bound");

SpiBusDevice::DependencyTransaction::DependencyTransaction(SpiBusDevice* bus, ISpiBusDriver* driver, uint32_t generation)
    : bus_(bus), driver_(driver), generation_(generation) {}

SpiBusDevice::DependencyTransaction::DependencyTransaction(DependencyTransaction&& other) noexcept
    : bus_(other.bus_), driver_(other.driver_), generation_(other.generation_) {
    other.bus_ = nullptr;
    other.driver_ = nullptr;
    other.generation_ = 0;
}

SpiBusDevice::DependencyTransaction& SpiBusDevice::DependencyTransaction::operator=(DependencyTransaction&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    release();
    bus_ = other.bus_;
    driver_ = other.driver_;
    generation_ = other.generation_;
    other.bus_ = nullptr;
    other.driver_ = nullptr;
    other.generation_ = 0;
    return *this;
}

SpiBusDevice::DependencyTransaction::~DependencyTransaction() {
    release();
}

SpiBusDevice::DependencyTransaction::operator bool() const {
    return bus_ != nullptr && driver_ != nullptr && generation_ != 0U && bus_->generation() == generation_;
}

ISpiBusDriver* SpiBusDevice::DependencyTransaction::driver() const {
    return bus_ != nullptr && driver_ != nullptr && bus_->generation() == generation_ ? driver_ : nullptr;
}

uint32_t SpiBusDevice::DependencyTransaction::generation() const {
    return generation_;
}

void SpiBusDevice::DependencyTransaction::release() {
    if (bus_ != nullptr) {
        bus_->releaseDependencyTransaction();
    }
    bus_ = nullptr;
    driver_ = nullptr;
    generation_ = 0;
}

SpiBusDevice::SpiBusDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : SpiBusDevice(
          [&configBlob]() {
              SpiBusDeviceConfigV1 config{};
              (void)decodeSpiBusDeviceConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          createArduinoSpiBusDriver()) {
    bindDeviceIdentity(record, configBlob);
}

SpiBusDevice::SpiBusDevice(const SpiBusDeviceConfigV1& config, ISpiBusDriver& driver)
    : DeviceRuntimeBase((PState)&SpiBusDevice::Idle), config_(config), driver_(driver) {}

SpiBusDevice::SpiBusDevice(const SpiBusDeviceConfigV1& config, std::unique_ptr<ISpiBusDriver> ownedDriver)
    : DeviceRuntimeBase((PState)&SpiBusDevice::Idle), config_(config), ownedDriver_(std::move(ownedDriver)),
      driver_(ownedDriver_ != nullptr ? *ownedDriver_ : defaultArduinoSpiBusDriver()) {}

const SpiBusDeviceConfigV1& SpiBusDevice::config() const {
    return config_;
}

bool SpiBusDevice::enabled() const {
    return config_.enabled != 0U;
}

const char* SpiBusDevice::name() const {
    return config_.name;
}

uint32_t SpiBusDevice::generation() const {
    return generation_;
}

bool SpiBusDevice::dependencyTransactionActive() const {
    return dependencyTransactionActive_;
}

const BusRuntimeDiagnostics& SpiBusDevice::diagnostics() const {
    return diagnostics_;
}

void SpiBusDevice::resetDiagnostics(uint32_t now) {
    (void)now;
    diagnostics_.reset();
    markRuntimeStateDirty();
}

void SpiBusDevice::recordDiagnosticsError(uint32_t errorCode, uint32_t now) {
    if (diagnostics_.recordError(errorCode, now, kSpiDiagnosticsDebounceMs)) {
        markRuntimeStateDirty();
    }
}

void SpiBusDevice::recordDiagnosticsSuccess() {
    if (diagnostics_.recordSuccess()) {
        markRuntimeStateDirty();
    }
}

void SpiBusDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool SpiBusDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = spiBusDeviceConfigSize(config_);
    return encodeSpiBusDeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool SpiBusDevice::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    SpiBusDeviceConfigV1 config = config_;
    config.enabled = baseConfig.enabled;
    std::memcpy(config.name, baseConfig.name, sizeof(config.name));
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = spiBusDeviceConfigSize(config);
    return encodeSpiBusDeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan SpiBusDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    SpiBusDeviceConfigV1 config{};
    if (!decodeSpiBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool changed = config.host != config_.host || config.sckPin != config_.sckPin || config.mosiPin != config_.mosiPin ||
                         config.misoPin != config_.misoPin;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = changed;
    plan.resetStateMachine = changed;
    return plan;
}

bool SpiBusDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    SpiBusDeviceConfigV1 config{};
    if (!decodeSpiBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

void SpiBusDevice::writeDeviceJson(JsonObject output) const {
    writeCommonDeviceJson(output);
    config_.writeJson(output);
    JsonObject runtimeJson = output.createNestedObject("runtime");
    runtimeJson["generation"] = generation_;
    runtimeJson["transactionActive"] = dependencyTransactionActive_;
    diagnostics_.writeJson(runtimeJson);
}

SpiBusDevice::DependencyTransaction SpiBusDevice::beginDependencyTransaction() {
    if (status_ != DeviceStatus::Ready || dependencyTransactionActive_ || disableRequested_ || deleteRequested_ || reconfigureRequested_ ||
        !config_.enabled) {
        return {};
    }

    dependencyTransactionActive_ = true;
    return DependencyTransaction(this, &driver_, generation_);
}

bool SpiBusDevice::hasDuplicateDependentSpiChipSelect(uint8_t pin, const IDeviceRuntime* ignoreDependent) const {
    for (const IDeviceRuntime* dependent : dependentRuntimes()) {
        if (dependent == nullptr || dependent == ignoreDependent) {
            continue;
        }

        uint8_t dependentPin{0};
        if (!dependent->spiChipSelectPin(dependentPin)) {
            continue;
        }
        if (samePin(pin, dependentPin)) {
            return true;
        }
    }
    return false;
}

DeviceTypeDescriptor SpiBusDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kSpiBusDeviceTypeId;
    descriptor.name = "SpiBusDevice";
    descriptor.currentConfigVersion = kSpiBusDeviceConfigVersion;
    descriptor.maxDependents = 16;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.createRuntime = &SpiBusDevice::createRuntime;
    descriptor.validateConfig = &SpiBusDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> SpiBusDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new SpiBusDevice(record, configBlob));
}

DeviceValidationResult SpiBusDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "spi bus config exceeds supported size"};
    }
    SpiBusDeviceConfigV1 config{};
    if (!decodeSpiBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "spi bus config is invalid"};
    }
    return {};
}

bool SpiBusDevice::handleCommand(const DeviceCommand& command) {
    if (command.type == DeviceCommandType::ResetDiagnostics) {
        if (command.payload.empty()) {
            resetDiagnostics(uptime());
            return true;
        }
        return false;
    }
    return false;
}

void SpiBusDevice::releaseHardware() {
    dependencyTransactionActive_ = false;
    (void)driver_.end();
}

void SpiBusDevice::end(uint32_t now) {
    (void)now;
    releaseHardware();
}

void SpiBusDevice::releaseDependencyTransaction() {
    dependencyTransactionActive_ = false;
}

DeviceValidationResult SpiBusDevice::initializeHardware(uint32_t now) {
    (void)now;
    if (!driver_.begin(config_.host, config_.sckPin, config_.mosiPin, config_.misoPin)) {
        recordDiagnosticsError(1U, uptime());
        return {DeviceError::StorageError, "spi bus driver initialization failed"};
    }
    recordDiagnosticsSuccess();
    ++generation_;
    if (generation_ == 0U) {
        ++generation_;
    }
    return {};
}

SM_STATE(SpiBusDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.enabled) {
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(SpiBusDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.enabled) {
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!initializeHardware(uptime()).ok()) {
        status_ = DeviceStatus::Faulted;
        requestFault();
        SM_GOTO(Faulted);
    }

    startRequested_ = false;
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(SpiBusDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.enabled) {
        releaseHardware();
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

SM_STATE(SpiBusDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    releaseHardware();
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (!initializeHardware(uptime()).ok()) {
        status_ = DeviceStatus::Faulted;
        requestFault();
        SM_GOTO(Faulted);
    }
    status_ = DeviceStatus::Ready;
    markRuntimeStateDirty();
    SM_GOTO(Ready);
}

SM_STATE(SpiBusDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    releaseHardware();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(SpiBusDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    releaseHardware();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        faultRequested_ = false;
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(SpiBusDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    releaseHardware();
    setDeleted();
}

SM_STATE(SpiBusDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_) {
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_ || startRequested_) {
        if (dependenciesReady()) {
            status_ = DeviceStatus::Reconfiguring;
            SM_GOTO(Reconfiguring);
        }
    }
}

} // namespace ewfm
