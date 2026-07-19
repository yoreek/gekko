#include "portal/DashboardLayoutStore.h"

#include "debug/Debug.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "dashboard";
constexpr const char* kLayoutKey = "layout";
constexpr const char* kLayoutBlobKey = "layout_blob";
constexpr const char* kRevisionKey = "revision";
constexpr char kStorageMagic[] = {'D', 'L', 'B', '1'};
constexpr uint8_t kStorageFormatVersion = 1U;

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
    if (!value.is<int>() && !value.is<unsigned int>() && !value.is<long>() && !value.is<unsigned long>() && !value.is<long long>() &&
        !value.is<unsigned long long>()) {
        return false;
    }

    const long long raw = value.as<long long>();
    if (raw < 0) {
        return false;
    }

    if (raw > UINT32_MAX) {
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

struct BinaryCursor {
    const std::vector<uint8_t>& bytes;
    size_t offset{0};

    bool readBytes(void* output, const size_t size) {
        if (offset + size > bytes.size()) {
            return false;
        }
        std::memcpy(output, bytes.data() + offset, size);
        offset += size;
        return true;
    }

    bool readU8(uint8_t& value) {
        return readBytes(&value, sizeof(value));
    }

    bool readU16(uint16_t& value) {
        uint8_t raw[2]{};
        if (!readBytes(raw, sizeof(raw))) {
            return false;
        }
        value = static_cast<uint16_t>(static_cast<uint16_t>(raw[0]) | (static_cast<uint16_t>(raw[1]) << 8U));
        return true;
    }

    bool readU32(uint32_t& value) {
        uint8_t raw[4]{};
        if (!readBytes(raw, sizeof(raw))) {
            return false;
        }
        value = static_cast<uint32_t>(raw[0]) | (static_cast<uint32_t>(raw[1]) << 8U) | (static_cast<uint32_t>(raw[2]) << 16U) |
                (static_cast<uint32_t>(raw[3]) << 24U);
        return true;
    }

    bool readString(std::string& value) {
        uint16_t length{0};
        if (!readU16(length)) {
            return false;
        }
        if (offset + length > bytes.size()) {
            return false;
        }
        value.assign(reinterpret_cast<const char*>(bytes.data() + offset), length);
        offset += length;
        return true;
    }

    bool finished() const {
        return offset == bytes.size();
    }
};

bool appendU8(std::vector<uint8_t>& bytes, const uint8_t value) {
    bytes.push_back(value);
    return true;
}

bool appendU16(std::vector<uint8_t>& bytes, const uint16_t value) {
    bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    return true;
}

bool appendU32(std::vector<uint8_t>& bytes, const uint32_t value) {
    bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    bytes.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
    bytes.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
    return true;
}

bool appendString(std::vector<uint8_t>& bytes, const std::string& value) {
    if (value.size() > UINT16_MAX) {
        return false;
    }
    appendU16(bytes, static_cast<uint16_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
    return true;
}

bool serializeLayoutBinary(const DashboardLayoutSnapshot& layout, std::vector<uint8_t>& bytes) {
    bytes.clear();
    bytes.reserve(128U + layout.panels.size() * 64U);
    bytes.insert(bytes.end(), kStorageMagic, kStorageMagic + sizeof(kStorageMagic));
    appendU8(bytes, kStorageFormatVersion);
    appendU32(bytes, layout.schemaVersion);
    if (!appendString(bytes, layout.activePanelId)) {
        return false;
    }
    if (layout.panels.size() > UINT8_MAX) {
        return false;
    }
    appendU8(bytes, static_cast<uint8_t>(layout.panels.size()));
    for (const auto& panel : layout.panels) {
        if (!appendString(bytes, panel.id) || !appendString(bytes, panel.name)) {
            return false;
        }
        appendU8(bytes, panel.order);
        if (panel.widgets.size() > UINT8_MAX) {
            return false;
        }
        appendU8(bytes, static_cast<uint8_t>(panel.widgets.size()));
        for (const auto& widget : panel.widgets) {
            appendU32(bytes, widget.deviceId);
            appendU16(bytes, widget.x);
            appendU16(bytes, widget.y);
            appendU16(bytes, widget.w);
            appendU16(bytes, widget.h);
        }
    }
    return bytes.size() <= DashboardLayoutStore::kMaxSerializedBytes;
}

bool deserializeLayoutBinary(const std::vector<uint8_t>& bytes, DashboardLayoutSnapshot& layout) {
    layout = {};
    BinaryCursor cursor{bytes};

    char magic[4]{};
    if (!cursor.readBytes(magic, sizeof(magic)) || std::memcmp(magic, kStorageMagic, sizeof(magic)) != 0) {
        return false;
    }

    uint8_t storageVersion{0};
    if (!cursor.readU8(storageVersion) || storageVersion != kStorageFormatVersion) {
        return false;
    }

    if (!cursor.readU32(layout.schemaVersion)) {
        return false;
    }

    if (!cursor.readString(layout.activePanelId)) {
        return false;
    }

    uint8_t panelCount{0};
    if (!cursor.readU8(panelCount)) {
        return false;
    }

    layout.panels.reserve(panelCount);
    for (uint8_t panelIndex = 0; panelIndex < panelCount; ++panelIndex) {
        DashboardPanelLayout panel{};
        if (!cursor.readString(panel.id) || !cursor.readString(panel.name)) {
            return false;
        }

        if (!cursor.readU8(panel.order)) {
            return false;
        }

        uint8_t widgetCount{0};
        if (!cursor.readU8(widgetCount)) {
            return false;
        }

        panel.widgets.reserve(widgetCount);
        for (uint8_t widgetIndex = 0; widgetIndex < widgetCount; ++widgetIndex) {
            DashboardLayoutWidget widget{};
            uint32_t deviceId{0};
            if (!cursor.readU32(deviceId) || !cursor.readU16(widget.x) || !cursor.readU16(widget.y) || !cursor.readU16(widget.w) ||
                !cursor.readU16(widget.h)) {
                return false;
            }
            widget.deviceId = static_cast<DeviceId>(deviceId);
            panel.widgets.push_back(widget);
        }

        layout.panels.push_back(std::move(panel));
    }

    return cursor.finished();
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
        registry_->forEachRuntime([&](const IDeviceRuntime& runtime) {
            DashboardLayoutWidget widget{};
            widget.deviceId = runtime.deviceId();
            widget.x = static_cast<uint16_t>(index % 6U);
            widget.y = static_cast<uint16_t>(index / 6U);
            widget.w = 1;
            widget.h = 1;
            panel.widgets.push_back(widget);
            ++index;
        });
    }

    layout.panels.push_back(panel);
    return layout;
}

DashboardLayoutLoadResult DashboardLayoutStore::load() {
    DashboardLayoutLoadResult result{};
    result.revision = readRevision();

    DashboardLayoutSnapshot parsed{};

    std::vector<uint8_t> storedBlob;
    if (storage_.getBlob(kLayoutBlobKey, storedBlob)) {
        if (storedBlob.empty() || storedBlob.size() > kMaxSerializedBytes || !deserializeLayoutBinary(storedBlob, parsed)) {
            result.layout = defaultLayout();
            result.defaulted = true;
            result.validation = {DashboardLayoutError::BadStorage, "stored dashboard layout is invalid"};
            return result;
        }
    } else {
        if (storage_.hasKey(kLayoutKey)) {
            (void)storage_.remove(kLayoutKey);
        }
        result.layout = defaultLayout();
        result.defaulted = true;
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
        bool hasDevices = false;
        if (registry_ != nullptr) {
            registry_->forEachRuntime([&](const IDeviceRuntime&) { hasDevices = true; });
        }
        if (!hasWidgets && hasDevices) {
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
        for (JsonVariantConst widgetJson : widgets) {
            DashboardLayoutWidget widget{};
            if (!parseWidget(widgetJson, widget)) {
                widget.deviceId = 0;
                widget.x = UINT16_MAX;
                widget.y = UINT16_MAX;
                widget.w = 0;
                widget.h = 0;
            }
            panel.widgets.push_back(widget);
        }

        layout.panels.push_back(panel);
        ++order;
    }

    return true;
}

bool DashboardLayoutStore::parseWidget(JsonVariantConst json, DashboardLayoutWidget& widget) const {
    if (json.is<JsonArrayConst>()) {
        const JsonArrayConst tuple = json.as<JsonArrayConst>();
        if (tuple.size() < 5) {
            return false;
        }

        uint32_t deviceId{0};
        if (!readUInt32(tuple[0], deviceId)) {
            return false;
        }
        if (!readUInt16(tuple[1], widget.x)) {
            return false;
        }
        if (!readUInt16(tuple[2], widget.y)) {
            return false;
        }
        if (!readUInt16(tuple[3], widget.w)) {
            return false;
        }
        if (!readUInt16(tuple[4], widget.h)) {
            return false;
        }

        widget.deviceId = static_cast<DeviceId>(deviceId);
        return true;
    }

    if (!json.is<JsonObjectConst>()) {
        return false;
    }
    const JsonObjectConst object = json.as<JsonObjectConst>();

    uint32_t deviceId{0};
    if (!readUInt32(object["deviceId"], deviceId)) {
        return false;
    }
    if (!readUInt16(object["x"], widget.x)) {
        return false;
    }
    if (!readUInt16(object["y"], widget.y)) {
        return false;
    }
    if (!readUInt16(object["w"], widget.w)) {
        return false;
    }
    if (!readUInt16(object["h"], widget.h)) {
        return false;
    }

    widget.deviceId = static_cast<DeviceId>(deviceId);
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
            JsonArray widgetJson = widgets.createNestedArray();
            widgetJson.add(widget.deviceId);
            widgetJson.add(widget.x);
            widgetJson.add(widget.y);
            widgetJson.add(widget.w);
            widgetJson.add(widget.h);
        }
    }
}

const char* DashboardLayoutStore::errorCode(DashboardLayoutError error) const {
    switch (error) {
    case DashboardLayoutError::BadJson:
        return "BAD_JSON";
    case DashboardLayoutError::BadStorage:
        return "BAD_STORAGE";
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
    std::vector<uint8_t> payload;
    if (!serializeLayoutBinary(layout, payload)) {
        return false;
    }

    if (!storage_.putBlob(kLayoutBlobKey, payload) || !storage_.putUInt(kRevisionKey, revision)) {
        return false;
    }

    if (storage_.hasKey(kLayoutKey)) {
        (void)storage_.remove(kLayoutKey);
    }

    return true;
}

uint32_t DashboardLayoutStore::readRevision() const {
    uint32_t revision{0};
    (void)storage_.getUInt(kRevisionKey, revision);
    return revision;
}

bool DashboardLayoutStore::deviceExists(const DeviceId deviceId) const {
    return registry_ == nullptr || registry_->runtime(deviceId) != nullptr;
}

} // namespace ewfm
