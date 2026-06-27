#include "devices/display/DisplayLayoutCodec.h"

#include "devices/core/DeviceBaseConfig.h"

#include <cstring>
#include <type_traits>

namespace ewfm {
namespace {

template <typename T> bool appendBinary(std::vector<uint8_t>& blob, const T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "appendBinary requires trivially copyable data");
    const size_t offset = blob.size();
    blob.resize(offset + sizeof(T));
    std::memcpy(blob.data() + offset, &value, sizeof(T));
    return true;
}

template <typename T> bool readBinary(const uint8_t* data, size_t size, size_t& offset, T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "readBinary requires trivially copyable data");
    if (offset + sizeof(T) > size) {
        return false;
    }
    std::memcpy(&value, data + offset, sizeof(T));
    offset += sizeof(T);
    return true;
}

bool copyText(char* dest, size_t capacity, const char* source) {
    if (dest == nullptr || capacity == 0U) {
        return false;
    }
    dest[0] = '\0';
    if (source == nullptr) {
        return true;
    }
    std::strncpy(dest, source, capacity - 1U);
    dest[capacity - 1U] = '\0';
    return true;
}

DisplayLayoutPageV1 defaultLayoutPage() {
    DisplayLayoutPageV1 page{};
    copyText(page.id, sizeof(page.id), "main");
    return page;
}

bool validateLayout(const DisplayLayoutRecordV1& layout) {
    if (layout.recordVersion != 1U || layout.schemaVersion != kDisplayLayoutSchemaVersion) {
        return false;
    }
    if (layout.activePageIndex >= layout.pages.size() && !layout.pages.empty()) {
        return false;
    }
    if (layout.pages.size() > kDisplayLayoutMaxPages) {
        return false;
    }
    for (const DisplayLayoutPageV1& page : layout.pages) {
        if (std::strlen(page.id) == 0U || page.widgets.size() > kDisplayLayoutMaxWidgetsPerPage) {
            return false;
        }
    }
    return true;
}

} // namespace

void writeDisplayLayoutJson(const DisplayLayoutRecordV1& layout, JsonObject output) {
    output["schemaVersion"] = layout.schemaVersion;
    const char* activePageId = "main";
    if (layout.activePageIndex < layout.pages.size()) {
        activePageId = layout.pages[layout.activePageIndex].id;
    } else if (!layout.pages.empty()) {
        activePageId = layout.pages.front().id;
    }
    output["activePageId"] = activePageId;
    JsonArray pages = output.createNestedArray("pages");
    for (const DisplayLayoutPageV1& page : layout.pages) {
        JsonObject pageJson = pages.createNestedObject();
        pageJson["id"] = page.id;
        JsonArray widgets = pageJson.createNestedArray("widgets");
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            JsonObject widgetJson = widgets.createNestedObject();
            widgetJson["bindingKind"] = widget.bindingKind;
            widgetJson["x"] = widget.x;
            widgetJson["y"] = widget.y;
            widgetJson["width"] = widget.width;
            widgetJson["height"] = widget.height;
            widgetJson["sourceDeviceId"] = widget.sourceDeviceId;
            widgetJson["metricId"] = widget.metricId;
            widgetJson["text"] = widget.text;
        }
    }
}

bool parseDisplayLayoutJson(const JsonObjectConst& input, DisplayLayoutRecordV1& layout) {
    layout = {};
    layout.deviceId = 0;
    layout.recordVersion = 1;
    layout.schemaVersion = input["schemaVersion"] | kDisplayLayoutSchemaVersion;
    const char* activePageId = input["activePageId"] | nullptr;
    const bool hasActivePageIndex = !input["activePageIndex"].isNull();
    const uint8_t requestedActivePageIndex = input["activePageIndex"] | 0U;

    const JsonArrayConst pages = input["pages"].as<JsonArrayConst>();
    if (pages.isNull()) {
        return false;
    }

    for (JsonVariantConst pageVariant : pages) {
        if (!pageVariant.is<JsonObjectConst>()) {
            return false;
        }
        if (layout.pages.size() >= kDisplayLayoutMaxPages) {
            return false;
        }
        const JsonObjectConst pageJson = pageVariant.as<JsonObjectConst>();
        DisplayLayoutPageV1 page{};
        if (!copyText(page.id, sizeof(page.id), pageJson["id"] | "")) {
            return false;
        }
        const JsonArrayConst widgets = pageJson["widgets"].as<JsonArrayConst>();
        if (!widgets.isNull()) {
            for (JsonVariantConst widgetVariant : widgets) {
                if (!widgetVariant.is<JsonObjectConst>()) {
                    return false;
                }
                if (page.widgets.size() >= kDisplayLayoutMaxWidgetsPerPage) {
                    return false;
                }
                const JsonObjectConst widgetJson = widgetVariant.as<JsonObjectConst>();
                DisplayLayoutWidgetV1 widget{};
                widget.bindingKind = widgetJson["bindingKind"] | 0U;
                widget.x = widgetJson["x"] | 0U;
                widget.y = widgetJson["y"] | 0U;
                widget.width = widgetJson["width"] | 1U;
                widget.height = widgetJson["height"] | 1U;
                widget.sourceDeviceId = widgetJson["sourceDeviceId"] | 0UL;
                widget.metricId = widgetJson["metricId"] | 0L;
                if (!copyText(widget.text, sizeof(widget.text), widgetJson["text"] | "")) {
                    return false;
                }
                page.widgets.push_back(widget);
            }
        }
        layout.pages.push_back(page);
    }

    if (layout.pages.empty()) {
        layout.pages.push_back(defaultLayoutPage());
    }

    if (activePageId != nullptr && activePageId[0] != '\0') {
        bool found = false;
        for (size_t index = 0; index < layout.pages.size(); ++index) {
            if (std::strcmp(layout.pages[index].id, activePageId) == 0) {
                layout.activePageIndex = static_cast<uint8_t>(index);
                found = true;
                break;
            }
        }
        if (!found) {
            layout.activePageIndex = 0U;
        }
    } else if (hasActivePageIndex) {
        layout.activePageIndex = requestedActivePageIndex;
    } else {
        layout.activePageIndex = 0U;
    }

    if (layout.activePageIndex >= layout.pages.size()) {
        layout.activePageIndex = 0U;
    }
    return validateLayout(layout);
}

bool encodeDisplayLayoutBinary(const DisplayLayoutRecordV1& layout, std::vector<uint8_t>& blob) {
    if (!validateLayout(layout)) {
        return false;
    }

    blob.clear();
    DisplayLayoutBinaryHeaderV1 header{};
    header.recordVersion = layout.recordVersion;
    header.deviceId = layout.deviceId;
    header.schemaVersion = layout.schemaVersion;
    header.activePageIndex = layout.activePageIndex;
    header.pageCount = static_cast<uint8_t>(layout.pages.size());
    if (!appendBinary(blob, header)) {
        return false;
    }

    for (const DisplayLayoutPageV1& page : layout.pages) {
        DisplayLayoutBinaryPageHeaderV1 pageHeader{};
        pageHeader.widgetCount = static_cast<uint8_t>(page.widgets.size());
        if (!copyText(pageHeader.id, sizeof(pageHeader.id), page.id)) {
            return false;
        }
        if (!appendBinary(blob, pageHeader)) {
            return false;
        }
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            DisplayLayoutBinaryWidgetV1 binaryWidget{};
            binaryWidget.bindingKind = widget.bindingKind;
            binaryWidget.x = widget.x;
            binaryWidget.y = widget.y;
            binaryWidget.width = widget.width;
            binaryWidget.height = widget.height;
            binaryWidget.sourceDeviceId = widget.sourceDeviceId;
            binaryWidget.metricId = widget.metricId;
            if (!copyText(binaryWidget.text, sizeof(binaryWidget.text), widget.text)) {
                return false;
            }
            if (!appendBinary(blob, binaryWidget)) {
                return false;
            }
        }
    }
    return true;
}

bool decodeDisplayLayoutBinary(const uint8_t* data, size_t size, DisplayLayoutRecordV1& layout) {
    layout = {};
    if (data == nullptr || size < sizeof(DisplayLayoutBinaryHeaderV1)) {
        return false;
    }

    size_t offset = 0;
    DisplayLayoutBinaryHeaderV1 header{};
    if (!readBinary(data, size, offset, header)) {
        return false;
    }
    if (header.recordVersion != 1U || header.schemaVersion != kDisplayLayoutSchemaVersion) {
        return false;
    }
    if (header.pageCount == 0U || header.pageCount > kDisplayLayoutMaxPages) {
        return false;
    }
    if (header.activePageIndex >= header.pageCount) {
        return false;
    }

    layout.deviceId = header.deviceId;
    layout.recordVersion = header.recordVersion;
    layout.schemaVersion = header.schemaVersion;
    layout.activePageIndex = header.activePageIndex;
    layout.pages.reserve(header.pageCount);

    for (uint8_t pageIndex = 0; pageIndex < header.pageCount; ++pageIndex) {
        DisplayLayoutBinaryPageHeaderV1 pageHeader{};
        if (!readBinary(data, size, offset, pageHeader)) {
            return false;
        }
        if (pageHeader.widgetCount > kDisplayLayoutMaxWidgetsPerPage) {
            return false;
        }
        DisplayLayoutPageV1 page{};
        if (!copyText(page.id, sizeof(page.id), pageHeader.id)) {
            return false;
        }
        page.widgets.reserve(pageHeader.widgetCount);
        for (uint8_t widgetIndex = 0; widgetIndex < pageHeader.widgetCount; ++widgetIndex) {
            DisplayLayoutBinaryWidgetV1 binaryWidget{};
            if (!readBinary(data, size, offset, binaryWidget)) {
                return false;
            }
            DisplayLayoutWidgetV1 widget{};
            widget.bindingKind = binaryWidget.bindingKind;
            widget.x = binaryWidget.x;
            widget.y = binaryWidget.y;
            widget.width = binaryWidget.width;
            widget.height = binaryWidget.height;
            widget.sourceDeviceId = binaryWidget.sourceDeviceId;
            widget.metricId = binaryWidget.metricId;
            if (!copyText(widget.text, sizeof(widget.text), binaryWidget.text)) {
                return false;
            }
            page.widgets.push_back(widget);
        }
        layout.pages.push_back(page);
    }

    return offset == size && validateLayout(layout);
}

} // namespace ewfm
