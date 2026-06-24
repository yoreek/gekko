#include "devices/bus/onewire/OneWireBusDevice.h"

#include <cstring>
#include <type_traits>
#include <utility>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS OneWireBusDevice

namespace {
constexpr DeviceTypeId kOneWireBusDeviceTypeId = 3;
constexpr uint32_t kOneWireBusDeviceConfigVersion = 1;

bool sameRomAddress(const OneWireRomAddress& left, const OneWireRomAddress& right) {
    return std::memcmp(left.bytes, right.bytes, sizeof(left.bytes)) == 0;
}
} // namespace

static_assert(std::is_trivially_copyable<OneWireBusDeviceConfigV1>::value, "OneWireBusDeviceConfigV1 must be POD");
static_assert(sizeof(OneWireBusDeviceConfigV1::kMagic) - 1U + sizeof(OneWireBusDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "OneWireBusDeviceConfigV1 exceeds device config bound");

OneWireBusDevice::DependencyTransaction::DependencyTransaction(OneWireBusDevice* bus, IOneWireBusDriver* driver, uint32_t generation)
    : bus_(bus), driver_(driver), generation_(generation) {}

OneWireBusDevice::DependencyTransaction::DependencyTransaction(DependencyTransaction&& other) noexcept
    : bus_(other.bus_), driver_(other.driver_), generation_(other.generation_) {
    other.bus_ = nullptr;
    other.driver_ = nullptr;
    other.generation_ = 0;
}

OneWireBusDevice::DependencyTransaction& OneWireBusDevice::DependencyTransaction::operator=(DependencyTransaction&& other) noexcept {
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

OneWireBusDevice::DependencyTransaction::~DependencyTransaction() {
    release();
}

OneWireBusDevice::DependencyTransaction::operator bool() const {
    return bus_ != nullptr && driver_ != nullptr;
}

IOneWireBusDriver* OneWireBusDevice::DependencyTransaction::driver() const {
    return driver_;
}

uint32_t OneWireBusDevice::DependencyTransaction::generation() const {
    return generation_;
}

void OneWireBusDevice::DependencyTransaction::release() {
    if (bus_ != nullptr) {
        bus_->releaseDependencyTransaction();
    }
    bus_ = nullptr;
    driver_ = nullptr;
    generation_ = 0;
}

OneWireBusDevice::OneWireBusDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : OneWireBusDevice(
          [&configBlob]() {
              OneWireBusDeviceConfigV1 config{};
              (void)decodeOneWireBusDeviceConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          createArduinoOneWireBusDriver()) {
    bindDeviceIdentity(record, configBlob);
}

OneWireBusDevice::OneWireBusDevice(const OneWireBusDeviceConfigV1& config, IOneWireBusDriver& driver)
    : DeviceRuntimeBase((PState)&OneWireBusDevice::Idle), config_(config), driver_(driver) {}

OneWireBusDevice::OneWireBusDevice(const OneWireBusDeviceConfigV1& config, std::unique_ptr<IOneWireBusDriver> ownedDriver)
    : DeviceRuntimeBase((PState)&OneWireBusDevice::Idle), config_(config), ownedDriver_(std::move(ownedDriver)),
      driver_(ownedDriver_ != nullptr ? *ownedDriver_ : defaultArduinoOneWireBusDriver()) {}

const OneWireBusDeviceConfigV1& OneWireBusDevice::config() const {
    return config_;
}

bool OneWireBusDevice::enabled() const {
    return config_.base.enabled != 0U;
}

const char* OneWireBusDevice::name() const {
    return config_.base.name;
}

const OneWireScanResult& OneWireBusDevice::scan() const {
    return scan_;
}

uint32_t OneWireBusDevice::generation() const {
    return generation_;
}

bool OneWireBusDevice::dependencyTransactionActive() const {
    return dependencyTransactionActive_;
}

void OneWireBusDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool OneWireBusDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oneWireBusDeviceConfigSize(config_);
    return encodeOneWireBusDeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool OneWireBusDevice::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    OneWireBusDeviceConfigV1 config = config_;
    config.base = baseConfig;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oneWireBusDeviceConfigSize(config);
    return encodeOneWireBusDeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan OneWireBusDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    OneWireBusDeviceConfigV1 config{};
    if (!decodeOneWireBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool gpioPinChanged = config.gpioPin != config_.gpioPin;
    const bool internalPullupChanged = config.internalPullup != config_.internalPullup;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = gpioPinChanged || internalPullupChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool OneWireBusDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    OneWireBusDeviceConfigV1 config{};
    if (!decodeOneWireBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

void OneWireBusDevice::writeDeviceJson(JsonObject output) const {
    writeCommonDeviceJson(output);
    writeOneWireBusDeviceConfigJson(config_, output);
    JsonObject scanObject = output.createNestedObject("scan");
    scanObject["in_progress"] = scan_.inProgress;
    scanObject["ready"] = scan_.ready;
    scanObject["deviceCount"] = scan_.deviceCount;
    scanObject["truncated"] = scan_.truncated;
    scanObject["invalidCandidateSeen"] = scan_.invalidCandidateSeen;
    JsonArray devices = scanObject.createNestedArray("devices");
    for (uint8_t index = 0; index < scan_.deviceCount; ++index) {
        JsonObject item = devices.createNestedObject();
        char rom[17]{};
        char family[3]{};
        (void)formatOneWireRomAddress(scan_.devices[index], rom);
        family[0] = "0123456789ABCDEF"[(scan_.devices[index].bytes[0] >> 4) & 0x0F];
        family[1] = "0123456789ABCDEF"[scan_.devices[index].bytes[0] & 0x0F];
        family[2] = '\0';
        item["address"] = rom;
        item["familyCode"] = family;
    }
}

OneWireBusDevice::DependencyTransaction OneWireBusDevice::beginDependencyTransaction() {
    if (status_ != DeviceStatus::Ready || isScanning() || dependencyTransactionActive_ || disableRequested_ || deleteRequested_ ||
        reconfigureRequested_ || config_.base.enabled == 0U) {
        return {};
    }

    dependencyTransactionActive_ = true;
    return DependencyTransaction(this, &driver_, generation_);
}

bool OneWireBusDevice::hasDuplicateDependentRomAddress(const OneWireRomAddress& address, const IDeviceRuntime* ignoreDependent) const {
    for (const IDeviceRuntime* dependent : dependentRuntimes()) {
        if (dependent == nullptr || dependent == ignoreDependent) {
            continue;
        }

        OneWireRomAddress dependentAddress{};
        if (!dependent->oneWireRomAddress(dependentAddress)) {
            continue;
        }
        if (sameRomAddress(address, dependentAddress)) {
            return true;
        }
    }
    return false;
}

DeviceTypeDescriptor OneWireBusDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kOneWireBusDeviceTypeId;
    descriptor.name = "OneWireBusDevice";
    descriptor.currentConfigVersion = kOneWireBusDeviceConfigVersion;
    descriptor.maxDependents = 16;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.createRuntime = &OneWireBusDevice::createRuntime;
    descriptor.validateConfig = &OneWireBusDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> OneWireBusDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new OneWireBusDevice(record, configBlob));
}

DeviceValidationResult OneWireBusDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "onewire bus config exceeds supported size"};
    }
    OneWireBusDeviceConfigV1 config{};
    if (!decodeOneWireBusDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "onewire bus config is invalid"};
    }
    return {};
}

bool OneWireBusDevice::handleCommand(const DeviceCommand& command) {
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
        !config_.base.enabled) {
        return false;
    }

    startScan();
    return true;
}

void OneWireBusDevice::resetScanResult() {
    scan_ = {};
}

bool OneWireBusDevice::hasVisibleScanState() const {
    return scan_.inProgress || scan_.ready || scan_.deviceCount > 0U || scan_.truncated || scan_.invalidCandidateSeen;
}

void OneWireBusDevice::startScan() {
    resetScanResult();
    scan_.inProgress = true;
    scan_.ready = false;
    scan_.truncated = false;
    scan_.invalidCandidateSeen = false;
    driver_.resetSearch();
    markRuntimeStateDirty();
    setState((PState)&OneWireBusDevice::Scanning);
}

void OneWireBusDevice::finishScan() {
    scan_.inProgress = false;
    scan_.ready = true;
    markRuntimeStateDirty();
}

void OneWireBusDevice::appendScanCandidate(const OneWireRomAddress& address) {
    if (!oneWireRomCrcValid(address)) {
        scan_.invalidCandidateSeen = true;
        markRuntimeStateDirty();
        return;
    }

    if (scan_.deviceCount >= kMaxOneWireScanDevices) {
        scan_.truncated = true;
        markRuntimeStateDirty();
        return;
    }

    scan_.devices[scan_.deviceCount] = address;
    ++scan_.deviceCount;
    markRuntimeStateDirty();
}

void OneWireBusDevice::releaseHardware() {
    dependencyTransactionActive_ = false;
    driver_.depower();
}

void OneWireBusDevice::end(uint32_t now) {
    (void)now;
    releaseHardware();
}

void OneWireBusDevice::releaseDependencyTransaction() {
    dependencyTransactionActive_ = false;
}

DeviceValidationResult OneWireBusDevice::initializeHardware(uint32_t now) {
    (void)now;
    if (!driver_.begin(config_.gpioPin, config_.internalPullup != 0U)) {
        return {DeviceError::StorageError, "onewire bus driver initialization failed"};
    }
    ++generation_;
    if (generation_ == 0U) {
        ++generation_;
    }
    return {};
}

bool OneWireBusDevice::isScanning() const {
    return scan_.inProgress;
}

SM_STATE(OneWireBusDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        resetScanResult();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.base.enabled) {
        resetScanResult();
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

SM_STATE(OneWireBusDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (hasVisibleScanState()) {
            markRuntimeStateDirty();
        }
        resetScanResult();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.base.enabled) {
        if (hasVisibleScanState()) {
            markRuntimeStateDirty();
        }
        resetScanResult();
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

SM_STATE(OneWireBusDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (hasVisibleScanState()) {
            markRuntimeStateDirty();
        }
        resetScanResult();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.base.enabled) {
        if (hasVisibleScanState()) {
            markRuntimeStateDirty();
        }
        resetScanResult();
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

SM_STATE(OneWireBusDevice::Scanning) {
    status_ = DeviceStatus::Ready;
    if (!scan_.inProgress) {
        SM_GOTO(Ready);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (hasVisibleScanState()) {
            markRuntimeStateDirty();
        }
        resetScanResult();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !config_.base.enabled) {
        if (hasVisibleScanState()) {
            markRuntimeStateDirty();
        }
        resetScanResult();
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        if (hasVisibleScanState()) {
            markRuntimeStateDirty();
        }
        resetScanResult();
        releaseHardware();
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }

    OneWireRomAddress address{};
    if (driver_.search(address)) {
        appendScanCandidate(address);
        return;
    }

    finishScan();
    SM_GOTO(Ready);
}

SM_STATE(OneWireBusDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    if (hasVisibleScanState()) {
        markRuntimeStateDirty();
    }
    resetScanResult();
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

SM_STATE(OneWireBusDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    resetScanResult();
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

SM_STATE(OneWireBusDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    resetScanResult();
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

SM_STATE(OneWireBusDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    resetScanResult();
    releaseHardware();
    setDeleted();
}

SM_STATE(OneWireBusDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        resetScanResult();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_) {
        resetScanResult();
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
