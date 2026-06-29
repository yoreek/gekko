#include "devices/display/DisplayLayoutCodec.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/DisplayTextPlaceholderAst.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <type_traits>

namespace ewfm {
namespace {

template <typename T> bool appendBinary(std::vector<uint8_t>& blob, const T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "appendBinary requires trivially copyable data");
    if (blob.size() + sizeof(T) > kMaxDisplayLayoutBytes) {
        return false;
    }
    const size_t offset = blob.size();
    blob.resize(offset + sizeof(T));
    std::memcpy(blob.data() + offset, &value, sizeof(T));
    return true;
}

bool appendBytes(std::vector<uint8_t>& blob, const uint8_t* data, size_t size) {
    if (size == 0U) {
        return true;
    }
    if (blob.size() + size > kMaxDisplayLayoutBytes) {
        return false;
    }
    const size_t offset = blob.size();
    blob.resize(offset + size);
    std::memcpy(blob.data() + offset, data, size);
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

bool readBytes(const uint8_t* data, size_t size, size_t& offset, size_t length, std::vector<uint8_t>& out) {
    if (offset + length > size) {
        return false;
    }
    out.assign(data + offset, data + offset + length);
    offset += length;
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

bool copyStringToBytes(std::vector<uint8_t>& dest, const char* source) {
    dest.clear();
    if (source == nullptr || source[0] == '\0') {
        return true;
    }

    auto decodeBase64Char = [](char ch) -> int {
        if (ch >= 'A' && ch <= 'Z')
            return ch - 'A';
        if (ch >= 'a' && ch <= 'z')
            return ch - 'a' + 26;
        if (ch >= '0' && ch <= '9')
            return ch - '0' + 52;
        if (ch == '+')
            return 62;
        if (ch == '/')
            return 63;
        return -1;
    };

    const size_t length = std::strlen(source);
    if ((length % 4U) != 0U) {
        return false;
    }

    dest.reserve((length / 4U) * 3U);
    for (size_t index = 0; index < length; index += 4U) {
        const char a = source[index];
        const char b = source[index + 1U];
        const char c = source[index + 2U];
        const char d = source[index + 3U];
        const int v0 = decodeBase64Char(a);
        const int v1 = decodeBase64Char(b);
        const int v2 = c == '=' ? -2 : decodeBase64Char(c);
        const int v3 = d == '=' ? -2 : decodeBase64Char(d);
        if (v0 < 0 || v1 < 0 || v2 == -1 || v3 == -1) {
            return false;
        }
        if (c == '=' && d != '=') {
            return false;
        }

        const uint32_t triple = (static_cast<uint32_t>(v0) << 18U) | (static_cast<uint32_t>(v1) << 12U) |
                                (static_cast<uint32_t>((v2 < 0 ? 0 : v2)) << 6U) | static_cast<uint32_t>(v3 < 0 ? 0 : v3);
        dest.push_back(static_cast<uint8_t>((triple >> 16U) & 0xFFU));
        if (c != '=') {
            dest.push_back(static_cast<uint8_t>((triple >> 8U) & 0xFFU));
        }
        if (d != '=') {
            dest.push_back(static_cast<uint8_t>(triple & 0xFFU));
        }
        if (dest.size() > kDisplayLayoutBitmapDataCapacity) {
            return false;
        }
    }
    return true;
}

std::string bytesToString(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return {};
    }
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((bytes.size() + 2U) / 3U) * 4U);
    for (size_t index = 0; index < bytes.size(); index += 3U) {
        const uint8_t b0 = bytes[index];
        const uint8_t b1 = (index + 1U) < bytes.size() ? bytes[index + 1U] : 0U;
        const uint8_t b2 = (index + 2U) < bytes.size() ? bytes[index + 2U] : 0U;
        const uint32_t triple = (static_cast<uint32_t>(b0) << 16U) | (static_cast<uint32_t>(b1) << 8U) | static_cast<uint32_t>(b2);
        result.push_back(kAlphabet[(triple >> 18U) & 0x3FU]);
        result.push_back(kAlphabet[(triple >> 12U) & 0x3FU]);
        result.push_back((index + 1U) < bytes.size() ? kAlphabet[(triple >> 6U) & 0x3FU] : '=');
        result.push_back((index + 2U) < bytes.size() ? kAlphabet[triple & 0x3FU] : '=');
    }
    return result;
}

bool readBool(const JsonVariantConst& value, const bool fallback) {
    return value.isNull() ? fallback : value.as<bool>();
}

DisplayLayoutPageV1 defaultLayoutPage() {
    DisplayLayoutPageV1 page{};
    copyText(page.id, sizeof(page.id), "main");
    copyText(page.name, sizeof(page.name), "Main");
    page.order = 0U;
    return page;
}

DisplayLayoutWidgetType normalizeWidgetType(const DisplayLayoutWidgetType type) {
    switch (type) {
    case DisplayLayoutWidgetType::Text:
    case DisplayLayoutWidgetType::Bitmap:
    case DisplayLayoutWidgetType::Rect:
    case DisplayLayoutWidgetType::Line:
    case DisplayLayoutWidgetType::Circle:
    case DisplayLayoutWidgetType::Ellipse:
        return type;
    default:
        return DisplayLayoutWidgetType::Text;
    }
}

const char* widgetTypeToString(DisplayLayoutWidgetType type) {
    switch (normalizeWidgetType(type)) {
    case DisplayLayoutWidgetType::Text:
        return "text";
    case DisplayLayoutWidgetType::Bitmap:
        return "bitmap";
    case DisplayLayoutWidgetType::Rect:
        return "rect";
    case DisplayLayoutWidgetType::Line:
        return "line";
    case DisplayLayoutWidgetType::Circle:
        return "circle";
    case DisplayLayoutWidgetType::Ellipse:
        return "ellipse";
    }
    return "text";
}

bool parseWidgetType(const JsonObjectConst& input, DisplayLayoutWidgetType& type, const char*& error) {
    const JsonVariantConst typeValue = input["type"];
    if (typeValue.isNull()) {
        type = DisplayLayoutWidgetType::Text;
        return true;
    }
    if (typeValue.is<uint8_t>() || typeValue.is<int>() || typeValue.is<unsigned int>()) {
        const int numeric = typeValue.as<int>();
        switch (numeric) {
        case 0:
            type = DisplayLayoutWidgetType::Text;
            return true;
        case 2:
            type = DisplayLayoutWidgetType::Bitmap;
            return true;
        case 3:
            type = DisplayLayoutWidgetType::Rect;
            return true;
        case 4:
            type = DisplayLayoutWidgetType::Line;
            return true;
        case 5:
            type = DisplayLayoutWidgetType::Circle;
            return true;
        case 6:
            type = DisplayLayoutWidgetType::Ellipse;
            return true;
        default:
            error = "unsupported display widget type";
            return false;
        }
    }
    const char* value = typeValue.as<const char*>();
    if (value == nullptr) {
        error = "display widget type is invalid";
        return false;
    }
    if (std::strcmp(value, "text") == 0) {
        type = DisplayLayoutWidgetType::Text;
    } else if (std::strcmp(value, "bitmap") == 0) {
        type = DisplayLayoutWidgetType::Bitmap;
    } else if (std::strcmp(value, "rect") == 0) {
        type = DisplayLayoutWidgetType::Rect;
    } else if (std::strcmp(value, "line") == 0) {
        type = DisplayLayoutWidgetType::Line;
    } else if (std::strcmp(value, "circle") == 0) {
        type = DisplayLayoutWidgetType::Circle;
    } else if (std::strcmp(value, "ellipse") == 0) {
        type = DisplayLayoutWidgetType::Ellipse;
    } else {
        error = "unsupported display widget type";
        return false;
    }
    return true;
}

const char* bindingKindToString(const DisplayLayoutBindingKind kind) {
    switch (kind) {
    case DisplayLayoutBindingKind::Unbound:
        return "unbound";
    case DisplayLayoutBindingKind::Device:
        return "device";
    case DisplayLayoutBindingKind::Metric:
        return "metric";
    case DisplayLayoutBindingKind::ConstantText:
        return "constant_text";
    }
    return "unbound";
}

MetricNamespace normalizeMetricNamespace(const MetricNamespace ns) {
    switch (ns) {
    case MetricNamespace::Device:
    case MetricNamespace::System:
    case MetricNamespace::Wifi:
        return ns;
    }
    return MetricNamespace::Device;
}

const char* metricNamespaceToString(const MetricNamespace ns) {
    return metricNamespaceName(normalizeMetricNamespace(ns));
}

uint16_t normalizeRefreshIntervalMs(const uint32_t value) {
    if (value == kDisplayLayoutRefreshIntervalDisabled) {
        return kDisplayLayoutRefreshIntervalDisabled;
    }
    if (value < kDisplayLayoutRefreshIntervalMinMs) {
        return kDisplayLayoutRefreshIntervalMinMs;
    }
    if (value > kDisplayLayoutRefreshIntervalMaxMs) {
        return kDisplayLayoutRefreshIntervalMaxMs;
    }
    return static_cast<uint16_t>(value);
}

bool parseWidgetMetricNamespace(const JsonObjectConst& input, uint8_t& metricNamespace) {
    const JsonVariantConst value = input["metricNamespace"];
    if (value.isNull()) {
        metricNamespace = static_cast<uint8_t>(MetricNamespace::Device);
        return true;
    }
    if (value.is<uint8_t>() || value.is<int>() || value.is<unsigned int>()) {
        const int numeric = value.as<int>();
        if (numeric < static_cast<int>(MetricNamespace::Device) || numeric > static_cast<int>(MetricNamespace::Wifi)) {
            return false;
        }
        metricNamespace = static_cast<uint8_t>(numeric);
        return true;
    }
    MetricNamespace parsed{};
    if (!parseMetricNamespace(value.as<const char*>(), parsed)) {
        return false;
    }
    metricNamespace = static_cast<uint8_t>(parsed);
    return true;
}

bool parseBindingKind(const JsonObjectConst& input, uint8_t& bindingKind) {
    const JsonVariantConst value = input["bindingKind"];
    if (value.isNull()) {
        bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound);
        return true;
    }
    if (value.is<uint8_t>() || value.is<int>() || value.is<unsigned int>()) {
        const int numeric = value.as<int>();
        if (numeric < 0 || numeric > static_cast<int>(DisplayLayoutBindingKind::ConstantText)) {
            return false;
        }
        bindingKind = static_cast<uint8_t>(numeric);
        return true;
    }
    const char* text = value.as<const char*>();
    if (text == nullptr) {
        return false;
    }
    if (std::strcmp(text, "unbound") == 0) {
        bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound);
    } else if (std::strcmp(text, "device") == 0) {
        bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::Device);
    } else if (std::strcmp(text, "metric") == 0) {
        bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::Metric);
    } else if (std::strcmp(text, "constant_text") == 0) {
        bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    } else {
        return false;
    }
    return true;
}

DisplayLayoutBitmapFormat normalizeBitmapFormat(const DisplayLayoutBitmapFormat format) {
    switch (format) {
    case DisplayLayoutBitmapFormat::Mono1:
    case DisplayLayoutBitmapFormat::Gray8:
    case DisplayLayoutBitmapFormat::Rgb565:
        return format;
    default:
        return DisplayLayoutBitmapFormat::Mono1;
    }
}

const char* bitmapFormatToString(DisplayLayoutBitmapFormat format) {
    switch (normalizeBitmapFormat(format)) {
    case DisplayLayoutBitmapFormat::Mono1:
        return "mono1";
    case DisplayLayoutBitmapFormat::Gray8:
        return "gray8";
    case DisplayLayoutBitmapFormat::Rgb565:
        return "rgb565";
    }
    return "mono1";
}

bool parseBitmapFormat(const JsonObjectConst& input, DisplayLayoutBitmapFormat& format) {
    const JsonVariantConst value = input["bitmapFormat"];
    if (value.isNull()) {
        format = DisplayLayoutBitmapFormat::Mono1;
        return true;
    }
    if (value.is<uint8_t>() || value.is<int>() || value.is<unsigned int>()) {
        const int numeric = value.as<int>();
        switch (numeric) {
        case 0:
            format = DisplayLayoutBitmapFormat::Mono1;
            return true;
        case 1:
            format = DisplayLayoutBitmapFormat::Gray8;
            return true;
        case 2:
            format = DisplayLayoutBitmapFormat::Rgb565;
            return true;
        default:
            return false;
        }
    }
    const char* text = value.as<const char*>();
    if (text == nullptr) {
        return false;
    }
    if (std::strcmp(text, "mono1") == 0) {
        format = DisplayLayoutBitmapFormat::Mono1;
    } else if (std::strcmp(text, "gray8") == 0) {
        format = DisplayLayoutBitmapFormat::Gray8;
    } else if (std::strcmp(text, "rgb565") == 0) {
        format = DisplayLayoutBitmapFormat::Rgb565;
    } else {
        return false;
    }
    return true;
}

uint8_t styleFlagsFromJson(const JsonObjectConst& input) {
    const JsonObjectConst styleFlags = input["styleFlags"].as<JsonObjectConst>();
    uint8_t flags = 0U;
    if (!styleFlags.isNull()) {
        if (readBool(styleFlags["filled"], false)) {
            flags |= 0x01U;
        }
        if (readBool(styleFlags["inverted"], false)) {
            flags |= 0x02U;
        }
        if (readBool(styleFlags["wrap"], false)) {
            flags |= 0x04U;
        }
    }
    return flags;
}

void writeStyleFlagsJson(const uint8_t styleFlags, JsonObject output) {
    output["filled"] = (styleFlags & 0x01U) != 0U;
    output["inverted"] = (styleFlags & 0x02U) != 0U;
    output["wrap"] = (styleFlags & 0x04U) != 0U;
}

bool parseWidget(const JsonObjectConst& widgetJson, DisplayLayoutWidgetV1& widget, const char*& error) {
    if (!copyText(widget.id, sizeof(widget.id), widgetJson["id"] | "")) {
        error = "display widget id is invalid";
        return false;
    }
    DisplayLayoutWidgetType type{};
    if (!parseWidgetType(widgetJson, type, error)) {
        return false;
    }
    widget.type = static_cast<uint8_t>(type);
    if (!parseBindingKind(widgetJson, widget.bindingKind)) {
        error = "display widget binding kind is invalid";
        return false;
    }
    if (!parseWidgetMetricNamespace(widgetJson, widget.metricNamespace)) {
        error = "display widget metric namespace is invalid";
        return false;
    }
    widget.x = widgetJson["x"] | 0U;
    widget.y = widgetJson["y"] | 0U;
    widget.width = widgetJson["width"] | 1U;
    widget.height = widgetJson["height"] | 1U;
    widget.sourceDeviceId = widgetJson["sourceDeviceId"] | 0UL;
    widget.metricId = widgetJson["metricId"] | 0L;
    widget.refreshIntervalMs = normalizeRefreshIntervalMs(widgetJson["refreshIntervalMs"] | 0UL);
    widget.fontSize = widgetJson["fontSize"] | 1U;
    widget.strokeWidth = widgetJson["strokeWidth"] | 1U;
    widget.autoSize = readBool(widgetJson["autoSize"], false) ? 1U : 0U;
    widget.styleFlags = styleFlagsFromJson(widgetJson);
    if (!copyText(widget.text, sizeof(widget.text), widgetJson["text"] | "")) {
        error = "display widget text is invalid";
        return false;
    }
    widget.bitmapFormat = static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1);
    widget.keepAspectRatio = readBool(widgetJson["keepAspectRatio"], false) ? 1U : 0U;
    widget.bitmapData.clear();
    if (type == DisplayLayoutWidgetType::Bitmap) {
        DisplayLayoutBitmapFormat bitmapFormat{};
        if (!parseBitmapFormat(widgetJson, bitmapFormat)) {
            error = "bitmap format is invalid";
            return false;
        }
        widget.bitmapFormat = static_cast<uint8_t>(bitmapFormat);
        const char* bitmapData = widgetJson["bitmapData"] | "";
        if (!copyStringToBytes(widget.bitmapData, bitmapData)) {
            error = "bitmap data exceeds supported size or is invalid";
            return false;
        }
    }
    return true;
}

bool validateWidget(const DisplayLayoutWidgetV1& widget) {
    const DisplayLayoutWidgetType type = normalizeWidgetType(static_cast<DisplayLayoutWidgetType>(widget.type));
    if (widget.type != static_cast<uint8_t>(type)) {
        return false;
    }
    if (widget.bindingKind != static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound) &&
        widget.bindingKind != static_cast<uint8_t>(DisplayLayoutBindingKind::Device) &&
        widget.bindingKind != static_cast<uint8_t>(DisplayLayoutBindingKind::Metric) &&
        widget.bindingKind != static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText)) {
        return false;
    }
    const MetricNamespace metricNamespace = normalizeMetricNamespace(static_cast<MetricNamespace>(widget.metricNamespace));
    if (widget.metricNamespace != static_cast<uint8_t>(metricNamespace)) {
        return false;
    }
    if (metricNamespace != MetricNamespace::Device && widget.sourceDeviceId != 0U) {
        return false;
    }
    if (widget.refreshIntervalMs != kDisplayLayoutRefreshIntervalDisabled &&
        (widget.refreshIntervalMs < kDisplayLayoutRefreshIntervalMinMs || widget.refreshIntervalMs > kDisplayLayoutRefreshIntervalMaxMs)) {
        return false;
    }
    if (widget.width == 0U || widget.height == 0U) {
        return false;
    }
    if (widget.fontSize == 0U || widget.fontSize > 8U || widget.strokeWidth == 0U || widget.strokeWidth > 32U) {
        return false;
    }
    if (widget.bitmapData.size() > kDisplayLayoutBitmapDataCapacity) {
        return false;
    }
    if (type != DisplayLayoutWidgetType::Bitmap && !widget.bitmapData.empty()) {
        return false;
    }
    if (type == DisplayLayoutWidgetType::Bitmap && widget.bitmapData.empty()) {
        return false;
    }
    if (type == DisplayLayoutWidgetType::Bitmap) {
        const DisplayLayoutBitmapFormat format = normalizeBitmapFormat(static_cast<DisplayLayoutBitmapFormat>(widget.bitmapFormat));
        if (widget.bitmapFormat != static_cast<uint8_t>(format)) {
            return false;
        }
    }
    return true;
}

bool validateLayout(const DisplayLayoutRecordV1& layout) {
    if ((layout.recordVersion != 1U && layout.recordVersion != 2U && layout.recordVersion != 3U &&
         layout.recordVersion != kDisplayLayoutRecordVersion) ||
        layout.schemaVersion != kDisplayLayoutSchemaVersion) {
        return false;
    }
    if (layout.pages.size() > kDisplayLayoutMaxPages) {
        return false;
    }
    if (layout.pages.empty()) {
        return false;
    }
    if (layout.activePageIndex >= layout.pages.size()) {
        return false;
    }
    for (const DisplayLayoutPageV1& page : layout.pages) {
        if (std::strlen(page.id) == 0U || page.widgets.size() > kDisplayLayoutMaxWidgetsPerPage) {
            return false;
        }
        if (std::strlen(page.name) == 0U) {
            return false;
        }
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            if (!validateWidget(widget)) {
                return false;
            }
        }
    }
    return true;
}

size_t estimateWidgetSize(const DisplayLayoutWidgetV1& widget) {
    return sizeof(DisplayLayoutBinaryWidgetV4) + widget.bitmapData.size();
}

void normalizePageOrder(DisplayLayoutRecordV1& layout) {
    std::sort(layout.pages.begin(), layout.pages.end(),
              [](const DisplayLayoutPageV1& left, const DisplayLayoutPageV1& right) { return left.order < right.order; });
    for (size_t index = 0; index < layout.pages.size(); ++index) {
        layout.pages[index].order = static_cast<uint8_t>(index);
    }
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
        pageJson["name"] = page.name;
        pageJson["order"] = page.order;
        JsonArray widgets = pageJson.createNestedArray("widgets");
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            JsonObject widgetJson = widgets.createNestedObject();
            widgetJson["type"] = widgetTypeToString(static_cast<DisplayLayoutWidgetType>(widget.type));
            widgetJson["id"] = widget.id;
            widgetJson["bindingKind"] = bindingKindToString(static_cast<DisplayLayoutBindingKind>(widget.bindingKind));
            widgetJson["metricNamespace"] = metricNamespaceToString(static_cast<MetricNamespace>(widget.metricNamespace));
            widgetJson["x"] = widget.x;
            widgetJson["y"] = widget.y;
            widgetJson["width"] = widget.width;
            widgetJson["height"] = widget.height;
            widgetJson["sourceDeviceId"] = widget.sourceDeviceId;
            widgetJson["metricId"] = widget.metricId;
            widgetJson["refreshIntervalMs"] = widget.refreshIntervalMs;
            widgetJson["fontSize"] = widget.fontSize;
            widgetJson["strokeWidth"] = widget.strokeWidth;
            widgetJson["autoSize"] = widget.autoSize != 0U;
            JsonObject styleFlags = widgetJson.createNestedObject("styleFlags");
            writeStyleFlagsJson(widget.styleFlags, styleFlags);
            widgetJson["text"] = widget.text;
            if (static_cast<DisplayLayoutWidgetType>(widget.type) == DisplayLayoutWidgetType::Bitmap) {
                widgetJson["bitmapFormat"] = bitmapFormatToString(static_cast<DisplayLayoutBitmapFormat>(widget.bitmapFormat));
                widgetJson["keepAspectRatio"] = widget.keepAspectRatio != 0U;
                const std::string bitmapData = bytesToString(widget.bitmapData);
                widgetJson["bitmapData"] = JsonString(bitmapData.c_str(), JsonString::Copied);
            }
        }
    }
}

bool parseDisplayLayoutJson(const JsonObjectConst& input, DisplayLayoutRecordV1& layout) {
    layout = {};
    layout.deviceId = 0;
    layout.recordVersion = kDisplayLayoutRecordVersion;
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
        if (!copyText(page.name, sizeof(page.name), pageJson["name"] | page.id)) {
            return false;
        }
        page.order = pageJson["order"] | static_cast<uint8_t>(layout.pages.size());
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
                const char* error = nullptr;
                if (!parseWidget(widgetJson, widget, error)) {
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

    normalizePageOrder(layout);

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
    if (!validateLayout(layout)) {
        return false;
    }
    return prepareDisplayLayoutTextAst(layout);
}

bool encodeDisplayLayoutBinary(const DisplayLayoutRecordV1& layout, std::vector<uint8_t>& blob) {
    if (!validateLayout(layout)) {
        return false;
    }

    blob.clear();
    size_t estimatedSize = sizeof(DisplayLayoutBinaryHeaderV1);
    for (const DisplayLayoutPageV1& page : layout.pages) {
        estimatedSize += sizeof(DisplayLayoutBinaryPageHeaderV1);
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            estimatedSize += estimateWidgetSize(widget);
        }
    }
    blob.reserve(estimatedSize);

    DisplayLayoutBinaryHeaderV1 header{};
    header.recordVersion = kDisplayLayoutRecordVersion;
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
        if (!copyText(pageHeader.name, sizeof(pageHeader.name), page.name)) {
            return false;
        }
        pageHeader.order = page.order;
        if (!appendBinary(blob, pageHeader)) {
            return false;
        }
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            DisplayLayoutBinaryWidgetV4 binaryWidget{};
            if (!copyText(binaryWidget.id, sizeof(binaryWidget.id), widget.id)) {
                return false;
            }
            binaryWidget.type = widget.type;
            binaryWidget.bindingKind = widget.bindingKind;
            binaryWidget.metricNamespace = widget.metricNamespace;
            binaryWidget.x = widget.x;
            binaryWidget.y = widget.y;
            binaryWidget.width = widget.width;
            binaryWidget.height = widget.height;
            binaryWidget.sourceDeviceId = widget.sourceDeviceId;
            binaryWidget.metricId = widget.metricId;
            binaryWidget.refreshIntervalMs = widget.refreshIntervalMs;
            binaryWidget.fontSize = widget.fontSize;
            binaryWidget.strokeWidth = widget.strokeWidth;
            binaryWidget.autoSize = widget.autoSize;
            binaryWidget.styleFlags = widget.styleFlags;
            binaryWidget.bitmapFormat = widget.bitmapFormat;
            binaryWidget.keepAspectRatio = widget.keepAspectRatio;
            binaryWidget.bitmapDataLength = static_cast<uint16_t>(widget.bitmapData.size());
            if (!copyText(binaryWidget.text, sizeof(binaryWidget.text), widget.text)) {
                return false;
            }
            if (!appendBinary(blob, binaryWidget)) {
                return false;
            }
            if (!appendBytes(blob, widget.bitmapData.data(), widget.bitmapData.size())) {
                return false;
            }
        }
    }
    return blob.size() <= kMaxDisplayLayoutBytes;
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
    if ((header.recordVersion != 1U && header.recordVersion != 2U && header.recordVersion != 3U &&
         header.recordVersion != kDisplayLayoutRecordVersion) ||
        header.schemaVersion != kDisplayLayoutSchemaVersion) {
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
        if (!copyText(page.name, sizeof(page.name), pageHeader.name)) {
            return false;
        }
        page.order = pageHeader.order;
        page.widgets.reserve(pageHeader.widgetCount);
        for (uint8_t widgetIndex = 0; widgetIndex < pageHeader.widgetCount; ++widgetIndex) {
            DisplayLayoutBinaryWidgetV4 binaryWidget{};
            if (header.recordVersion == 1U) {
                DisplayLayoutBinaryWidgetV1 legacyWidget{};
                if (!readBinary(data, size, offset, legacyWidget)) {
                    return false;
                }
                if (!copyText(binaryWidget.id, sizeof(binaryWidget.id), legacyWidget.id)) {
                    return false;
                }
                binaryWidget.type = legacyWidget.type;
                binaryWidget.bindingKind = legacyWidget.bindingKind;
                binaryWidget.metricNamespace = static_cast<uint8_t>(MetricNamespace::Device);
                binaryWidget.x = legacyWidget.x;
                binaryWidget.y = legacyWidget.y;
                binaryWidget.width = legacyWidget.width;
                binaryWidget.height = legacyWidget.height;
                binaryWidget.sourceDeviceId = legacyWidget.sourceDeviceId;
                binaryWidget.metricId = legacyWidget.metricId;
                binaryWidget.refreshIntervalMs = kDisplayLayoutRefreshIntervalDisabled;
                binaryWidget.fontSize = legacyWidget.fontSize;
                binaryWidget.strokeWidth = legacyWidget.strokeWidth;
                binaryWidget.autoSize = legacyWidget.autoSize;
                binaryWidget.styleFlags = legacyWidget.styleFlags;
                binaryWidget.bitmapFormat = legacyWidget.bitmapFormat;
                binaryWidget.keepAspectRatio = legacyWidget.keepAspectRatio;
                binaryWidget.bitmapDataLength = legacyWidget.bitmapDataLength;
                if (!copyText(binaryWidget.text, sizeof(binaryWidget.text), legacyWidget.text)) {
                    return false;
                }
            } else if (header.recordVersion == 2U) {
                DisplayLayoutBinaryWidgetV2 legacyWidget{};
                if (!readBinary(data, size, offset, legacyWidget)) {
                    return false;
                }
                if (!copyText(binaryWidget.id, sizeof(binaryWidget.id), legacyWidget.id)) {
                    return false;
                }
                binaryWidget.type = legacyWidget.type;
                binaryWidget.bindingKind = legacyWidget.bindingKind;
                binaryWidget.metricNamespace = legacyWidget.metricNamespace;
                binaryWidget.x = legacyWidget.x;
                binaryWidget.y = legacyWidget.y;
                binaryWidget.width = legacyWidget.width;
                binaryWidget.height = legacyWidget.height;
                binaryWidget.sourceDeviceId = legacyWidget.sourceDeviceId;
                binaryWidget.metricId = legacyWidget.metricId;
                binaryWidget.refreshIntervalMs = kDisplayLayoutRefreshIntervalDisabled;
                binaryWidget.fontSize = legacyWidget.fontSize;
                binaryWidget.strokeWidth = legacyWidget.strokeWidth;
                binaryWidget.autoSize = legacyWidget.autoSize;
                binaryWidget.styleFlags = legacyWidget.styleFlags;
                binaryWidget.bitmapFormat = legacyWidget.bitmapFormat;
                binaryWidget.keepAspectRatio = legacyWidget.keepAspectRatio;
                binaryWidget.bitmapDataLength = legacyWidget.bitmapDataLength;
                if (!copyText(binaryWidget.text, sizeof(binaryWidget.text), legacyWidget.text)) {
                    return false;
                }
            } else {
                if (!readBinary(data, size, offset, binaryWidget)) {
                    return false;
                }
            }
            if (binaryWidget.bitmapDataLength > kDisplayLayoutBitmapDataCapacity) {
                return false;
            }
            DisplayLayoutWidgetV1 widget{};
            if (!copyText(widget.id, sizeof(widget.id), binaryWidget.id)) {
                return false;
            }
            widget.type = binaryWidget.type;
            widget.bindingKind = binaryWidget.bindingKind;
            widget.metricNamespace = binaryWidget.metricNamespace;
            widget.x = binaryWidget.x;
            widget.y = binaryWidget.y;
            widget.width = binaryWidget.width;
            widget.height = binaryWidget.height;
            widget.sourceDeviceId = binaryWidget.sourceDeviceId;
            widget.metricId = binaryWidget.metricId;
            widget.refreshIntervalMs = binaryWidget.refreshIntervalMs;
            widget.fontSize = binaryWidget.fontSize;
            widget.strokeWidth = binaryWidget.strokeWidth;
            widget.autoSize = binaryWidget.autoSize;
            widget.styleFlags = binaryWidget.styleFlags;
            widget.bitmapFormat = binaryWidget.bitmapFormat;
            widget.keepAspectRatio = binaryWidget.keepAspectRatio;
            if (!copyText(widget.text, sizeof(widget.text), binaryWidget.text)) {
                return false;
            }
            if (!readBytes(data, size, offset, binaryWidget.bitmapDataLength, widget.bitmapData)) {
                return false;
            }
            page.widgets.push_back(widget);
        }
        layout.pages.push_back(page);
    }

    normalizePageOrder(layout);
    if (offset != size || !validateLayout(layout)) {
        return false;
    }
    return prepareDisplayLayoutTextAst(layout);
}

} // namespace ewfm
