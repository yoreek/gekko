#pragma once

#include "config/ConfigStore.h"
#include "devices/registry/DeviceRegistry.h"

#include <ArduinoJson.h>
#include <cstdint>
#include <string>
#include <vector>

namespace ewfm {

struct DashboardLayoutWidget {
    DeviceId deviceId{0};
    uint16_t x{0};
    uint16_t y{0};
    uint16_t w{1};
    uint16_t h{1};
};

struct DashboardPanelLayout {
    std::string id{};
    std::string name{};
    uint8_t order{0};
    std::vector<DashboardLayoutWidget> widgets{};
};

struct DashboardLayoutSnapshot {
    uint32_t schemaVersion{1};
    std::string activePanelId{"main"};
    std::vector<DashboardPanelLayout> panels{};
};

enum class DashboardLayoutError {
    None,
    BadJson,
    UnsupportedSchema,
    EmptyPanels,
    TooManyPanels,
    DuplicatePanelId,
    DuplicatePanelName,
    PanelNameTooLong,
    InvalidActivePanel,
    InvalidWidget,
    DuplicateWidget,
    StorageError,
};

struct DashboardLayoutResult {
    DashboardLayoutError error{DashboardLayoutError::None};
    const char* message{""};

    bool ok() const {
        return error == DashboardLayoutError::None;
    }
};

struct DashboardLayoutLoadResult {
    DashboardLayoutResult validation{};
    DashboardLayoutSnapshot layout{};
    uint32_t revision{0};
    bool defaulted{false};
};

struct DashboardLayoutSaveResult {
    DashboardLayoutResult validation{};
    DashboardLayoutSnapshot layout{};
    uint32_t revision{0};

    bool ok() const {
        return validation.ok();
    }
};

class DashboardLayoutStore {
public:
    static constexpr uint32_t kSchemaVersion = 1;
    static constexpr size_t kMaxPanels = 8;
    static constexpr size_t kMaxPanelNameLength = 32;
    static constexpr size_t kMaxWidgetsPerPanel = 64;
    static constexpr size_t kMaxSerializedBytes = 4096;

    explicit DashboardLayoutStore(IConfigStorage& storage, const DeviceRegistry* registry = nullptr)
        : storage_(storage), registry_(registry) {}

    bool begin();
    void setRegistry(const DeviceRegistry* registry) {
        registry_ = registry;
    }

    DashboardLayoutLoadResult load();
    DashboardLayoutSaveResult save(const DashboardLayoutSnapshot& layout);
    DashboardLayoutSaveResult saveJson(JsonVariantConst json);

    DashboardLayoutSnapshot defaultLayout() const;
    DashboardLayoutResult validate(const DashboardLayoutSnapshot& layout) const;
    bool pruneUnknownDevices(DashboardLayoutSnapshot& layout) const;
    void writeLayoutJson(JsonObject target, const DashboardLayoutSnapshot& layout) const;
    const char* errorCode(DashboardLayoutError error) const;

private:
    bool parseLayout(JsonVariantConst json, DashboardLayoutSnapshot& layout) const;
    bool persistLayout(const DashboardLayoutSnapshot& layout, uint32_t revision);
    uint32_t readRevision() const;
    bool deviceExists(DeviceId deviceId) const;

    IConfigStorage& storage_;
    const DeviceRegistry* registry_{nullptr};
};

} // namespace ewfm
