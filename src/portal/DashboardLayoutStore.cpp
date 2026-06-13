#include "portal/DashboardLayoutStore.h"

#include "debug/Debug.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "dashboard";
constexpr const char* kLayoutKey = "layout";
constexpr const char* kRevisionKey = "revision";

std::string toLowerAscii(const std::string& value) {
    std::string lowered = value;
    for (char& c : lowered) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return lowered;
}

bool readUInt32(JsonVariantConst value, uint32_t& output) {
    if (!value.is<int>() && !value.is<unsigned int>() && !value.is<long>() && !value.is<unsigned long>()) {
        return false;
    }

    const long raw = value.as<long>();
    if (raw < 0) {
        return false;
    }

    output = static_cast<uint32_t>(raw);
    return true;
}

bool readUInt16(JsonVariantConst value, uint16_t& output) {
    uint32_t raw{0};
    if (!readUInt32(value, raw) || raw > UINT16_MAX) {
        return false;
    }

    output = static_cast<uint16_t>(raw);
    return true;
}
} // namespace

bool DashboardLayoutStore::begin() {
    return storage_.begin(kNamespace, false);
}

DashboardLayoutSnapshot DashboardLayoutStore::defaultLayout() const {
    DashboardLayoutSnapshot layout{};
    layout.schemaVersion = kSchemaVersion;
    layout.activePanelId = "main";

    DashboardPanelLayout panel{};
    panel.id = "main";
    panel.name = "Main";
    panel.order = 0;

    if (registry_ != nullptr) {
        uint16_t index = 0;
        for (const auto& record : registry_->list()) {
            DashboardLayoutWidget widget{};
            widget.deviceId = record.header.deviceId;
            widget.x = static_cast<uint16_t>(index % 6U);
            widget.y = static_cast<uint16_t>(index / 6U);
            widget.w = 1;
            widget.h = 1;
            panel.widgets.push_back(widget);
            ++index;
        }
    }

    layout.panels.push_back(panel);
    return layout;
}

DashboardLayoutLoadResult DashboardLayoutStore::load() {
    DashboardLayoutLoadResult result{};
    result.revision = readRevision();

    std::string stored;
    if (!storage_.getString(kLayoutKey, stored) || stored.empty() || stored.size() > kMaxSerializedBytes) {
        result.layout = defaultLayout();
        result.defaulted = true;
        return result;
    }

    DynamicJsonDocument doc(kMaxSerializedBytes);
    const DeserializationError error = deserializeJson(doc, stored.c_str());
    if (error) {
        result.layout = defaultLayout();
        result.defaulted = true;
        result.validation = {DashboardLayoutError::BadJson, "stored dashboard layout is invalid"};
        return result;
    }

    DashboardLayoutSnapshot parsed{};
    if (!parseLayout(doc.as<JsonVariantConst>(), parsed)) {
        result.layout = defaultLayout();
        result.defaulted = true;
        result.validation = {DashboardLayoutError::BadJson, "stored dashboard layout is invalid"};
        return result;
    }

    result.validation = validate(parsed);
    if (!result.validation.ok()) {
        result.layout = defaultLayout();
        result.defaulted = true;
        return result;
    }

    if (pruneUnknownDevices(parsed)) {
        bool hasWidgets = false;
        for (const auto& panel : parsed.panels) {
            hasWidgets = hasWidgets || !panel.widgets.empty();
        }
        if (!hasWidgets && registry_ != nullptr && !registry_->list().empty()) {
            result.layout = defaultLayout();
            result.defaulted = true;
            return result;
        }

        result.revision += 1;
        (void)persistLayout(parsed, result.revision);
    }

    result.layout = std::move(parsed);
    return result;
}

DashboardLayoutSaveResult DashboardLayoutStore::saveJson(JsonVariantConst json) {
    DashboardLayoutSnapshot layout{};
    if (!parseLayout(json, layout)) {
        DashboardLayoutSaveResult result{};
        result.validation = validate(layout);
        result.layout = layout;
        result.revision = readRevision();
        return result;
    }
    return save(layout);
}

DashboardLayoutSaveResult DashboardLayoutStore::save(const DashboardLayoutSnapshot& layout) {
    DashboardLayoutSaveResult result{};
    result.revision = readRevision();
    result.layout = layout;

    result.validation = validate(result.layout);
    if (!result.validation.ok()) {
        return result;
    }

    (void)pruneUnknownDevices(result.layout);
    result.validation = validate(result.layout);
    if (!result.validation.ok()) {
        return result;
    }

    result.revision += 1;
    if (!persistLayout(result.layout, result.revision)) {
        result.validation = {DashboardLayoutError::StorageError, "failed to save dashboard layout"};
        return result;
    }

    return result;
}

bool DashboardLayoutStore::parseLayout(JsonVariantConst json, DashboardLayoutSnapshot& layout) const {
    layout = {};
    const JsonObjectConst root = json.as<JsonObjectConst>();
    if (root.isNull()) {
        return false;
    }

    layout.schemaVersion = root["schema_version"] | 0U;
    layout.activePanelId = root["active_panel_id"] | "";

    const JsonArrayConst panels = root["panels"].as<JsonArrayConst>();
    uint8_t order = 0;
    for (JsonObjectConst panelJson : panels) {
        DashboardPanelLayout panel{};
        panel.id = panelJson["id"] | "";
        panel.name = panelJson["name"] | "";
        panel.order = static_cast<uint8_t>(panelJson["order"] | order);

        const JsonArrayConst widgets = panelJson["widgets"].as<JsonArrayConst>();
        for (JsonObjectConst widgetJson : widgets) {
            DashboardLayoutWidget widget{};
            uint32_t deviceId{0};
            if (!readUInt32(widgetJson["device_id"], deviceId)) {
                deviceId = 0;
            }
            widget.deviceId = static_cast<DeviceId>(deviceId);
            if (!readUInt16(widgetJson["x"], widget.x)) {
                widget.x = UINT16_MAX;
            }
            if (!readUInt16(widgetJson["y"], widget.y)) {
                widget.y = UINT16_MAX;
            }
            if (!readUInt16(widgetJson["w"], widget.w)) {
                widget.w = 0;
            }
            if (!readUInt16(widgetJson["h"], widget.h)) {
                widget.h = 0;
            }
            panel.widgets.push_back(widget);
        }

        layout.panels.push_back(panel);
        ++order;
    }

    return true;
}

DashboardLayoutResult DashboardLayoutStore::validate(const DashboardLayoutSnapshot& layout) const {
    if (layout.schemaVersion != kSchemaVersion) {
        return {DashboardLayoutError::UnsupportedSchema, "unsupported dashboard layout schema"};
    }
    if (layout.panels.empty()) {
        return {DashboardLayoutError::EmptyPanels, "dashboard layout must contain at least one panel"};
    }
    if (layout.panels.size() > kMaxPanels) {
        return {DashboardLayoutError::TooManyPanels, "dashboard layout exceeds panel limit"};
    }

    bool activePanelFound = false;
    std::vector<std::string> panelIds;
    std::vector<std::string> panelNames;
    for (const auto& panel : layout.panels) {
        if (panel.id.empty()) {
            return {DashboardLayoutError::DuplicatePanelId, "dashboard panel id is empty"};
        }
        if (panel.name.empty()) {
            return {DashboardLayoutError::DuplicatePanelName, "dashboard panel name is empty"};
        }
        if (panel.name.size() > kMaxPanelNameLength) {
            return {DashboardLayoutError::PanelNameTooLong, "dashboard panel name is too long"};
        }
        if (std::find(panelIds.begin(), panelIds.end(), panel.id) != panelIds.end()) {
            return {DashboardLayoutError::DuplicatePanelId, "dashboard panel id is duplicated"};
        }
        const std::string loweredName = toLowerAscii(panel.name);
        if (std::find(panelNames.begin(), panelNames.end(), loweredName) != panelNames.end()) {
            return {DashboardLayoutError::DuplicatePanelName, "dashboard panel name is duplicated"};
        }
        panelIds.push_back(panel.id);
        panelNames.push_back(loweredName);
        activePanelFound = activePanelFound || panel.id == layout.activePanelId;

        if (panel.widgets.size() > kMaxWidgetsPerPanel) {
            return {DashboardLayoutError::InvalidWidget, "dashboard panel contains too many widgets"};
        }
        std::vector<DeviceId> widgetDeviceIds;
        for (const auto& widget : panel.widgets) {
            if (widget.deviceId == 0U || widget.x == UINT16_MAX || widget.y == UINT16_MAX || widget.w == 0U || widget.h == 0U) {
                return {DashboardLayoutError::InvalidWidget, "dashboard widget coordinates are invalid"};
            }
            if (std::find(widgetDeviceIds.begin(), widgetDeviceIds.end(), widget.deviceId) != widgetDeviceIds.end()) {
                return {DashboardLayoutError::DuplicateWidget, "dashboard widget device is duplicated"};
            }
            widgetDeviceIds.push_back(widget.deviceId);
        }
    }

    if (!activePanelFound) {
        return {DashboardLayoutError::InvalidActivePanel, "dashboard active panel does not exist"};
    }

    return {};
}

bool DashboardLayoutStore::pruneUnknownDevices(DashboardLayoutSnapshot& layout) const {
    bool changed = false;
    for (auto& panel : layout.panels) {
        const size_t before = panel.widgets.size();
        panel.widgets.erase(std::remove_if(panel.widgets.begin(), panel.widgets.end(),
                                           [this](const DashboardLayoutWidget& widget) { return !deviceExists(widget.deviceId); }),
                            panel.widgets.end());
        changed = changed || before != panel.widgets.size();
    }
    return changed;
}

void DashboardLayoutStore::writeLayoutJson(JsonObject target, const DashboardLayoutSnapshot& layout) const {
    target["schema_version"] = layout.schemaVersion;
    target["active_panel_id"] = layout.activePanelId;
    JsonArray panels = target.createNestedArray("panels");
    for (const auto& panel : layout.panels) {
        JsonObject panelJson = panels.createNestedObject();
        panelJson["id"] = panel.id;
        panelJson["name"] = panel.name;
        panelJson["order"] = panel.order;
        JsonArray widgets = panelJson.createNestedArray("widgets");
        for (const auto& widget : panel.widgets) {
            JsonObject widgetJson = widgets.createNestedObject();
            widgetJson["device_id"] = widget.deviceId;
            widgetJson["x"] = widget.x;
            widgetJson["y"] = widget.y;
            widgetJson["w"] = widget.w;
            widgetJson["h"] = widget.h;
        }
    }
}

const char* DashboardLayoutStore::errorCode(DashboardLayoutError error) const {
    switch (error) {
    case DashboardLayoutError::BadJson:
        return "BAD_JSON";
    case DashboardLayoutError::UnsupportedSchema:
        return "UNSUPPORTED_SCHEMA";
    case DashboardLayoutError::EmptyPanels:
        return "EMPTY_PANELS";
    case DashboardLayoutError::TooManyPanels:
        return "TOO_MANY_PANELS";
    case DashboardLayoutError::DuplicatePanelId:
        return "DUPLICATE_PANEL_ID";
    case DashboardLayoutError::DuplicatePanelName:
        return "DUPLICATE_PANEL_NAME";
    case DashboardLayoutError::PanelNameTooLong:
        return "PANEL_NAME_TOO_LONG";
    case DashboardLayoutError::InvalidActivePanel:
        return "INVALID_ACTIVE_PANEL";
    case DashboardLayoutError::InvalidWidget:
        return "INVALID_WIDGET";
    case DashboardLayoutError::DuplicateWidget:
        return "DUPLICATE_WIDGET";
    case DashboardLayoutError::StorageError:
        return "STORAGE_ERROR";
    case DashboardLayoutError::None:
    default:
        return "BAD_LAYOUT";
    }
}

bool DashboardLayoutStore::persistLayout(const DashboardLayoutSnapshot& layout, const uint32_t revision) {
    DynamicJsonDocument doc(kMaxSerializedBytes);
    JsonObject root = doc.to<JsonObject>();
    writeLayoutJson(root, layout);

    std::string payload;
    serializeJson(doc, payload);
    if (payload.size() > kMaxSerializedBytes) {
        return false;
    }

    return storage_.putString(kLayoutKey, payload) && storage_.putUInt(kRevisionKey, revision);
}

uint32_t DashboardLayoutStore::readRevision() const {
    uint32_t revision{0};
    (void)storage_.getUInt(kRevisionKey, revision);
    return revision;
}

bool DashboardLayoutStore::deviceExists(const DeviceId deviceId) const {
    return registry_ == nullptr || registry_->find(deviceId) != nullptr;
}

} // namespace ewfm
