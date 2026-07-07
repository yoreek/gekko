#include "devices/bus/i2c/I2cBusDevice.h"

#include <cstdio>
#include <cstring>
#include <type_traits>
#include <utility>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS I2cBusDevice

namespace {
constexpr DeviceTypeId kI2cBusDeviceTypeId = 6;
constexpr uint32_t kI2cBusDeviceConfigVersion = 1;
constexpr uint32_t kI2cDiagnosticsDebounceMs = 1000U;
} // namespace

static_assert(std::is_trivially_copyable<I2cBusDeviceConfigV1>::value, "I2cBusDeviceConfigV1 must be POD");
static_assert(sizeof(I2cBusDeviceConfigV1::kMagic) - 1U + sizeof(I2cBusDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "I2cBusDeviceConfigV1 exceeds device config bound");

I2cBusDevice::DependencyTransaction::DependencyTransaction(I2cBusDevice* bus, II2cBusDriver* driver, uint32_t generation)
    : bus_(bus), driver_(driver), generation_(generation) {}

I2cBusDevice::DependencyTransaction::DependencyTransaction(DependencyTransaction&& other) noexcept
    : bus_(other.bus_), driver_(other.driver_), generation_(other.generation_) {
    other.bus_ = nullptr;
    other.driver_ = nullptr;
    other.generation_ = 0;
}

I2cBusDevice::DependencyTransaction& I2cBusDevice::DependencyTransaction::operator=(DependencyTransaction&& other) noexcept {
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

I2cBusDevice::DependencyTransaction::~DependencyTransaction() {
    release();
}

I2cBusDevice::DependencyTransaction::operator bool() const {
    return bus_ != nullptr && driver_ != nullptr && generation_ != 0U;
}

II2cBusDriver* I2cBusDevice::DependencyTransaction::driver() const {
    return driver_;
}

uint32_t I2cBusDevice::DependencyTransaction::generation() const {
    return generation_;
}

void I2cBusDevice::DependencyTransaction::release() {
    if (bus_ != nullptr) {
        bus_->releaseDependencyTransaction();
    }
    bus_ = nullptr;
    driver_ = nullptr;
    generation_ = 0;
}

I2cBusDevice::I2cBusDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : I2cBusDevice(
          [&configBlob]() {
              I2cBusDeviceConfigV1 config{};
              (void)decodeI2cBusDeviceConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          createArduinoI2cBusDriver()) {
    bindDeviceIdentity(record, configBlob);
}

I2cBusDevice::I2cBusDevice(const I2cBusDeviceConfigV1& config, II2cBusDriver& driver)
    : DeviceRuntimeBase((PState)&I2cBusDevice::Idle), config_(config), driver_(driver) {}

I2cBusDevice::I2cBusDevice(const I2cBusDeviceConfigV1& config, std::unique_ptr<II2cBusDriver> ownedDriver)
    : DeviceRuntimeBase((PState)&I2cBusDevice::Idle), config_(config), ownedDriver_(std::move(ownedDriver)),
      driver_(ownedDriver_ != nullptr ? *ownedDriver_ : defaultArduinoI2cBusDriver()) {}

const I2cBusDeviceConfigV1& I2cBusDevice::config() const {
    return config_;
}

bool I2cBusDevice::enabled() const {
    return config_.enabled != 0U;
}

const char* I2cBusDevice::name() const {
    return config_.name;
}

uint32_t I2cBusDevice::generation() const {
    return generation_;
}

bool I2cBusDevice::dependencyTransactionActive() const {
    return dependencyTransactionActive_;
}

const I2cScanResult& I2cBusDevice::scan() const {
    return scan_;
}

const BusRuntimeDiagnostics& I2cBusDevice::diagnostics() const {
    return diagnostics_;
}

void I2cBusDevice::resetDiagnostics(uint32_t now) {
    (void)now;
    diagnostics_.reset();
    markRuntimeStateDirty();
}

void I2cBusDevice::recordDiagnosticsError(uint32_t errorCode, uint32_t now) {
    if (diagnostics_.recordError(errorCode, now, kI2cDiagnosticsDebounceMs)) {
        markRuntimeStateDirty();
    }
}

void I2cBusDevice::recordDiagnosticsSuccess() {
    if (diagnostics_.recordSuccess()) {
        markRuntimeStateDirty();
    }
}

void I2cBusDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool I2cBusDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = i2cBusDeviceConfigSize(config_);
    return encodeI2cBusDeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool I2cBusDevice::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    I2cBusDeviceConfigV1 config = config_;
    config.enabled = baseConfig.enabled;
    std::memcpy(config.name, baseConfig.name, sizeof(config.name));
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = i2cBusDeviceConfigSize(config);
    return encodeI2cBusDeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan I2cBusDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    I2cBusDeviceConfigV1 config{};
    if (!decodeI2cBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool changed = config.sdaPin != config_.sdaPin || config.sclPin != config_.sclPin ||
                         config.internalPullup != config_.internalPullup || config.frequencyHz != config_.frequencyHz;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = changed;
    plan.resetStateMachine = changed;
    return plan;
}

bool I2cBusDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    I2cBusDeviceConfigV1 config{};
    if (!decodeI2cBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

void I2cBusDevice::writeDeviceJson(JsonObject output) const {
    writeCommonDeviceJson(output);
    config_.writeJson(output);
    // Reuse the "runtime" object IDeviceApiAdapter::writeCommonDeviceJson already created (with
    // status/effectiveStatus) rather than createNestedObject(), which would clear it and silently
    // drop those fields from the response.
    JsonObject runtimeJson = output["runtime"].as<JsonObject>();
    runtimeJson["generation"] = generation_;
    runtimeJson["transactionActive"] = dependencyTransactionActive_;
    diagnostics_.writeJson(runtimeJson);
    JsonObject scanJson = runtimeJson.createNestedObject("scan");
    scanJson["inProgress"] = scan_.inProgress;
    scanJson["ready"] = scan_.ready;
    scanJson["deviceCount"] = scan_.deviceCount;
    scanJson["truncated"] = scan_.truncated;
    scanJson["nextAddress"] = scan_.nextAddress;
    JsonArray devices = scanJson.createNestedArray("devices");
    for (uint8_t index = 0; index < scan_.deviceCount; ++index) {
        JsonObject item = devices.createNestedObject();
        char addressHex[8]{};
        std::snprintf(addressHex, sizeof(addressHex), "0x%02X", scan_.devices[index]);
        item["address"] = scan_.devices[index];
        item["addressHex"] = addressHex;
    }
}

I2cBusDevice::DependencyTransaction I2cBusDevice::beginDependencyTransaction() {
    if (status_ != DeviceStatus::Ready || dependencyTransactionActive_ || disableRequested_ || deleteRequested_ || reconfigureRequested_ ||
        !config_.enabled) {
        return {};
    }

    dependencyTransactionActive_ = true;
    return DependencyTransaction(this, &driver_, generation_);
}

bool I2cBusDevice::hasDuplicateDependentI2cAddress(uint8_t address, const IDeviceRuntime* ignoreDependent) const {
    if (address > 0x7FU) {
        return false;
    }
    for (const IDeviceRuntime* dependent : dependentRuntimes()) {
        if (dependent == nullptr || dependent == ignoreDependent) {
            continue;
        }

        uint8_t dependentAddress{0};
        if (!dependent->i2cAddress(dependentAddress)) {
            continue;
        }
        if (dependentAddress == address) {
            return true;
        }
    }
    return false;
}

DeviceTypeDescriptor I2cBusDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kI2cBusDeviceTypeId;
    descriptor.name = "I2cBusDevice";
    descriptor.currentConfigVersion = kI2cBusDeviceConfigVersion;
    descriptor.maxDependents = 16;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.createRuntime = &I2cBusDevice::createRuntime;
    descriptor.validateConfig = &I2cBusDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> I2cBusDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new I2cBusDevice(record, configBlob));
}

DeviceValidationResult I2cBusDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "i2c bus config exceeds supported size"};
    }
    I2cBusDeviceConfigV1 config{};
    if (!decodeI2cBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "i2c bus config is invalid"};
    }
    return {};
}

bool I2cBusDevice::handleCommand(const DeviceCommand& command) {
    if (command.type == DeviceCommandType::ResetDiagnostics) {
        if (command.payload.empty()) {
            resetDiagnostics(uptime());
            return true;
        }
        return false;
    }
    if (command.type != DeviceCommandType::Scan && command.type != DeviceCommandType::Custom) {
        return false;
    }
    if (command.type == DeviceCommandType::Custom && !command.payload.equals("scan")) {
        return false;
    }
    if (command.type == DeviceCommandType::Scan && !command.payload.empty()) {
        return false;
    }
    if (status_ != DeviceStatus::Ready || isScanning() || disableRequested_ || deleteRequested_ || reconfigureRequested_ ||
        !config_.enabled) {
        return false;
    }

    startScan();
    return true;
}

void I2cBusDevice::releaseHardware() {
    dependencyTransactionActive_ = false;
    scan_.inProgress = false;
    (void)driver_.end();
}

void I2cBusDevice::end(uint32_t now) {
    (void)now;
    releaseHardware();
}

void I2cBusDevice::releaseDependencyTransaction() {
    dependencyTransactionActive_ = false;
}

DeviceValidationResult I2cBusDevice::initializeHardware(uint32_t now) {
    (void)now;
    if (!driver_.begin(config_.sdaPin, config_.sclPin, config_.frequencyHz, config_.internalPullup != 0U)) {
        recordDiagnosticsError(1U, uptime());
        return {DeviceError::StorageError, "i2c bus driver initialization failed"};
    }
    if (!driver_.setClock(config_.frequencyHz)) {
        (void)driver_.end();
        recordDiagnosticsError(2U, uptime());
        return {DeviceError::StorageError, "i2c bus clock configuration failed"};
    }
    recordDiagnosticsSuccess();
    ++generation_;
    if (generation_ == 0U) {
        ++generation_;
    }
    return {};
}

void I2cBusDevice::resetScanResult() {
    scan_ = {};
}

void I2cBusDevice::startScan() {
    resetScanResult();
    scan_.inProgress = true;
    scan_.ready = false;
    scan_.truncated = false;
    scan_.nextAddress = kI2cScanFirstAddress;
    markRuntimeStateDirty();
    setState((PState)&I2cBusDevice::Scanning);
}

void I2cBusDevice::finishScan() {
    scan_.inProgress = false;
    scan_.ready = true;
    markRuntimeStateDirty();
}

void I2cBusDevice::cancelScan() {
    scan_.inProgress = false;
    scan_.ready = false;
    markRuntimeStateDirty();
}

void I2cBusDevice::appendScanAddress(uint8_t address) {
    if (scan_.deviceCount >= kMaxI2cScanDevices) {
        scan_.truncated = true;
        markRuntimeStateDirty();
        return;
    }

    scan_.devices[scan_.deviceCount] = address;
    ++scan_.deviceCount;
    markRuntimeStateDirty();
}

bool I2cBusDevice::isScanning() const {
    return scan_.inProgress;
}

SM_STATE(I2cBusDevice::Idle) {
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

SM_STATE(I2cBusDevice::Starting) {
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

SM_STATE(I2cBusDevice::Ready) {
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

SM_STATE(I2cBusDevice::Scanning) {
    status_ = DeviceStatus::Ready;
    if (!scan_.inProgress) {
        SM_GOTO(Ready);
    }
    if (!dependenciesReady()) {
        cancelScan();
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (deleteRequested_) {
        cancelScan();
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.enabled) {
        cancelScan();
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        cancelScan();
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (scan_.nextAddress > kI2cScanLastAddress) {
        finishScan();
        SM_GOTO(Ready);
    }

    const uint8_t address = scan_.nextAddress++;
    driver_.beginTransmission(address);
    const uint8_t result = driver_.endTransmission();
    if (result == 0U) {
        appendScanAddress(address);
    } else if (result > 2U) {
        recordDiagnosticsError(result, uptime());
    }
    if (scan_.nextAddress > kI2cScanLastAddress) {
        finishScan();
        SM_GOTO(Ready);
    }
}

SM_STATE(I2cBusDevice::Reconfiguring) {
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

SM_STATE(I2cBusDevice::Disabled) {
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

SM_STATE(I2cBusDevice::Faulted) {
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

SM_STATE(I2cBusDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    releaseHardware();
    setDeleted();
}

SM_STATE(I2cBusDevice::DependencyBlocked) {
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
