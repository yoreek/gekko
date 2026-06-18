#include "devices/bus/onewire/OneWireBusDevice.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS OneWireBusDevice

namespace {
constexpr DeviceTypeId kOneWireBusDeviceTypeId = 3;
constexpr uint32_t kOneWireBusDeviceConfigVersion = 1;
} // namespace

static_assert(std::is_trivially_copyable<OneWireBusDeviceConfigV1>::value, "OneWireBusDeviceConfigV1 must be POD");
static_assert(sizeof(OneWireBusDeviceConfigV1::kMagicKey) + sizeof(OneWireBusDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "OneWireBusDeviceConfigV1 exceeds device config bound");

OneWireBusDevice::OneWireBusDevice(const DeviceRecord& record)
    : OneWireBusDevice(
          [&record]() {
              OneWireBusDeviceConfigV1 config{};
              (void)decodeOneWireBusDeviceConfig(record.configPayload, config);
              config.enabled = record.enabled ? 1U : 0U;
              return config;
          }(),
          defaultArduinoOneWireBusDriver()) {}

OneWireBusDevice::OneWireBusDevice(const OneWireBusDeviceConfigV1& config, IOneWireBusDriver& driver)
    : DeviceRuntimeBase((PState)&OneWireBusDevice::Idle), config_(config), driver_(driver) {}

const OneWireBusDeviceConfigV1& OneWireBusDevice::config() const {
    return config_;
}

const OneWireScanResult& OneWireBusDevice::scan() const {
    return scan_;
}

DeviceTypeDescriptor OneWireBusDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kOneWireBusDeviceTypeId;
    descriptor.name = "OneWireBusDevice";
    descriptor.currentConfigVersion = kOneWireBusDeviceConfigVersion;
    descriptor.canHaveChildren = true;
    descriptor.maxChildren = 16;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.createRuntime = &OneWireBusDevice::createRuntime;
    descriptor.validateConfig = &OneWireBusDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> OneWireBusDevice::createRuntime(const DeviceRecord& record) {
    return std::unique_ptr<IDeviceRuntime>(new OneWireBusDevice(record));
}

DeviceValidationResult OneWireBusDevice::validateConfig(const DeviceRecord& record) {
    if (record.configPayload.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "onewire bus config exceeds supported size"};
    }
    OneWireBusDeviceConfigV1 config{};
    if (!decodeOneWireBusDeviceConfig(record.configPayload, config)) {
        return {DeviceError::InvalidConfig, "onewire bus config is invalid"};
    }
    return {};
}

bool OneWireBusDevice::handleCommand(const DeviceCommand& command) {
    if (command.type != DeviceCommandType::Custom) {
        return false;
    }
    if (!command.payload.equals("scan")) {
        return false;
    }
    if (status_ != DeviceStatus::Ready || isScanning() || disableRequested_ || deleteRequested_ || reconfigureRequested_ ||
        !config_.enabled) {
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
    if (!oneWireRomCrcValid(driver_, address)) {
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
    driver_.depower();
}

DeviceValidationResult OneWireBusDevice::initializeHardware(uint32_t now) {
    (void)now;
    if (!driver_.begin(config_.gpioPin, config_.internalPullup != 0U)) {
        return {DeviceError::StorageError, "onewire bus driver initialization failed"};
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
    if (disableRequested_ || !config_.enabled) {
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
    if (!parentReady()) {
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
    if (disableRequested_ || !config_.enabled) {
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
    if (!parentReady()) {
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
    if (disableRequested_ || !config_.enabled) {
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
    if (disableRequested_ || !config_.enabled) {
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
    if (!parentReady()) {
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
        if (parentReady()) {
            status_ = DeviceStatus::Reconfiguring;
            SM_GOTO(Reconfiguring);
        }
    }
}

} // namespace ewfm
