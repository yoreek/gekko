#pragma once

#include "devices/sensors/humidity/HumiditySensorTypes.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

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
class DisplayDeviceBase;
class CharacterDisplayRuntimeBase;

constexpr uint32_t kDeviceRegistrySchemaVersion = 1;
constexpr uint16_t kDeviceRegistryIndexVersion = 2;
constexpr uint16_t kDeviceRecordHeaderVersion = 4;
constexpr uint16_t kRetainedStateRecordVersion = 2;
constexpr size_t kMaxDynamicDevices = 200;
constexpr size_t kMaxDynamicDeviceNameLength = 32;
constexpr size_t kMaxDeviceBaseNameLength = kMaxDynamicDeviceNameLength;
constexpr size_t kMaxDeviceConfigBytes = 512;
constexpr size_t kMaxDisplayLayoutBytes = 4096;
constexpr size_t kMaxRetainedStateBytes = 64;
constexpr size_t kMaxDeviceEventBytes = 256;
constexpr size_t kMaxDeviceEventKindBytes = 32;
constexpr size_t kMaxRegistryIndexBytes = 2048;
constexpr size_t kMaxDeviceRecordBytes = 1024;
constexpr size_t kMaxDeviceIdGenerationAttempts = 8;
constexpr size_t kMaxDeviceDependencies = 16;
constexpr size_t kMaxProvidedRoles = 3;
constexpr uint16_t kAnalogOutputLevelMax = 4095U;
constexpr uint16_t kAnalogInputResolutionMax = 4095U;

enum class DeviceRole : uint8_t {
    Unknown = 0,
    OneWireBus = 1,
    TemperatureSensor = 2,
    Switch = 3,
    I2CBus = 4,
    Ssd1306 = 5,
    SpiBus = 6,
    MetricSource = 7,
    RealTimeClock = 8,
    PortExpander = 9,
    Schedule = 10,
    Condition = 11,
    AnalogOutput = 12,
    AnalogOutputGroup = 13,
    AnalogInput = 14,
    AnalogInputHub = 15,
};

struct DeviceDependencyLink {
    DeviceRole role{DeviceRole::Unknown};
    DeviceId deviceId{0};
    // A per-link "should this source's status be inverted" flag, used by Condition-role links
    // (see IStatusRuntime below). Stored in a wire byte that was previously reserved-and-discarded,
    // so this does not change the on-flash record size or require a version bump.
    bool invert{false};
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

// A requirement only names the role it needs; which concrete types can satisfy that role is
// never enumerated here. Each type that can provide a role declares it once, on itself, via the
// corresponding I*Runtime interface's `kProvidedRole` (see below) -- the validator compares
// `DeviceTypeDescriptor::providedRole` against `role` directly instead of matching a hand-kept
// type-id list.
struct DeviceDependencyRequirement {
    DeviceRole role{DeviceRole::Unknown};
    bool required{false};
};

const char* deviceRoleName(DeviceRole role);
bool parseDeviceRole(std::string_view value, DeviceRole& role);

// Roles that are inherently optional dependencies wherever they appear -- they represent a
// soft/reference-only link to another device (e.g. a display placeholder's metric source), not a
// functional requirement, so a Disabled/Faulted/etc. status on the target must never cascade into
// the dependent device's own status or block it from becoming Ready. Add new roles here as more
// reference-only dependency kinds are introduced; no per-descriptor opt-in is needed.
inline bool isGloballyOptionalDependencyRole(DeviceRole role) {
    return role == DeviceRole::MetricSource;
}

// The bounded set of roles a device type provides to other devices' dependencies. Most types
// provide exactly one role (mirrors the single `kProvidedRole` each I*Runtime interface declares);
// a handful of "dual-purpose" types (e.g. a switch that can also serve as an AND-condition source)
// provide two. Capped at kMaxProvidedRoles with no heap allocation, since DeviceTypeDescriptor is a
// small in-memory value built once per type at boot.
struct ProvidedRoles {
    std::array<DeviceRole, kMaxProvidedRoles> roles{};
    uint8_t count{0};

    static ProvidedRoles of(std::initializer_list<DeviceRole> list) {
        ProvidedRoles result;
        for (DeviceRole role : list) {
            if (result.count >= kMaxProvidedRoles) {
                break;
            }
            result.roles[result.count++] = role;
        }
        return result;
    }

    bool contains(DeviceRole role) const {
        for (uint8_t index = 0; index < count; ++index) {
            if (roles[index] == role) {
                return true;
            }
        }
        return false;
    }
};

// Role-marker interfaces: implementing one of these is what makes a device type a provider of
// the corresponding DeviceRole. `descriptor()` for such a type includes
// `I*Runtime::kProvidedRole` in its `providedRoles` set -- the role constant lives on the
// interface itself, nowhere else, so every implementor is guaranteed to agree with any other
// implementor of the same interface.
class ITemperatureReadingRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::TemperatureSensor;

    ITemperatureReadingRuntime() = default;
    ITemperatureReadingRuntime(const ITemperatureReadingRuntime&) = delete;
    ITemperatureReadingRuntime& operator=(const ITemperatureReadingRuntime&) = delete;
    ITemperatureReadingRuntime(ITemperatureReadingRuntime&&) = delete;
    ITemperatureReadingRuntime& operator=(ITemperatureReadingRuntime&&) = delete;
    virtual ~ITemperatureReadingRuntime() = default;

    virtual bool latestTemperatureReading(TemperatureReading& reading) const = 0;
    virtual const char* latestTemperatureStatus() const = 0;
};

// Deliberately role-less: humidity is consumed only through the metric pipeline (displays,
// metric catalog), which feature-detects `humidityReadingRuntime()` on the runtime. No device
// depends on humidity by role today, so declaring a DeviceRole for it would only widen the
// dependency validator surface for nothing. Add a kProvidedRole here if a by-role consumer
// (e.g. a humidistat) ever appears.
class IHumidityReadingRuntime {
public:
    IHumidityReadingRuntime() = default;
    IHumidityReadingRuntime(const IHumidityReadingRuntime&) = delete;
    IHumidityReadingRuntime& operator=(const IHumidityReadingRuntime&) = delete;
    IHumidityReadingRuntime(IHumidityReadingRuntime&&) = delete;
    IHumidityReadingRuntime& operator=(IHumidityReadingRuntime&&) = delete;
    virtual ~IHumidityReadingRuntime() = default;

    virtual bool latestHumidityReading(HumidityReading& reading) const = 0;
    virtual const char* latestHumidityStatus() const = 0;
};

template <typename ValueType> class IOutputRuntime {
public:
    using StateType = ValueType;

    IOutputRuntime() = default;
    IOutputRuntime(const IOutputRuntime&) = delete;
    IOutputRuntime& operator=(const IOutputRuntime&) = delete;
    IOutputRuntime(IOutputRuntime&&) = delete;
    IOutputRuntime& operator=(IOutputRuntime&&) = delete;
    virtual ~IOutputRuntime() = default;

    virtual StateType currentOutputState() const = 0;
    virtual bool requestOutputState(StateType state, uint32_t now) = 0;
};

class ISwitchOutputRuntime : public IOutputRuntime<bool> {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::Switch;
};

class IAnalogOutputRuntime : public IOutputRuntime<uint16_t> {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::AnalogOutput;
};

enum class AnalogOutputMode : uint8_t {
    Manual = 0,
    Scheduled = 1,
    Off = 2,
};

const char* analogOutputModeName(AnalogOutputMode mode);

class IScheduledAnalogOutputRuntime {
public:
    IScheduledAnalogOutputRuntime() = default;
    IScheduledAnalogOutputRuntime(const IScheduledAnalogOutputRuntime&) = delete;
    IScheduledAnalogOutputRuntime& operator=(const IScheduledAnalogOutputRuntime&) = delete;
    virtual ~IScheduledAnalogOutputRuntime() = default;

    virtual AnalogOutputMode analogOutputMode() const = 0;
    virtual bool requestAnalogOutputMode(AnalogOutputMode mode, uint32_t now) = 0;
    virtual uint16_t requestedAnalogOutputState() const = 0;
    virtual bool analogOutputTimeValid() const = 0;
};

class IAnalogOutputGroupRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::AnalogOutputGroup;

    IAnalogOutputGroupRuntime() = default;
    IAnalogOutputGroupRuntime(const IAnalogOutputGroupRuntime&) = delete;
    IAnalogOutputGroupRuntime& operator=(const IAnalogOutputGroupRuntime&) = delete;
    virtual ~IAnalogOutputGroupRuntime() = default;

    virtual AnalogOutputMode analogOutputGroupMode() const = 0;
    virtual bool requestAnalogOutputGroupMode(AnalogOutputMode mode, uint32_t now) = 0;
};

// One physical or logical analog input reading, normalized the same way as analog output: a
// `uint16_t` code in `0..kAnalogInputResolutionMax` alongside the physical value the code was
// derived from. `milliVolts` is the authoritative value consumers should compute from (e.g. a
// resistive-divider sensor); `rawCode` exists for UI/diagnostics and is only a best-effort
// normalization of whatever native resolution the backend ADC actually has.
struct AnalogInputReading {
    uint16_t rawCode{0};
    int32_t milliVolts{0};
    uint32_t measuredAtMs{0};
    bool valid{false};
};

// A source of one analog reading: the ESP32's own ADC on a pin, or one channel of an
// AnalogInputHub (ADS1115, CD74HC4067, ...). Consumers (e.g. an NTC thermistor sensor) depend on
// this role without caring which concrete backend produced the reading.
class IAnalogInputRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::AnalogInput;

    IAnalogInputRuntime() = default;
    IAnalogInputRuntime(const IAnalogInputRuntime&) = delete;
    IAnalogInputRuntime& operator=(const IAnalogInputRuntime&) = delete;
    IAnalogInputRuntime(IAnalogInputRuntime&&) = delete;
    IAnalogInputRuntime& operator=(IAnalogInputRuntime&&) = delete;
    virtual ~IAnalogInputRuntime() = default;

    virtual bool latestAnalogInputReading(AnalogInputReading& reading) const = 0;
    virtual const char* latestAnalogInputStatus() const = 0;
};

enum class AnalogInputHubPollResult : uint8_t {
    Busy = 0,
    Pending = 1,
    Ready = 2,
    Fault = 3,
};

// A multi-channel analog source (ADS1115, CD74HC4067) shared by several AnalogInput-role leaf
// devices, one per physical channel. Only one (channel, requester) pair can be in flight at a
// time -- the hub arbitrates instead of blocking, so a channel leaf polls this every tick until it
// gets Ready/Fault instead of the hub ever calling delay(). `requester` is the calling leaf's own
// DeviceId, used to tell "my request is still being served" apart from "someone else has the hub
// busy right now".
class IAnalogInputHubRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::AnalogInputHub;

    IAnalogInputHubRuntime() = default;
    IAnalogInputHubRuntime(const IAnalogInputHubRuntime&) = delete;
    IAnalogInputHubRuntime& operator=(const IAnalogInputHubRuntime&) = delete;
    IAnalogInputHubRuntime(IAnalogInputHubRuntime&&) = delete;
    IAnalogInputHubRuntime& operator=(IAnalogInputHubRuntime&&) = delete;
    virtual ~IAnalogInputHubRuntime() = default;

    virtual uint8_t channelCount() const = 0;
    // Bumped whenever the hub itself is reconfigured (address/gain/pins), so a dependent channel
    // leaf can tell its in-flight request was invalidated out from under it.
    virtual uint32_t generation() const = 0;
    virtual AnalogInputHubPollResult pollChannelReading(uint8_t channel, DeviceId requester, uint32_t now, AnalogInputReading& outReading,
                                                        const char*& outStatus) = 0;
    // Releases a channel's claim on the hub (disable/delete/reconfigure of the requesting leaf), so
    // a stuck in-flight request can't wedge the hub against every other channel forever.
    virtual void releaseChannelRequest(uint8_t channel, DeviceId requester) = 0;
};

// A hardware RTC (e.g. DS3231) that can be read for its current time, written to keep it aligned
// with a more trusted time source, and opted in/out as the system time synchronizer via
// `useForSystemTimeSync()`. See RtcSyncCoordinator for how this role feeds NtpManager.
class IRealTimeClockRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::RealTimeClock;

    IRealTimeClockRuntime() = default;
    IRealTimeClockRuntime(const IRealTimeClockRuntime&) = delete;
    IRealTimeClockRuntime& operator=(const IRealTimeClockRuntime&) = delete;
    IRealTimeClockRuntime(IRealTimeClockRuntime&&) = delete;
    IRealTimeClockRuntime& operator=(IRealTimeClockRuntime&&) = delete;
    virtual ~IRealTimeClockRuntime() = default;

    virtual bool latestTimeReading(uint32_t& outUtcEpoch, bool& outLostPower) const = 0;
    virtual bool writeTime(uint32_t utcEpoch) = 0;
    virtual bool useForSystemTimeSync() const = 0;
};

// A multi-channel I2C GPIO expander (PCF8574/PCF8575/...) that tracks channel on/off state in
// memory and exposes it to per-channel switch devices depending on it via DeviceRole::PortExpander.
// The chip itself is treated as write-only: state is periodically re-pushed rather than read back,
// because these parts are known to occasionally lose their output state (e.g. on brownout).
class IPortExpanderRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::PortExpander;

    IPortExpanderRuntime() = default;
    IPortExpanderRuntime(const IPortExpanderRuntime&) = delete;
    IPortExpanderRuntime& operator=(const IPortExpanderRuntime&) = delete;
    IPortExpanderRuntime(IPortExpanderRuntime&&) = delete;
    IPortExpanderRuntime& operator=(IPortExpanderRuntime&&) = delete;
    virtual ~IPortExpanderRuntime() = default;

    virtual uint8_t channelCount() const = 0;
    virtual bool isChannelOn(uint8_t channel) const = 0;
    virtual bool requestChannelState(uint8_t channel, bool on, uint32_t now) = 0;
};

// Bus roles have exactly one implementation each today, so dependents still access them through
// the concrete class (static_cast), not through these interfaces -- they exist purely to carry
// `kProvidedRole` for the dependency-role mechanism described above.
// A time-of-day/weekday rule set that reports whether it is currently "active", so a consumer
// (e.g. AutoSwitchDevice) can follow it without knowing anything about wall-clock time itself. The
// implementing device reads the app's current time itself (DateTime::current()) whenever asked -
// there is no push/subscription mechanism here.
class IScheduleRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::Schedule;

    IScheduleRuntime() = default;
    IScheduleRuntime(const IScheduleRuntime&) = delete;
    IScheduleRuntime& operator=(const IScheduleRuntime&) = delete;
    IScheduleRuntime(IScheduleRuntime&&) = delete;
    IScheduleRuntime& operator=(IScheduleRuntime&&) = delete;
    virtual ~IScheduleRuntime() = default;

    virtual bool isActive() const = 0;
    virtual bool timeValid() const = 0;
};

// A secondary, orthogonal capability: "this device can report a boolean status," regardless of
// whatever role(s) it primarily provides. Implemented alongside IScheduleRuntime/ISwitchOutputRuntime
// (not instead of), so a consumer's `condition`-role dependency link can point at a schedule, a
// switch, or another AutoSwitchDevice uniformly. See DeviceTypeDescriptor::providedRoles: this
// interface's kProvidedRole is one of potentially several roles a type declares.
class IStatusRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::Condition;

    IStatusRuntime() = default;
    IStatusRuntime(const IStatusRuntime&) = delete;
    IStatusRuntime& operator=(const IStatusRuntime&) = delete;
    IStatusRuntime(IStatusRuntime&&) = delete;
    IStatusRuntime& operator=(IStatusRuntime&&) = delete;
    virtual ~IStatusRuntime() = default;

    virtual bool isActive() const = 0;
};

class IOneWireBusRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::OneWireBus;

    IOneWireBusRuntime() = default;
    IOneWireBusRuntime(const IOneWireBusRuntime&) = delete;
    IOneWireBusRuntime& operator=(const IOneWireBusRuntime&) = delete;
    IOneWireBusRuntime(IOneWireBusRuntime&&) = delete;
    IOneWireBusRuntime& operator=(IOneWireBusRuntime&&) = delete;
    virtual ~IOneWireBusRuntime() = default;
};

class II2cBusRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::I2CBus;

    II2cBusRuntime() = default;
    II2cBusRuntime(const II2cBusRuntime&) = delete;
    II2cBusRuntime& operator=(const II2cBusRuntime&) = delete;
    II2cBusRuntime(II2cBusRuntime&&) = delete;
    II2cBusRuntime& operator=(II2cBusRuntime&&) = delete;
    virtual ~II2cBusRuntime() = default;
};

class ISpiBusRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::SpiBus;

    ISpiBusRuntime() = default;
    ISpiBusRuntime(const ISpiBusRuntime&) = delete;
    ISpiBusRuntime& operator=(const ISpiBusRuntime&) = delete;
    ISpiBusRuntime(ISpiBusRuntime&&) = delete;
    ISpiBusRuntime& operator=(ISpiBusRuntime&&) = delete;
    virtual ~ISpiBusRuntime() = default;
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
    Delete = 6,
    Scan = 8,
    SetOutput = 9,
    Custom = 10,
    SetDeps = 11,
    ResetDiagnostics = 12,
    CheckDevice = 13,
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

    const DeviceDependencyLink* dependencyLink(DeviceRole role) const {
        const DeviceDependencyLink* links = dependencyLinks();
        const uint8_t count = dependencyCount();
        for (uint8_t index = 0; index < count; ++index) {
            if (links[index].role == role) {
                return &links[index];
            }
        }
        return nullptr;
    }

    DeviceId dependencyDeviceId(DeviceRole role) const {
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
    virtual void setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) {
        (void)role;
        (void)dependencyRuntime;
    }
    virtual void setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) {
        (void)index;
        (void)dependencyRuntime;
    }
    virtual IDeviceRuntime* dependencyRuntime(DeviceRole role) const {
        (void)role;
        return nullptr;
    }
    virtual IDeviceRuntime* dependencyRuntimeAt(uint8_t index) const {
        (void)index;
        return nullptr;
    }
    virtual const ITemperatureReadingRuntime* temperatureReadingRuntime() const {
        return nullptr;
    }
    virtual const IHumidityReadingRuntime* humidityReadingRuntime() const {
        return nullptr;
    }
    virtual const ISwitchOutputRuntime* switchOutputRuntime() const {
        return nullptr;
    }
    virtual const IAnalogOutputRuntime* analogOutputRuntime() const {
        return nullptr;
    }
    virtual const IScheduledAnalogOutputRuntime* scheduledAnalogOutputRuntime() const {
        return nullptr;
    }
    virtual const IAnalogOutputGroupRuntime* analogOutputGroupRuntime() const {
        return nullptr;
    }
    virtual const IAnalogInputRuntime* analogInputRuntime() const {
        return nullptr;
    }
    virtual const IAnalogInputHubRuntime* analogInputHubRuntime() const {
        return nullptr;
    }
    virtual const IRealTimeClockRuntime* realTimeClockRuntime() const {
        return nullptr;
    }
    virtual const IPortExpanderRuntime* portExpanderRuntime() const {
        return nullptr;
    }
    virtual const IScheduleRuntime* scheduleRuntime() const {
        return nullptr;
    }
    virtual const IStatusRuntime* statusRuntime() const {
        return nullptr;
    }
    virtual DisplayDeviceBase* displayRuntime() {
        return nullptr;
    }
    virtual const DisplayDeviceBase* displayRuntime() const {
        return nullptr;
    }
    virtual CharacterDisplayRuntimeBase* characterDisplayRuntime() {
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
    virtual DeviceId dependencyDeviceId(DeviceRole role) const {
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
    virtual bool expanderChannel(uint8_t& channel) const {
        (void)channel;
        return false;
    }
    // Multi-channel sibling of expanderChannel(), for a dependent that reserves several channels of
    // one port expander at once (e.g. an HD44780 LCD's RS/E/D4-D7/backlight lines). Writes up to
    // maxOut channel numbers into `out` and returns how many were written; default reports none.
    virtual uint8_t expanderChannels(uint8_t* out, uint8_t maxOut) const {
        (void)out;
        (void)maxOut;
        return 0;
    }
    virtual bool hasDuplicateDependentChannel(uint8_t channel, const IDeviceRuntime* ignoreDependent = nullptr) const {
        (void)channel;
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
    // Roles listed here are controlling relationships: only one registry device may own a given
    // target for that role. External REST/HA commands remain valid and are not registry owners.
    ProvidedRoles exclusiveDependencyRoles{};
    // The role(s) this type provides to other devices' dependencies, if any (empty means it
    // provides none). Each entry is set from the corresponding I*Runtime interface's
    // `kProvidedRole` -- see ProvidedRoles above.
    ProvidedRoles providedRoles{};
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
