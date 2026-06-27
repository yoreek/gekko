#pragma once

#include "devices/sensors/temperature/TemperatureSensorTypes.h"
#include "devices/switch/OutputState.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ewfm {

using DeviceId = uint32_t;
using DeviceTypeId = uint16_t;
using DeviceRevision = uint32_t;

struct OneWireRomAddress;
class DeviceRetainedDataStore;
class DeviceScopedDataStore;

constexpr uint32_t kDeviceRegistrySchemaVersion = 1;
constexpr uint16_t kDeviceRegistryIndexVersion = 2;
constexpr uint16_t kDeviceRecordHeaderVersion = 4;
constexpr uint16_t kRetainedStateRecordVersion = 2;
constexpr size_t kMaxDynamicDevices = 200;
constexpr size_t kMaxDynamicDeviceNameLength = 32;
constexpr size_t kMaxDeviceBaseNameLength = kMaxDynamicDeviceNameLength;
constexpr size_t kMaxDeviceConfigBytes = 512;
constexpr size_t kMaxRetainedStateBytes = 64;
constexpr size_t kMaxDeviceEventBytes = 256;
constexpr size_t kMaxDeviceEventKindBytes = 32;
constexpr size_t kMaxRegistryIndexBytes = 2048;
constexpr size_t kMaxDeviceRecordBytes = 1024;
constexpr size_t kMaxDeviceIdGenerationAttempts = 8;
constexpr size_t kMaxDeviceDependencies = 4;

enum class DeviceDependencyRole : uint8_t {
    Unknown = 0,
    OneWireBus = 1,
    TemperatureSensor = 2,
    Switch = 3,
    I2CBus = 4,
    OledDisplay = 5,
    SpiBus = 6,
};

struct DeviceDependencyLink {
    DeviceDependencyRole role{DeviceDependencyRole::Unknown};
    DeviceId deviceId{0};
};

enum class DeviceStatus : uint8_t;
struct DeviceValidationResult;

enum class DeviceStatus : uint8_t {
    Unknown = 0,
    Creating = 1,
    Starting = 2,
    Ready = 3,
    Disabled = 4,
    Faulted = 5,
    DependencyBlocked = 6,
    Reconfiguring = 7,
    Stopping = 8,
    Deleting = 9,
};

struct DeviceRecordBase {
    DeviceId id{0};
    const char* typeName{nullptr};
    uint32_t configRevision{0};
};

struct DeviceRuntimeSnapshotBase {
    DeviceStatus status{DeviceStatus::Unknown};
    DeviceStatus effectiveStatus{DeviceStatus::Unknown};
};

template <typename TConfig, typename TRuntime> struct DeviceApiRecord {
    DeviceRecordBase record{};
    TConfig config{};
    TRuntime runtime{};
};

template <typename TConfig> struct DeviceSetupRecord {
    DeviceRecordBase record{};
    TConfig config{};
};

struct DeviceDependencyRequirement {
    DeviceDependencyRole role{DeviceDependencyRole::Unknown};
    bool required{false};
    std::vector<DeviceTypeId> compatibleTypeIds{};
};

const char* deviceDependencyRoleName(DeviceDependencyRole role);
bool parseDeviceDependencyRole(std::string_view value, DeviceDependencyRole& role);

class ITemperatureReadingRuntime {
public:
    ITemperatureReadingRuntime() = default;
    ITemperatureReadingRuntime(const ITemperatureReadingRuntime&) = delete;
    ITemperatureReadingRuntime& operator=(const ITemperatureReadingRuntime&) = delete;
    ITemperatureReadingRuntime(ITemperatureReadingRuntime&&) = delete;
    ITemperatureReadingRuntime& operator=(ITemperatureReadingRuntime&&) = delete;
    virtual ~ITemperatureReadingRuntime() = default;

    virtual bool latestTemperatureReading(TemperatureReading& reading) const = 0;
    virtual const char* latestTemperatureStatus() const = 0;
};

class ISwitchOutputRuntime {
public:
    ISwitchOutputRuntime() = default;
    ISwitchOutputRuntime(const ISwitchOutputRuntime&) = delete;
    ISwitchOutputRuntime& operator=(const ISwitchOutputRuntime&) = delete;
    ISwitchOutputRuntime(ISwitchOutputRuntime&&) = delete;
    ISwitchOutputRuntime& operator=(ISwitchOutputRuntime&&) = delete;
    virtual ~ISwitchOutputRuntime() = default;

    virtual OutputStateMask supportedOutputStateMask() const = 0;
    virtual OutputState currentOutputState() const = 0;
    virtual bool requestOutputState(OutputState state, uint32_t now) = 0;
};

class IDevicePersistedState {
public:
    IDevicePersistedState() = default;
    IDevicePersistedState(const IDevicePersistedState&) = delete;
    IDevicePersistedState& operator=(const IDevicePersistedState&) = delete;
    IDevicePersistedState(IDevicePersistedState&&) = delete;
    IDevicePersistedState& operator=(IDevicePersistedState&&) = delete;
    virtual ~IDevicePersistedState() = default;

    virtual DeviceValidationResult loadPersistedState(DeviceScopedDataStore& store) = 0;
    virtual DeviceValidationResult savePersistedState(DeviceScopedDataStore& store) const = 0;
    virtual DeviceValidationResult clearPersistedState(DeviceScopedDataStore& store) = 0;
    virtual DeviceValidationResult applyPersistedStateUpdate(const uint8_t* data, size_t size) = 0;
};

template <size_t kCapacity> struct BoundedText {
    bool assign(std::string_view value) {
        if (value.size() > kCapacity) {
            clear();
            overflow_ = true;
            return false;
        }
        if (!value.empty()) {
            std::memcpy(data_.data(), value.data(), value.size());
        }
        length_ = value.size();
        data_[length_] = '\0';
        overflow_ = false;
        return true;
    }

    bool assign(const char* value) {
        return assign(value == nullptr ? std::string_view{} : std::string_view(value));
    }

    void clear() {
        length_ = 0;
        overflow_ = false;
        data_[0] = '\0';
    }

    bool valid() const {
        return !overflow_;
    }

    bool empty() const {
        return length_ == 0;
    }

    const char* c_str() const {
        return data_.data();
    }

    std::string_view view() const {
        return std::string_view(data_.data(), length_);
    }

    bool equals(const char* value) const {
        return view() == (value == nullptr ? std::string_view{} : std::string_view(value));
    }

private:
    std::array<char, kCapacity + 1> data_{};
    size_t length_{0};
    bool overflow_{false};
};

template <size_t kCapacity> struct BoundedBlob {
    BoundedBlob() = default;

    bool assign(const uint8_t* value, size_t size) {
        if (size > kCapacity) {
            clear();
            overflow_ = true;
            return false;
        }
        if (value != nullptr && size != 0U) {
            std::memcpy(data_.data(), value, size);
        }
        length_ = size;
        overflow_ = false;
        return true;
    }

    bool assign(const std::vector<uint8_t>& value) {
        return assign(value.data(), value.size());
    }

    void clear() {
        length_ = 0;
        overflow_ = false;
        if (!data_.empty()) {
            data_[0] = 0U;
        }
    }

    bool valid() const {
        return !overflow_;
    }

    bool empty() const {
        return length_ == 0;
    }

    bool setSize(size_t size) {
        if (size > kCapacity) {
            clear();
            overflow_ = true;
            return false;
        }
        length_ = size;
        overflow_ = false;
        return true;
    }

    const uint8_t* data() const {
        return data_.data();
    }

    uint8_t* data() {
        return data_.data();
    }

    size_t size() const {
        return length_;
    }

private:
    std::array<uint8_t, kCapacity> data_{};
    size_t length_{0};
    bool overflow_{false};
};

using DeviceConfigBlob = BoundedBlob<kMaxDeviceConfigBytes>;

enum class DeviceCommandType : uint8_t {
    None = 0,
    Create = 1,
    UpdateConfig = 2,
    Rename = 3,
    Enable = 4,
    Disable = 5,
    Delete = 6,
    SetStatus = 7,
    Scan = 8,
    SetOutput = 9,
    Custom = 10,
    SetDeps = 11,
};

enum class DevicePersistencePolicy : uint8_t {
    Immediate = 0,
    Delayed = 1,
    Coalesced = 2,
};

enum class DeviceCadence : uint8_t {
    FastLoop = 0,
    Tick100ms = 1,
    Tick1s = 2,
};

enum class DeviceError {
    None = 0,
    StorageError,
    InvalidDeviceId,
    DuplicateDeviceId,
    UnsupportedType,
    InvalidVersion,
    CorruptRecord,
    BoundsExceeded,
    InvalidConfig,
    InvalidRelationship,
    MissingRecord,
    InvalidCommand,
};

struct DeviceValidationResult {
    DeviceError error{DeviceError::None};
    const char* message{"ok"};

    bool ok() const {
        return error == DeviceError::None;
    }
};

struct DeviceRecordHeader {
    uint16_t recordVersion{kDeviceRecordHeaderVersion};
    DeviceId deviceId{0};
    DeviceTypeId typeId{0};
    uint32_t configVersion{0};
    uint32_t configRevision{0};
    uint32_t payloadLength{0};
    uint32_t payloadChecksum{0};
};

enum class DeviceEventKind : uint8_t {
    RegistryLoaded = 0,
    DeviceCreated = 1,
    DeviceUpdated = 2,
    DeviceDeleted = 3,
    StatusChanged = 4,
    StateChanged = 5,
    CommandAccepted = 6,
    CommandRejected = 7,
    ConfigPersisted = 8,
    RetainedStateChanged = 9,
    PersistencePendingCleared = 10,
};

const char* deviceEventKindName(DeviceEventKind kind);

struct DeviceIndexEntry {
    DeviceId deviceId{0};
    DeviceTypeId typeId{0};
};

#pragma pack(push, 1)
struct DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "BASE-1";
    uint8_t enabled{1};
    char name[kMaxDeviceBaseNameLength + 1]{};

    DeviceValidationResult validate() const;
    bool parseJson(const JsonObjectConst& input, const char*& error);
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

struct DeviceRegistryEntry {
    DeviceRecordHeader header{};
    std::array<DeviceDependencyLink, kMaxDeviceDependencies> deps{};
    uint8_t depCount{0};
    DevicePersistencePolicy persistencePolicy{DevicePersistencePolicy::Delayed};
    DeviceStatus status{DeviceStatus::Unknown};

    bool hasDeps() const {
        return dependencyCount() > 0;
    }

    const DeviceDependencyLink* dependencyLinks() const {
        return deps.data();
    }

    DeviceDependencyLink* dependencyLinks() {
        return deps.data();
    }

    const DeviceDependencyLink* dependencyLink(DeviceDependencyRole role) const {
        const DeviceDependencyLink* links = dependencyLinks();
        const uint8_t count = dependencyCount();
        for (uint8_t index = 0; index < count; ++index) {
            if (links[index].role == role) {
                return &links[index];
            }
        }
        return nullptr;
    }

    DeviceId dependencyDeviceId(DeviceDependencyRole role) const {
        const DeviceDependencyLink* link = dependencyLink(role);
        return link != nullptr ? link->deviceId : 0;
    }

    uint8_t dependencyCount() const {
        return depCount;
    }
};

struct DeviceRegistrySnapshot {
    uint32_t schemaVersion{kDeviceRegistrySchemaVersion};
    std::vector<DeviceIndexEntry> indexEntries{};
    std::vector<DeviceRegistryEntry> records{};
};

struct SwitchRetainedStateRecord {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0};
    OutputState outputState{OutputState::Off};
};

struct DeviceCommand {
    struct DepsPayload {
        std::array<DeviceDependencyLink, kMaxDeviceDependencies> deps{};
        uint8_t depCount{0};
    };

    DeviceCommandType type{DeviceCommandType::None};
    DeviceId deviceId{0};
    BoundedText<kMaxDeviceEventBytes> payload{};
    DepsPayload depsPayload{};
    bool depsPayloadValid{false};
    DevicePersistencePolicy persistencePolicy{DevicePersistencePolicy::Delayed};

    DeviceCommand() = default;

    DeviceCommand(DeviceCommandType commandType, DeviceId commandDeviceId, std::string_view commandPayload,
                  DevicePersistencePolicy commandPolicy = DevicePersistencePolicy::Delayed)
        : type(commandType), deviceId(commandDeviceId), persistencePolicy(commandPolicy) {
        (void)payload.assign(commandPayload);
    }

    DeviceCommand(DeviceCommandType commandType, DeviceId commandDeviceId, DepsPayload commandDepsPayload,
                  DevicePersistencePolicy commandPolicy = DevicePersistencePolicy::Delayed)
        : type(commandType), deviceId(commandDeviceId), depsPayload(commandDepsPayload), depsPayloadValid(true),
          persistencePolicy(commandPolicy) {}

    bool valid() const {
        if (type == DeviceCommandType::SetDeps) {
            return depsPayloadValid;
        }
        return payload.valid();
    }
};

struct DeviceEvent {
    DeviceEventKind kind{DeviceEventKind::RegistryLoaded};
    BoundedText<kMaxDeviceEventKindBytes> eventKind{};
    uint32_t registryRevision{0};
    uint32_t configRevision{0};
    DeviceId deviceId{0};
    DeviceTypeId typeId{0};
    BoundedText<kMaxDynamicDeviceNameLength> name{};
    BoundedText<kMaxDynamicDeviceNameLength> typeName{};
    DeviceStatus previousStatus{DeviceStatus::Unknown};
    DeviceStatus status{DeviceStatus::Unknown};
    bool pendingPersistence{false};
    bool commandAccepted{false};
    BoundedText<kMaxDeviceEventBytes> detail{};
};

struct DeviceConfigUpdatePlan {
    bool endOldConfig{false};
    bool resetStateMachine{false};
};

class IDeviceRuntime {
public:
    IDeviceRuntime() = default;
    IDeviceRuntime(const IDeviceRuntime&) = delete;
    IDeviceRuntime& operator=(const IDeviceRuntime&) = delete;
    IDeviceRuntime(IDeviceRuntime&&) = delete;
    IDeviceRuntime& operator=(IDeviceRuntime&&) = delete;
    virtual ~IDeviceRuntime() = default;

    virtual void begin(uint32_t now) = 0;
    virtual void tickFastLoop(uint32_t now) = 0;
    virtual void tick100ms(uint32_t now) = 0;
    virtual void tick1s(uint32_t now) = 0;
    virtual void end(uint32_t now) {
        (void)now;
    }
    virtual DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const {
        (void)configBlob;
        return {};
    }
    virtual bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
        (void)configBlob;
        (void)now;
        return false;
    }
    virtual void resetStateMachine(uint32_t now) {
        (void)now;
    }
    virtual void setDependencyRuntime(DeviceDependencyRole role, IDeviceRuntime* dependencyRuntime) {
        (void)role;
        (void)dependencyRuntime;
    }
    virtual IDeviceRuntime* dependencyRuntime(DeviceDependencyRole role) const {
        (void)role;
        return nullptr;
    }
    virtual const ITemperatureReadingRuntime* temperatureReadingRuntime() const {
        return nullptr;
    }
    virtual const ISwitchOutputRuntime* switchOutputRuntime() const {
        return nullptr;
    }
    virtual IDevicePersistedState* persistedStateRuntime() {
        return nullptr;
    }
    virtual const IDevicePersistedState* persistedStateRuntime() const {
        return nullptr;
    }
    virtual uint8_t dependencyCount() const {
        return 0;
    }
    virtual const DeviceDependencyLink* dependencyLinks() const {
        return nullptr;
    }
    virtual void attachDependentRuntime(IDeviceRuntime* dependentRuntime) {
        (void)dependentRuntime;
    }
    virtual void detachDependentRuntime(IDeviceRuntime* dependentRuntime) {
        (void)dependentRuntime;
    }
    virtual const std::vector<IDeviceRuntime*>& dependentRuntimes() const {
        static const std::vector<IDeviceRuntime*> kEmptyDependents;
        return kEmptyDependents;
    }
    virtual void requestReconfigure() = 0;
    virtual void requestDisable() = 0;
    virtual void requestDelete() = 0;
    virtual void clearLifecycleRequests() {}
    virtual DeviceStatus status() const = 0;
    virtual void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
        (void)record;
        (void)config;
    }
    virtual DeviceId deviceId() const {
        return 0;
    }
    virtual DeviceTypeId typeId() const {
        return 0;
    }
    virtual uint32_t configVersion() const {
        return 0;
    }
    virtual uint32_t configRevision() const {
        return 0;
    }
    virtual bool hasDependencies() const {
        return false;
    }
    virtual DeviceId dependencyDeviceId(DeviceDependencyRole role) const {
        (void)role;
        return 0;
    }
    virtual bool enabled() const {
        return true;
    }
    virtual const char* name() const {
        return nullptr;
    }
    virtual DevicePersistencePolicy persistencePolicy() const {
        return DevicePersistencePolicy::Delayed;
    }
    virtual bool handleCommand(const DeviceCommand& command) = 0;
    virtual bool retainedStateDirty() const {
        return false;
    }
    virtual bool runtimeStateDirty() const {
        return false;
    }
    virtual bool serializeConfigBlob(DeviceConfigBlob& configBlob) const {
        (void)configBlob;
        return false;
    }
    virtual void writeDeviceJson(JsonObject output) const {
        (void)output;
    }
    virtual bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
        (void)configBlob;
        (void)baseConfig;
        return false;
    }
    virtual DeviceValidationResult saveRetainedState(DeviceRetainedDataStore& store) const {
        (void)store;
        return {DeviceError::InvalidConfig, "device type does not support retained state"};
    }
    virtual DeviceValidationResult loadRetainedState(DeviceRetainedDataStore& store) {
        (void)store;
        return {DeviceError::InvalidConfig, "device type does not support retained state"};
    }
    virtual void clearRetainedStateDirty() {}
    virtual void clearRuntimeStateDirty() {}
    virtual bool oneWireRomAddress(OneWireRomAddress& address) const {
        (void)address;
        return false;
    }
    virtual bool hasDuplicateDependentRomAddress(const OneWireRomAddress& address, const IDeviceRuntime* ignoreDependent = nullptr) const {
        (void)address;
        (void)ignoreDependent;
        return false;
    }
    virtual bool i2cAddress(uint8_t& address) const {
        (void)address;
        return false;
    }
    virtual bool hasDuplicateDependentI2cAddress(uint8_t address, const IDeviceRuntime* ignoreDependent = nullptr) const {
        (void)address;
        (void)ignoreDependent;
        return false;
    }
    virtual bool spiChipSelectPin(uint8_t& pin) const {
        (void)pin;
        return false;
    }
    virtual bool hasDuplicateDependentSpiChipSelect(uint8_t pin, const IDeviceRuntime* ignoreDependent = nullptr) const {
        (void)pin;
        (void)ignoreDependent;
        return false;
    }
};

class DeviceTypeDescriptor {
public:
    using RuntimeFactory = std::unique_ptr<IDeviceRuntime> (*)(const DeviceRegistryEntry&, const DeviceConfigBlob&);
    using ConfigValidator = DeviceValidationResult (*)(const DeviceRegistryEntry&, const DeviceConfigBlob&);

    DeviceTypeId typeId{0};
    const char* name{nullptr};
    uint32_t currentConfigVersion{1};
    uint8_t maxDependents{0};
    bool supportsCommands{false};
    bool supportsRetainedState{false};
    DevicePersistencePolicy defaultPersistencePolicy{DevicePersistencePolicy::Delayed};
    bool ticksFastLoop{false};
    bool ticks100ms{false};
    bool ticks1s{false};
    std::vector<DeviceDependencyRequirement> dependencyRequirements{};
#ifdef UNIT_TEST
#endif
    RuntimeFactory createRuntime{nullptr};
    ConfigValidator validateConfig{nullptr};
};

class DeviceTypeRegistry {
public:
    bool registerDescriptor(const DeviceTypeDescriptor& descriptor);
    const DeviceTypeDescriptor* find(DeviceTypeId typeId) const;
    const std::vector<DeviceTypeDescriptor>& descriptors() const {
        return descriptors_;
    }

    static DeviceTypeRegistry withDefaults();

private:
    std::vector<DeviceTypeDescriptor> descriptors_{};
};

inline bool deviceIdIsReserved(DeviceId id) {
    return id == 0;
}

} // namespace ewfm
