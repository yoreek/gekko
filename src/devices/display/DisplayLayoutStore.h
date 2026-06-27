#pragma once

#include "devices/registry/DeviceScopedDataStore.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace ewfm {

enum class DisplayLayoutBindingKind : uint8_t {
    Unbound = 0,
    Device = 1,
    Metric = 2,
    ConstantText = 3,
};

constexpr uint8_t kDisplayLayoutSchemaVersion = 1;
constexpr size_t kDisplayLayoutMaxPages = 2;
constexpr size_t kDisplayLayoutMaxWidgetsPerPage = 4;
constexpr size_t kDisplayLayoutPageIdCapacity = 16;
constexpr size_t kDisplayLayoutTextCapacity = 32;

struct DisplayLayoutWidgetV1 {
    uint8_t bindingKind{static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    char text[kDisplayLayoutTextCapacity]{};
};

struct DisplayLayoutPageV1 {
    char id[kDisplayLayoutPageIdCapacity]{};
    std::vector<DisplayLayoutWidgetV1> widgets{};
};

struct DisplayLayoutRecordV1 {
    DeviceId deviceId{0};
    uint16_t recordVersion{1};
    uint8_t schemaVersion{kDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    std::vector<DisplayLayoutPageV1> pages{};
};

#pragma pack(push, 1)
struct DisplayLayoutBinaryHeaderV1 {
    uint16_t recordVersion{1};
    DeviceId deviceId{0};
    uint8_t schemaVersion{kDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    uint8_t pageCount{0};
    uint8_t reserved{0};
};

struct DisplayLayoutBinaryPageHeaderV1 {
    uint8_t widgetCount{0};
    char id[kDisplayLayoutPageIdCapacity]{};
};

struct DisplayLayoutBinaryWidgetV1 {
    uint8_t bindingKind{static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    char text[kDisplayLayoutTextCapacity]{};
};
#pragma pack(pop)

class DisplayLayoutStore {
public:
    static constexpr const char* kDataType = "display_layout";

    explicit DisplayLayoutStore(IConfigStorage& storage)
        : ownedStorage_(new DeviceScopedDataStore(storage)), storage_(ownedStorage_.get()) {}
    explicit DisplayLayoutStore(DeviceScopedDataStore& storage) : storage_(&storage) {}

    bool begin(bool readOnly = false);
    DeviceValidationResult load(DeviceId deviceId, DisplayLayoutRecordV1& record);
    DeviceValidationResult save(const DisplayLayoutRecordV1& record);
    bool remove(DeviceId deviceId);
    bool clearDevice(DeviceId deviceId);

private:
    std::unique_ptr<DeviceScopedDataStore> ownedStorage_{};
    DeviceScopedDataStore* storage_;
};

bool encodeDisplayLayoutBinary(const DisplayLayoutRecordV1& layout, std::vector<uint8_t>& blob);
bool decodeDisplayLayoutBinary(const uint8_t* data, size_t size, DisplayLayoutRecordV1& layout);

} // namespace ewfm
