#include "devices/display/DisplayTextPlaceholderAst.h"

#include "devices/display/DisplayTextEvaluator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"
#include "devices/switch/OutputState.h"
#include "metrics/MetricValueResolver.h"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace ewfm {

namespace {

struct ParsedPlaceholder {
    MetricNamespace ns{MetricNamespace::Device};
    DeviceId sourceId{0};
    int32_t metricId{0};
    DisplayTextFilterKind filter{DisplayTextFilterKind::None};
};

class ResolverTextValueSource final : public IDisplayTextValueSource {
public:
    ResolverTextValueSource(const MetricNamespace ns, const int32_t metricId) : ns_(ns), metricId_(metricId) {}

    bool get(const MetricValueResolver& resolver, MetricValue& value) const override {
        return resolver.resolve(ns_, 0U, metricId_, value);
    }

private:
    MetricNamespace ns_;
    int32_t metricId_;
};

class DeviceTextValueSource final : public IDisplayTextValueSource {
public:
    DeviceTextValueSource(const IDeviceRuntime* runtime, const MetricNamespace ns, const int32_t metricId)
        : runtime_(runtime), ns_(ns), metricId_(metricId) {}

    bool get(const MetricValueResolver& resolver, MetricValue& value) const override {
        (void)resolver;
        if (runtime_ == nullptr) {
            return false;
        }
        switch (ns_) {
        case MetricNamespace::Device:
            switch (metricId_) {
            case kDeviceMetricTemperature: {
                const ITemperatureReadingRuntime* temperature = runtime_->temperatureReadingRuntime();
                if (temperature == nullptr) {
                    return false;
                }
                TemperatureReading reading{};
                if (!temperature->latestTemperatureReading(reading) || !reading.valid) {
                    return false;
                }
                value.valueType = MetricValueType::Float;
                value.number.floatValue = static_cast<float>(reading.milliCelsius) / 1000.0f;
                std::snprintf(value.text, sizeof(value.text), "%.2f C", static_cast<double>(value.number.floatValue));
                return true;
            }
            case kDeviceMetricSwitchState: {
                const ISwitchOutputRuntime* switchOutput = runtime_->switchOutputRuntime();
                if (switchOutput == nullptr) {
                    return false;
                }
                std::snprintf(value.text, sizeof(value.text), "%s", outputStateName(switchOutput->currentOutputState()));
                value.valueType = MetricValueType::String;
                return true;
            }
            default:
                return false;
            }
        default:
            return false;
        }
    }

private:
    const IDeviceRuntime* runtime_;
    MetricNamespace ns_;
    int32_t metricId_;
};

uint32_t hashText(std::string_view text) {
    uint32_t hash = 2166136261U;
    for (const unsigned char ch : text) {
        hash ^= ch;
        hash *= 16777619U;
    }
    return hash;
}

bool appendText(char* dest, const size_t capacity, size_t& length, const char* value, const size_t valueLength, bool& truncated) {
    if (dest == nullptr || capacity == 0U || value == nullptr) {
        return false;
    }
    const size_t available = capacity - 1U;
    if (length >= available) {
        truncated = valueLength > 0U;
        dest[available] = '\0';
        return !truncated;
    }
    const size_t copyLength = valueLength <= available - length ? valueLength : available - length;
    if (copyLength > 0U) {
        std::memcpy(dest + length, value, copyLength);
        length += copyLength;
        dest[length] = '\0';
    }
    if (copyLength < valueLength) {
        truncated = true;
    }
    return !truncated;
}

bool parseUnsigned(const char* begin, const char* end, DeviceId& value) {
    if (begin == nullptr || end == nullptr || begin >= end) {
        return false;
    }
    uint32_t parsed = 0U;
    for (const char* cursor = begin; cursor < end; ++cursor) {
        if (!std::isdigit(static_cast<unsigned char>(*cursor))) {
            return false;
        }
        parsed = parsed * 10U + static_cast<uint32_t>(*cursor - '0');
    }
    if (parsed == 0U) {
        return false;
    }
    value = parsed;
    return true;
}

bool metricKeyToId(const MetricNamespace ns, const char* key, const size_t keyLength, int32_t& metricId) {
    if (key == nullptr || keyLength == 0U) {
        return false;
    }
    const auto equals = [&](const char* expected) {
        return std::strlen(expected) == keyLength && std::strncmp(key, expected, keyLength) == 0;
    };
    switch (ns) {
    case MetricNamespace::Device:
        if (equals("temperature")) {
            metricId = kDeviceMetricTemperature;
            return true;
        }
        if (equals("state")) {
            metricId = kDeviceMetricSwitchState;
            return true;
        }
        return false;
    case MetricNamespace::System:
        if (equals("time")) {
            metricId = kSystemMetricTime;
            return true;
        }
        if (equals("uptime")) {
            metricId = kSystemMetricUptime;
            return true;
        }
        if (equals("wifi.station_ip")) {
            metricId = kSystemMetricWifiStationIp;
            return true;
        }
        if (equals("wifi.setup_ap_ip")) {
            metricId = kSystemMetricWifiSetupApIp;
            return true;
        }
        return false;
    case MetricNamespace::Wifi:
        return false;
    }
    return false;
}

bool parseFilterName(const char* begin, const char* end, DisplayTextFilterKind& filter) {
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    if (begin >= end) {
        filter = DisplayTextFilterKind::None;
        return true;
    }

    const size_t length = static_cast<size_t>(end - begin);
    const auto equals = [&](const char* expected) { return std::strlen(expected) == length && std::strncmp(begin, expected, length) == 0; };
    if (equals("text")) {
        filter = DisplayTextFilterKind::Text;
        return true;
    }
    if (equals("upper")) {
        filter = DisplayTextFilterKind::Upper;
        return true;
    }
    if (equals("lower")) {
        filter = DisplayTextFilterKind::Lower;
        return true;
    }
    if (equals("trim")) {
        filter = DisplayTextFilterKind::Trim;
        return true;
    }
    return false;
}

DisplayTextValueSourcePtr makeValueSource(const DisplayTextPlaceholderSegment& placeholder, const DeviceRegistry& registry) {
    switch (placeholder.ns) {
    case MetricNamespace::Device: {
        const IDeviceRuntime* runtime = registry.runtime(placeholder.sourceId);
        if (runtime == nullptr) {
            return {};
        }
        return std::make_shared<DeviceTextValueSource>(runtime, placeholder.ns, placeholder.metricId);
    }
    case MetricNamespace::System:
        return std::make_shared<ResolverTextValueSource>(placeholder.ns, placeholder.metricId);
    case MetricNamespace::Wifi:
        return {};
    }
    return {};
}

bool parsePlaceholderBody(const char* begin, const char* end, ParsedPlaceholder& parsed, DisplayTextCompileStatus& status) {
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    const char* pipe = static_cast<const char*>(std::memchr(begin, '|', static_cast<size_t>(end - begin)));
    const char* exprEnd = pipe != nullptr ? pipe : end;
    const char* filterBegin = pipe != nullptr ? pipe + 1 : end;
    while (exprEnd > begin && std::isspace(static_cast<unsigned char>(*(exprEnd - 1)))) {
        --exprEnd;
    }
    if (pipe != nullptr && std::memchr(pipe + 1, '|', static_cast<size_t>(end - pipe - 1)) != nullptr) {
        status = DisplayTextCompileStatus::MalformedSyntax;
        return false;
    }
    const char* firstDot = static_cast<const char*>(std::memchr(begin, '.', static_cast<size_t>(end - begin)));
    if (firstDot == nullptr) {
        status = DisplayTextCompileStatus::MalformedSyntax;
        return false;
    }
    if (firstDot >= exprEnd) {
        status = DisplayTextCompileStatus::MalformedSyntax;
        return false;
    }

    MetricNamespace ns{};
    if (std::strncmp(begin, "dev", static_cast<size_t>(firstDot - begin)) == 0 && firstDot - begin == 3) {
        ns = MetricNamespace::Device;
    } else if (std::strncmp(begin, "system", static_cast<size_t>(firstDot - begin)) == 0 && firstDot - begin == 6) {
        ns = MetricNamespace::System;
    } else {
        status = DisplayTextCompileStatus::UnknownNamespace;
        return false;
    }

    DeviceId sourceId = 0U;
    const char* metricKey = firstDot + 1;
    if (ns == MetricNamespace::Device) {
        const char* secondDot = static_cast<const char*>(std::memchr(metricKey, '.', static_cast<size_t>(exprEnd - metricKey)));
        if (secondDot == nullptr) {
            status = DisplayTextCompileStatus::MalformedSyntax;
            return false;
        }
        if (!parseUnsigned(metricKey, secondDot, sourceId)) {
            status = DisplayTextCompileStatus::MissingDeviceId;
            return false;
        }
        metricKey = secondDot + 1;
    }

    int32_t metricId = 0;
    if (!metricKeyToId(ns, metricKey, static_cast<size_t>(exprEnd - metricKey), metricId)) {
        status = DisplayTextCompileStatus::UnknownMetric;
        return false;
    }
    DisplayTextFilterKind filter = DisplayTextFilterKind::None;
    if (!parseFilterName(filterBegin, end, filter)) {
        status = DisplayTextCompileStatus::MalformedSyntax;
        return false;
    }
    parsed.ns = ns;
    parsed.sourceId = sourceId;
    parsed.metricId = metricId;
    parsed.filter = filter;
    return true;
}

const char* findPlaceholderEnd(const char* begin) {
    if (begin == nullptr) {
        return nullptr;
    }
    return std::strstr(begin, "}}");
}

bool applyFilter(char* text, const size_t capacity, const DisplayTextFilterKind filter) {
    if (text == nullptr || capacity == 0U) {
        return false;
    }
    switch (filter) {
    case DisplayTextFilterKind::None:
    case DisplayTextFilterKind::Text:
        return true;
    case DisplayTextFilterKind::Upper:
        for (size_t index = 0; text[index] != '\0'; ++index) {
            text[index] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[index])));
        }
        return true;
    case DisplayTextFilterKind::Lower:
        for (size_t index = 0; text[index] != '\0'; ++index) {
            text[index] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[index])));
        }
        return true;
    case DisplayTextFilterKind::Trim: {
        const size_t total = std::strlen(text);
        size_t begin = 0U;
        size_t end = total;
        while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) {
            ++begin;
        }
        while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1U]))) {
            --end;
        }
        if (begin > 0U) {
            std::memmove(text, text + begin, end - begin);
        }
        text[end - begin] = '\0';
        return true;
    }
    }
    return false;
}

void appendResolvedText(std::string_view sourceText, const DisplayTextCompiledWidget& compiled, const MetricValueResolver& resolver,
                        DisplayTextEvaluationResult& result) {
    bool truncated = false;
    bool hadPlaceholder = false;
    bool hadInvalidPlaceholder = false;
    bool hadMissingMetric = false;
    size_t length = 0U;
    result.text[0] = '\0';

    for (const DisplayTextSegment& segment : compiled.segments) {
        if (!segment.placeholder) {
            const std::string_view literal =
                segment.literal.length == 0U ? std::string_view{} : sourceText.substr(segment.literal.offset, segment.literal.length);
            appendText(result.text, sizeof(result.text), length, literal.data(), literal.size(), truncated);
            continue;
        }

        hadPlaceholder = true;
        if (!segment.valid) {
            hadInvalidPlaceholder = true;
            continue;
        }
        MetricValue value{};
        bool resolved = false;
        if (segment.source != nullptr) {
            resolved = segment.source->get(resolver, value) && metricValueHasValue(value);
        } else {
            resolved = resolver.resolve(segment.reference.ns, segment.reference.sourceId, segment.reference.metricId, value) &&
                       metricValueHasValue(value);
        }
        if (resolved) {
            char filtered[kDisplayEvaluatedTextCapacity]{};
            std::snprintf(filtered, sizeof(filtered), "%s", value.text);
            (void)applyFilter(filtered, sizeof(filtered), segment.filter);
            appendText(result.text, sizeof(result.text), length, filtered, std::strlen(filtered), truncated);
        } else {
            hadMissingMetric = true;
            appendText(result.text, sizeof(result.text), length, "", 0U, truncated);
        }
    }

    result.dynamic = hadPlaceholder;
    result.available = !truncated;
    if (truncated) {
        result.status = DisplayTextEvaluationStatus::Truncated;
        return;
    }
    if (hadInvalidPlaceholder) {
        result.status = DisplayTextEvaluationStatus::InvalidPlaceholder;
        return;
    }
    if (hadMissingMetric) {
        result.status = DisplayTextEvaluationStatus::MissingMetric;
        return;
    }
    result.status = hadPlaceholder ? DisplayTextEvaluationStatus::Resolved : DisplayTextEvaluationStatus::Static;
}

} // namespace

DisplayTextCompileResult compileDisplayTextWidget(std::string_view text) {
    DisplayTextCompileResult result{};
    result.compiled.sourceHash = hashText(text);

    const char* cursor = text.data();
    const char* end = text.data() + text.size();
    if (text.empty()) {
        result.compiled.segments.push_back(DisplayTextSegment{false, true, DisplayTextLiteralSpan{0U, 0U}, {}});
        return result;
    }

    while (cursor < end) {
        const char* placeholderStart = std::strstr(cursor, "{{");
        if (placeholderStart == nullptr || placeholderStart >= end) {
            result.compiled.segments.push_back(DisplayTextSegment{
                false, true, DisplayTextLiteralSpan{static_cast<uint16_t>(cursor - text.data()), static_cast<uint16_t>(end - cursor)}, {}});
            break;
        }

        if (placeholderStart > cursor) {
            result.compiled.segments.push_back(DisplayTextSegment{
                false,
                true,
                DisplayTextLiteralSpan{static_cast<uint16_t>(cursor - text.data()), static_cast<uint16_t>(placeholderStart - cursor)},
                {}});
        }

        const char* placeholderEnd = findPlaceholderEnd(placeholderStart + 2);
        if (placeholderEnd == nullptr || placeholderEnd > end) {
            result.status = DisplayTextCompileStatus::MalformedSyntax;
            result.compiled.segments.clear();
            return result;
        }

        ParsedPlaceholder parsed{};
        DisplayTextCompileStatus status = DisplayTextCompileStatus::Ok;
        if (!parsePlaceholderBody(placeholderStart + 2, placeholderEnd, parsed, status)) {
            if (status == DisplayTextCompileStatus::MalformedSyntax) {
                result.status = status;
                result.compiled.segments.clear();
                return result;
            }
            DisplayTextSegment segment{};
            segment.placeholder = true;
            segment.valid = false;
            result.compiled.segments.push_back(segment);
            cursor = placeholderEnd + 2;
            continue;
        }

        DisplayTextSegment segment{};
        segment.placeholder = true;
        segment.valid = true;
        segment.reference = {parsed.ns, parsed.sourceId, parsed.metricId};
        segment.filter = parsed.filter;
        result.compiled.segments.push_back(segment);
        cursor = placeholderEnd + 2;
    }

    if (result.compiled.segments.empty()) {
        result.compiled.segments.push_back(
            DisplayTextSegment{false, true, DisplayTextLiteralSpan{0U, static_cast<uint16_t>(text.size())}, {}});
    }
    return result;
}

bool ensureDisplayTextWidgetAst(const DisplayLayoutWidgetV1& widget, const DisplayTextCompiledWidget*& compiled,
                                DisplayTextCompileStatus* status) {
    compiled = nullptr;
    const uint32_t sourceHash = hashText(widget.text);
    if (!widget.textAst.has_value() || widget.textAst->sourceHash != sourceHash) {
        const DisplayTextCompileResult result = compileDisplayTextWidget(widget.text);
        if (!result.ok()) {
            widget.textAst.reset();
            if (status != nullptr) {
                *status = result.status;
            }
            return false;
        }
        widget.textAst = result.compiled;
    }
    compiled = &*widget.textAst;
    if (status != nullptr) {
        *status = DisplayTextCompileStatus::Ok;
    }
    return true;
}

bool evaluateDisplayTextWidget(std::string_view sourceText, const DisplayTextCompiledWidget& compiled, const MetricValueResolver& resolver,
                               DisplayTextEvaluationResult& result) {
    result = {};
    if (compiled.segments.empty()) {
        result.status = DisplayTextEvaluationStatus::Static;
        result.available = true;
        result.text[0] = '\0';
        return true;
    }
    appendResolvedText(sourceText, compiled, resolver, result);
    return true;
}

bool evaluateDisplayTextWidget(const DisplayLayoutWidgetV1& widget, const MetricValueResolver& resolver,
                               DisplayTextEvaluationResult& result) {
    result = {};
    if (static_cast<DisplayLayoutWidgetType>(widget.type) != DisplayLayoutWidgetType::Text) {
        result.available = false;
        result.status = DisplayTextEvaluationStatus::InvalidWidget;
        return false;
    }
    if (widget.bindingKind == static_cast<uint8_t>(DisplayLayoutBindingKind::Metric) && widget.text[0] == '\0') {
        MetricValue value{};
        const bool resolved =
            resolver.resolve(static_cast<MetricNamespace>(widget.metricNamespace), widget.sourceDeviceId, widget.metricId, value) &&
            metricValueHasValue(value);
        result.dynamic = true;
        result.available = true;
        if (resolved) {
            result.status = DisplayTextEvaluationStatus::Resolved;
            std::snprintf(result.text, sizeof(result.text), "%s", value.text);
        } else {
            result.status = DisplayTextEvaluationStatus::MissingMetric;
            result.text[0] = '\0';
        }
        return true;
    }
    const DisplayTextCompiledWidget* compiled = nullptr;
    DisplayTextCompileStatus status = DisplayTextCompileStatus::Ok;
    if (!ensureDisplayTextWidgetAst(widget, compiled, &status)) {
        result.available = true;
        result.status = DisplayTextEvaluationStatus::InvalidPlaceholder;
        result.text[0] = '\0';
        return true;
    }
    return evaluateDisplayTextWidget(widget.text, *compiled, resolver, result);
}

bool displayTextPlaceholderSupportsRuntime(const IDeviceRuntime& runtime, const DisplayTextPlaceholderSegment& placeholder) {
    if (placeholder.ns != MetricNamespace::Device) {
        return true;
    }
    switch (placeholder.metricId) {
    case kDeviceMetricTemperature:
        return runtime.temperatureReadingRuntime() != nullptr;
    case kDeviceMetricSwitchState:
        return runtime.switchOutputRuntime() != nullptr;
    default:
        return false;
    }
}

bool displayTextWidgetUsesStructuredPlaceholders(const DisplayLayoutWidgetV1& widget) {
    if (static_cast<DisplayLayoutWidgetType>(widget.type) != DisplayLayoutWidgetType::Text) {
        return false;
    }
    return std::strstr(widget.text, "{{") != nullptr;
}

bool prepareDisplayLayoutTextAst(const DisplayLayoutRecordV1& layout) {
    for (const DisplayLayoutPageV1& page : layout.pages) {
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            if (static_cast<DisplayLayoutWidgetType>(widget.type) != DisplayLayoutWidgetType::Text) {
                continue;
            }
            const DisplayTextCompiledWidget* compiled = nullptr;
            if (!ensureDisplayTextWidgetAst(widget, compiled, nullptr)) {
                return false;
            }
        }
    }
    return true;
}

bool bindDisplayLayoutTextAst(const DisplayLayoutRecordV1& layout, const DeviceRegistry& registry) {
    for (const DisplayLayoutPageV1& page : layout.pages) {
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            if (static_cast<DisplayLayoutWidgetType>(widget.type) != DisplayLayoutWidgetType::Text) {
                continue;
            }
            const DisplayTextCompiledWidget* compiled = nullptr;
            if (!ensureDisplayTextWidgetAst(widget, compiled, nullptr) || compiled == nullptr) {
                continue;
            }
            DisplayTextCompiledWidget bound = *compiled;
            for (DisplayTextSegment& segment : bound.segments) {
                if (!segment.placeholder || !segment.valid) {
                    continue;
                }
                segment.source = makeValueSource(segment.reference, registry);
                if (segment.source == nullptr) {
                    return false;
                }
            }
            widget.textAst = std::move(bound);
        }
    }
    return true;
}

DeviceValidationResult validateDisplayTextWidget(const DisplayLayoutWidgetV1& widget, const DeviceRegistry& registry) {
    if (static_cast<DisplayLayoutWidgetType>(widget.type) != DisplayLayoutWidgetType::Text) {
        return {DeviceError::InvalidConfig, "display text widget is invalid"};
    }

    const DisplayTextCompiledWidget* compiled = nullptr;
    DisplayTextCompileStatus status = DisplayTextCompileStatus::Ok;
    if (!ensureDisplayTextWidgetAst(widget, compiled, &status)) {
        switch (status) {
        case DisplayTextCompileStatus::MalformedSyntax:
            return {DeviceError::InvalidConfig, "display text placeholder syntax is invalid"};
        case DisplayTextCompileStatus::UnknownNamespace:
            return {DeviceError::InvalidConfig, "display text placeholder namespace is invalid"};
        case DisplayTextCompileStatus::MissingDeviceId:
            return {DeviceError::InvalidConfig, "display text placeholder device id is invalid"};
        case DisplayTextCompileStatus::UnknownMetric:
            return {DeviceError::InvalidConfig, "display text placeholder metric is invalid"};
        case DisplayTextCompileStatus::InvalidWidget:
        case DisplayTextCompileStatus::Ok:
            break;
        }
        return {DeviceError::InvalidConfig, "display text placeholder syntax is invalid"};
    }

    for (const DisplayTextSegment& segment : compiled->segments) {
        if (!segment.placeholder) {
            continue;
        }
        if (!segment.valid) {
            return {DeviceError::InvalidConfig, "display text placeholder syntax is invalid"};
        }
        if (segment.reference.ns != MetricNamespace::Device) {
            continue;
        }
        const IDeviceRuntime* runtime = registry.runtime(segment.reference.sourceId);
        if (runtime == nullptr) {
            return {DeviceError::InvalidRelationship, "display text source device is missing"};
        }
        if (!displayTextPlaceholderSupportsRuntime(*runtime, segment.reference)) {
            return {DeviceError::InvalidRelationship, "display text placeholder metric is invalid for source device"};
        }
    }

    return {};
}

} // namespace ewfm
