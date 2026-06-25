#pragma once

#include "devices/display/oled/OledDisplayDeviceConfig.h"
#include "devices/registry/DeviceScopedDataStore.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace ewfm {

enum class OledDisplayLayoutBindingKind : uint8_t {
    Unbound = 0,
    Device = 1,
    Metric = 2,
    ConstantText = 3,
};

constexpr uint8_t kOledDisplayLayoutSchemaVersion = 1;
constexpr size_t kOledDisplayLayoutMaxPages = 2;
constexpr size_t kOledDisplayLayoutMaxWidgetsPerPage = 4;
constexpr size_t kOledDisplayLayoutPageIdCapacity = 16;
constexpr size_t kOledDisplayLayoutTextCapacity = 32;

struct OledDisplayLayoutWidgetV1 {
    uint8_t bindingKind{static_cast<uint8_t>(OledDisplayLayoutBindingKind::Unbound)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    char text[kOledDisplayLayoutTextCapacity]{};
};

struct OledDisplayLayoutPageV1 {
    char id[kOledDisplayLayoutPageIdCapacity]{};
    std::vector<OledDisplayLayoutWidgetV1> widgets{};
};

struct OledDisplayLayoutRecordV1 {
    DeviceId deviceId{0};
    uint16_t recordVersion{1};
    uint8_t schemaVersion{kOledDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    std::vector<OledDisplayLayoutPageV1> pages{};
};

#pragma pack(push, 1)
struct OledDisplayLayoutBinaryHeaderV1 {
    uint16_t recordVersion{1};
    DeviceId deviceId{0};
    uint8_t schemaVersion{kOledDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    uint8_t pageCount{0};
    uint8_t reserved{0};
};

struct OledDisplayLayoutBinaryPageHeaderV1 {
    uint8_t widgetCount{0};
    char id[kOledDisplayLayoutPageIdCapacity]{};
};

struct OledDisplayLayoutBinaryWidgetV1 {
    uint8_t bindingKind{static_cast<uint8_t>(OledDisplayLayoutBindingKind::Unbound)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    char text[kOledDisplayLayoutTextCapacity]{};
};
#pragma pack(pop)

class OledDisplayLayoutStore {
public:
    static constexpr const char* kDataType = "display_layout";

    explicit OledDisplayLayoutStore(IConfigStorage& storage)
        : ownedStorage_(new DeviceScopedDataStore(storage)), storage_(ownedStorage_.get()) {}
    explicit OledDisplayLayoutStore(DeviceScopedDataStore& storage) : storage_(&storage) {}

    bool begin(bool readOnly = false);
    DeviceValidationResult load(DeviceId deviceId, OledDisplayLayoutRecordV1& record);
    DeviceValidationResult save(const OledDisplayLayoutRecordV1& record);
    bool remove(DeviceId deviceId);
    bool clearDevice(DeviceId deviceId);

private:
    std::unique_ptr<DeviceScopedDataStore> ownedStorage_{};
    DeviceScopedDataStore* storage_;
};

bool encodeOledDisplayLayoutBinary(const OledDisplayLayoutRecordV1& layout, std::vector<uint8_t>& blob);
bool decodeOledDisplayLayoutBinary(const uint8_t* data, size_t size, OledDisplayLayoutRecordV1& layout);

} // namespace ewfm
