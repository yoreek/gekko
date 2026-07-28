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
    Digital = 1,
    Bitmap = 2,
    Rect = 3,
    Line = 4,
    Circle = 5,
    Ellipse = 6,
    Character = 7,
};

enum class DisplayDigitalAlign : uint8_t {
    Left = 0,
    Center = 1,
    Right = 2,
};

enum class DisplayLayoutBitmapFormat : uint8_t {
    Mono1 = 0,
    Gray8 = 1,
    Rgb565 = 2,
};

constexpr uint8_t kDisplayLayoutSchemaVersion = 3;
constexpr uint16_t kDisplayLayoutRecordVersion = 7;
constexpr size_t kDisplayLayoutMaxPages = 2;
constexpr size_t kDisplayLayoutMaxWidgetsPerPage = 10;
constexpr size_t kDisplayLayoutPageIdCapacity = 16;
constexpr size_t kDisplayLayoutPageNameCapacity = 16;
constexpr size_t kDisplayLayoutWidgetIdCapacity = 16;
constexpr size_t kDisplayLayoutTextCapacity = 128;
constexpr size_t kDisplayLayoutTextCapacityLegacy = 32;
constexpr size_t kDisplayLayoutDigitalPatternCapacity = 8;
constexpr size_t kDisplayLayoutBitmapDataCapacity = 3072;
// Sized for the blob-store key format this feature actually uses ("dev/<deviceId hex, <=8
// chars>/<8-char random suffix>"), not the store's generic kBlobStoreMaxKeyBytes ceiling (96) -
// "dev"(3) + '/' + 8 + '/' + 8 + '\0' = 22, rounded up to 32 for headroom.
constexpr size_t kDisplayLayoutImageKeyCapacity = 32;
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
    uint16_t color{0xFFFFU};
    uint8_t digitalAlign{static_cast<uint8_t>(DisplayDigitalAlign::Right)};
    char digitalOverflow[kDisplayLayoutDigitalPatternCapacity]{};
    char digitalMissing[kDisplayLayoutDigitalPatternCapacity]{};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    char text[kDisplayLayoutTextCapacity]{};
    mutable std::optional<DisplayTextCompiledWidget> textAst{};
    // Blob-store key for a Bitmap widget's pixel bytes (empty for every other widget type); the
    // bytes themselves live in LittleFsBlobStore, not here (see docs/blob-store.md).
    char imageKey[kDisplayLayoutImageKeyCapacity]{};
    // Lazily-populated cache of the bytes fetched from the blob store for `imageKey`, read once per
    // render session. Same pattern as `textAst` above: reset happens for free because reloading the
    // layout (DisplayLayoutStore::load) always constructs a fresh DisplayLayoutWidgetV1, so there is
    // no separate invalidation path to maintain.
    mutable std::optional<std::vector<uint8_t>> cachedImageBytes{};
};

constexpr bool isDisplayTextWidgetType(const DisplayLayoutWidgetType type) {
    return type == DisplayLayoutWidgetType::Text || type == DisplayLayoutWidgetType::Character || type == DisplayLayoutWidgetType::Digital;
}

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
    uint16_t backgroundColor{0U};
    std::vector<DisplayLayoutPageV1> pages{};
};

#pragma pack(push, 1)
struct [[deprecated("legacy persisted display layout header; decode/migration only")]] DisplayLayoutBinaryHeaderV1 {
    uint16_t recordVersion{1U};
    DeviceId deviceId{0};
    uint8_t schemaVersion{kDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    uint8_t pageCount{0};
    uint8_t reserved{0};
};

struct DisplayLayoutBinaryHeaderV5 {
    uint16_t recordVersion{5U};
    DeviceId deviceId{0};
    uint8_t schemaVersion{kDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    uint8_t pageCount{0};
    uint8_t reserved{0};
    uint16_t backgroundColor{0U};
};

struct DisplayLayoutBinaryHeaderV6 {
    uint16_t recordVersion{kDisplayLayoutRecordVersion};
    DeviceId deviceId{0};
    uint8_t schemaVersion{kDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    uint8_t pageCount{0};
    uint8_t reserved{0};
    uint16_t backgroundColor{0U};
};

struct DisplayLayoutBinaryPageHeaderV1 {
    uint8_t widgetCount{0};
    char id[kDisplayLayoutPageIdCapacity]{};
    char name[kDisplayLayoutPageNameCapacity]{};
    uint8_t order{0};
};

struct [[deprecated("legacy persisted display layout widget; decode/migration only")]] DisplayLayoutBinaryWidgetV1 {
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

struct [[deprecated("legacy persisted display layout widget; decode/migration only")]] DisplayLayoutBinaryWidgetV2 {
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

struct [[deprecated("legacy persisted display layout widget; decode/migration only")]] DisplayLayoutBinaryWidgetV3 {
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

struct [[deprecated("legacy persisted display layout widget; decode/migration only")]] DisplayLayoutBinaryWidgetV4 {
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

struct [[deprecated("legacy persisted display layout widget; decode/migration only")]] DisplayLayoutBinaryWidgetV5 {
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
    uint16_t color{0xFFFFU};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    uint16_t bitmapDataLength{0};
    char text[kDisplayLayoutTextCapacity]{};
};

struct [[deprecated("legacy persisted display layout widget; decode/migration only")]] DisplayLayoutBinaryWidgetV6 {
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
    uint16_t color{0xFFFFU};
    uint8_t digitalAlign{static_cast<uint8_t>(DisplayDigitalAlign::Right)};
    char digitalOverflow[kDisplayLayoutDigitalPatternCapacity]{};
    char digitalMissing[kDisplayLayoutDigitalPatternCapacity]{};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    uint16_t bitmapDataLength{0};
    char text[kDisplayLayoutTextCapacity]{};
};

// Current wire format (record version kDisplayLayoutRecordVersion = 7). Unlike V1-V6, the bitmap
// payload is not a variable-length trailer following this struct - it is a fixed-size blob-store
// key embedded directly in the struct, since the actual pixel bytes now live in LittleFsBlobStore.
struct DisplayLayoutBinaryWidgetV7 {
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
    uint16_t color{0xFFFFU};
    uint8_t digitalAlign{static_cast<uint8_t>(DisplayDigitalAlign::Right)};
    char digitalOverflow[kDisplayLayoutDigitalPatternCapacity]{};
    char digitalMissing[kDisplayLayoutDigitalPatternCapacity]{};
    uint8_t bitmapFormat{static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1)};
    uint8_t keepAspectRatio{0};
    char imageKey[kDisplayLayoutImageKeyCapacity]{};
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
