#pragma once

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

constexpr uint32_t kDeviceRegistrySchemaVersion = 1;
constexpr uint16_t kDeviceRegistryIndexVersion = 1;
constexpr uint16_t kDeviceRecordHeaderVersion = 1;
constexpr uint16_t kRetainedStateRecordVersion = 1;
constexpr size_t kMaxDynamicDevices = 200;
constexpr size_t kMaxDynamicDeviceNameLength = 32;
constexpr size_t kMaxDeviceConfigBytes = 512;
constexpr size_t kMaxRetainedStateBytes = 64;
constexpr size_t kMaxDeviceEventBytes = 256;
constexpr size_t kMaxRegistryIndexBytes = 2048;
constexpr size_t kMaxDeviceRecordBytes = 1024;
constexpr size_t kMaxDeviceIdGenerationAttempts = 8;

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

enum class DeviceCommandType : uint8_t {
    None = 0,
    Create = 1,
    UpdateConfig = 2,
    Rename = 3,
    Enable = 4,
    Disable = 5,
    Delete = 6,
    SetStatus = 7,
    Custom = 8,
    SetParent = 9,
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

struct DeviceIndexEntry {
    DeviceId deviceId{0};
    DeviceTypeId typeId{0};
};

struct DeviceRecord {
    DeviceRecordHeader header{};
    std::string name{};
    bool enabled{true};
    bool hasParent{false};
    DeviceId parentDeviceId{0};
    DevicePersistencePolicy persistencePolicy{DevicePersistencePolicy::Delayed};
    DeviceStatus status{DeviceStatus::Unknown};
    std::string configPayload{};
};

struct DeviceRegistrySnapshot {
    uint32_t schemaVersion{kDeviceRegistrySchemaVersion};
    std::vector<DeviceIndexEntry> indexEntries{};
    std::vector<DeviceRecord> records{};
};

struct RetainedStateRecord {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0};
    std::string payload{};
};

struct DeviceCommand {
    DeviceCommandType type{DeviceCommandType::None};
    DeviceId deviceId{0};
    BoundedText<kMaxDeviceEventBytes> payload{};
    DevicePersistencePolicy persistencePolicy{DevicePersistencePolicy::Delayed};

    DeviceCommand() = default;

    DeviceCommand(DeviceCommandType commandType, DeviceId commandDeviceId, std::string_view commandPayload,
                  DevicePersistencePolicy commandPolicy = DevicePersistencePolicy::Delayed)
        : type(commandType), deviceId(commandDeviceId), persistencePolicy(commandPolicy) {
        (void)payload.assign(commandPayload);
    }

    bool valid() const {
        return payload.valid();
    }
};

struct DeviceEvent {
    DeviceEventKind kind{DeviceEventKind::RegistryLoaded};
    uint32_t registryRevision{0};
    uint32_t configRevision{0};
    DeviceId deviceId{0};
    DeviceTypeId typeId{0};
    DeviceStatus previousStatus{DeviceStatus::Unknown};
    DeviceStatus status{DeviceStatus::Unknown};
    bool pendingPersistence{false};
    bool commandAccepted{false};
    BoundedText<kMaxDeviceEventBytes> detail{};
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
    virtual void setParentRuntime(IDeviceRuntime* parentRuntime) {
        (void)parentRuntime;
    }
    virtual IDeviceRuntime* parentRuntime() const {
        return nullptr;
    }
    virtual void attachChildRuntime(IDeviceRuntime* childRuntime) {
        (void)childRuntime;
    }
    virtual void detachChildRuntime(IDeviceRuntime* childRuntime) {
        (void)childRuntime;
    }
    virtual const std::vector<IDeviceRuntime*>& childRuntimes() const {
        static const std::vector<IDeviceRuntime*> kEmptyChildren;
        return kEmptyChildren;
    }
    virtual void requestReconfigure() = 0;
    virtual void requestDisable() = 0;
    virtual void requestDelete() = 0;
    virtual DeviceStatus status() const = 0;
    virtual bool handleCommand(const DeviceCommand& command) = 0;
};

class DeviceTypeDescriptor {
public:
    using RuntimeFactory = std::unique_ptr<IDeviceRuntime> (*)(const DeviceRecord&);
    using ConfigValidator = DeviceValidationResult (*)(const DeviceRecord&);

    DeviceTypeId typeId{0};
    const char* name{nullptr};
    uint32_t currentConfigVersion{1};
    bool canHaveChildren{false};
    uint8_t maxChildren{0};
    bool supportsCommands{false};
    bool supportsRetainedState{false};
    DevicePersistencePolicy defaultPersistencePolicy{DevicePersistencePolicy::Delayed};
    bool ticksFastLoop{false};
    bool ticks100ms{false};
    bool ticks1s{false};
    std::vector<DeviceTypeId> compatibleParentTypes{};
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
