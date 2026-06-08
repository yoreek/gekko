#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
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
    std::string payload{};
    DevicePersistencePolicy persistencePolicy{DevicePersistencePolicy::Delayed};
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
