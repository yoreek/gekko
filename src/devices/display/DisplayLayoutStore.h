#pragma once

#include "devices/display/DisplayTextPlaceholderTypes.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "metrics/MetricTypes.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ewfm {

enum class DisplayLayoutBindingKind : uint8_t {
    Unbound = 0,
    Device = 1,
    Metric = 2,
    ConstantText = 3,
};

enum class DisplayLayoutWidgetType : uint8_t {
    Text = 0,
    Bitmap = 2,
    Rect = 3,
    Line = 4,
    Circle = 5,
    Ellipse = 6,
};

enum class DisplayLayoutBitmapFormat : uint8_t {
    Mono1 = 0,
    Gray8 = 1,
    Rgb565 = 2,
};

constexpr uint8_t kDisplayLayoutSchemaVersion = 1;
constexpr uint16_t kDisplayLayoutRecordVersion = 4;
constexpr size_t kDisplayLayoutMaxPages = 2;
constexpr size_t kDisplayLayoutMaxWidgetsPerPage = 10;
constexpr size_t kDisplayLayoutPageIdCapacity = 16;
constexpr size_t kDisplayLayoutPageNameCapacity = 16;
constexpr size_t kDisplayLayoutWidgetIdCapacity = 16;
constexpr size_t kDisplayLayoutTextCapacity = 128;
constexpr size_t kDisplayLayoutTextCapacityLegacy = 32;
constexpr size_t kDisplayLayoutBitmapDataCapacity = 3072;
constexpr uint16_t kDisplayLayoutRefreshIntervalDisabled = 0;
constexpr uint16_t kDisplayLayoutRefreshIntervalMinMs = 250;
constexpr uint16_t kDisplayLayoutRefreshIntervalMaxMs = 60000;

struct DisplayLayoutWidgetV1 {
    char id[kDisplayLayoutWidgetIdCapacity]{};
    uint8_t type{static_cast<uint8_t>(DisplayLayoutWidgetType::Text)};
    uint8_t bindingKind{static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound)};
    uint8_t metricNamespace{static_cast<uint8_t>(MetricNamespace::Device)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    uint16_t refreshIntervalMs{kDisplayLayoutRefreshIntervalDisabled};
    uint8_t fontSize{1};
    uint8_t strokeWidth{1};
    uint8_t autoSize{0};
    uint8_t styleFlags{0};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    char text[kDisplayLayoutTextCapacity]{};
    mutable std::optional<DisplayTextCompiledWidget> textAst{};
    std::vector<uint8_t> bitmapData{};
};

struct DisplayLayoutPageV1 {
    char id[kDisplayLayoutPageIdCapacity]{};
    char name[kDisplayLayoutPageNameCapacity]{};
    uint8_t order{0};
    std::vector<DisplayLayoutWidgetV1> widgets{};
};

struct DisplayLayoutRecordV1 {
    DeviceId deviceId{0};
    uint16_t recordVersion{kDisplayLayoutRecordVersion};
    uint8_t schemaVersion{kDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    std::vector<DisplayLayoutPageV1> pages{};
};

#pragma pack(push, 1)
struct DisplayLayoutBinaryHeaderV1 {
    uint16_t recordVersion{kDisplayLayoutRecordVersion};
    DeviceId deviceId{0};
    uint8_t schemaVersion{kDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    uint8_t pageCount{0};
    uint8_t reserved{0};
};

struct DisplayLayoutBinaryPageHeaderV1 {
    uint8_t widgetCount{0};
    char id[kDisplayLayoutPageIdCapacity]{};
    char name[kDisplayLayoutPageNameCapacity]{};
    uint8_t order{0};
};

struct DisplayLayoutBinaryWidgetV1 {
    char id[kDisplayLayoutWidgetIdCapacity]{};
    uint8_t type{static_cast<uint8_t>(DisplayLayoutWidgetType::Text)};
    uint8_t bindingKind{static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    uint8_t fontSize{1};
    uint8_t strokeWidth{1};
    uint8_t autoSize{0};
    uint8_t styleFlags{0};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    uint16_t bitmapDataLength{0};
    char text[kDisplayLayoutTextCapacityLegacy]{};
};

struct DisplayLayoutBinaryWidgetV2 {
    char id[kDisplayLayoutWidgetIdCapacity]{};
    uint8_t type{static_cast<uint8_t>(DisplayLayoutWidgetType::Text)};
    uint8_t bindingKind{static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound)};
    uint8_t metricNamespace{static_cast<uint8_t>(MetricNamespace::Device)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    uint8_t fontSize{1};
    uint8_t strokeWidth{1};
    uint8_t autoSize{0};
    uint8_t styleFlags{0};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    uint16_t bitmapDataLength{0};
    char text[kDisplayLayoutTextCapacityLegacy]{};
};

struct DisplayLayoutBinaryWidgetV3 {
    char id[kDisplayLayoutWidgetIdCapacity]{};
    uint8_t type{static_cast<uint8_t>(DisplayLayoutWidgetType::Text)};
    uint8_t bindingKind{static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound)};
    uint8_t metricNamespace{static_cast<uint8_t>(MetricNamespace::Device)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    uint16_t refreshIntervalMs{kDisplayLayoutRefreshIntervalDisabled};
    uint8_t fontSize{1};
    uint8_t strokeWidth{1};
    uint8_t autoSize{0};
    uint8_t styleFlags{0};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    uint16_t bitmapDataLength{0};
    char text[kDisplayLayoutTextCapacityLegacy]{};
};

struct DisplayLayoutBinaryWidgetV4 {
    char id[kDisplayLayoutWidgetIdCapacity]{};
    uint8_t type{static_cast<uint8_t>(DisplayLayoutWidgetType::Text)};
    uint8_t bindingKind{static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound)};
    uint8_t metricNamespace{static_cast<uint8_t>(MetricNamespace::Device)};
    uint8_t x{0};
    uint8_t y{0};
    uint8_t width{1};
    uint8_t height{1};
    uint32_t sourceDeviceId{0};
    int32_t metricId{0};
    uint16_t refreshIntervalMs{kDisplayLayoutRefreshIntervalDisabled};
    uint8_t fontSize{1};
    uint8_t strokeWidth{1};
    uint8_t autoSize{0};
    uint8_t styleFlags{0};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    uint16_t bitmapDataLength{0};
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
