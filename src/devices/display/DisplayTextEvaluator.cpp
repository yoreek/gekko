#include "devices/display/DisplayTextEvaluator.h"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace ewfm {

namespace {

struct ParsedPlaceholder {
    MetricNamespace ns{MetricNamespace::Device};
    DeviceId sourceId{0};
    int32_t metricId{0};
};

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

bool copyEvaluatedText(DisplayTextEvaluationResult& result, const char* value) {
    result.text[0] = '\0';
    bool truncated = false;
    size_t length = 0U;
    appendText(result.text, sizeof(result.text), length, value != nullptr ? value : "", std::strlen(value != nullptr ? value : ""),
               truncated);
    if (truncated) {
        result.status = DisplayTextEvaluationStatus::Truncated;
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
    auto equals = [&](const char* expected) { return std::strlen(expected) == keyLength && std::strncmp(key, expected, keyLength) == 0; };
    switch (ns) {
    case MetricNamespace::Device:
        if (equals("status")) {
            metricId = kDeviceMetricStatus;
            return true;
        }
        if (equals("effective_status")) {
            metricId = kDeviceMetricEffectiveStatus;
            return true;
        }
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
        return false;
    case MetricNamespace::Wifi:
        if (equals("status")) {
            metricId = kWifiMetricStatus;
            return true;
        }
        if (equals("station_ip")) {
            metricId = kWifiMetricStationIp;
            return true;
        }
        if (equals("setup_ap_ip")) {
            metricId = kWifiMetricSetupApIp;
            return true;
        }
        return false;
    }
    return false;
}

bool parsePlaceholderBody(const char* begin, const char* end, ParsedPlaceholder& parsed) {
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    const char* firstDot = static_cast<const char*>(std::memchr(begin, '.', static_cast<size_t>(end - begin)));
    if (firstDot == nullptr) {
        return false;
    }

    MetricNamespace ns{};
    if (std::strncmp(begin, "dev", static_cast<size_t>(firstDot - begin)) == 0 && firstDot - begin == 3) {
        ns = MetricNamespace::Device;
    } else if (std::strncmp(begin, "system", static_cast<size_t>(firstDot - begin)) == 0 && firstDot - begin == 6) {
        ns = MetricNamespace::System;
    } else if (std::strncmp(begin, "wifi", static_cast<size_t>(firstDot - begin)) == 0 && firstDot - begin == 4) {
        ns = MetricNamespace::Wifi;
    } else {
        return false;
    }

    DeviceId sourceId = 0U;
    const char* metricKey = firstDot + 1;
    if (ns == MetricNamespace::Device) {
        const char* secondDot = static_cast<const char*>(std::memchr(metricKey, '.', static_cast<size_t>(end - metricKey)));
        if (secondDot == nullptr || !parseUnsigned(metricKey, secondDot, sourceId)) {
            return false;
        }
        metricKey = secondDot + 1;
    }
    int32_t metricId = 0;
    if (!metricKeyToId(ns, metricKey, static_cast<size_t>(end - metricKey), metricId)) {
        return false;
    }
    parsed.ns = ns;
    parsed.sourceId = sourceId;
    parsed.metricId = metricId;
    return true;
}

const char* findPlaceholderEnd(const char* begin) {
    if (begin == nullptr) {
        return nullptr;
    }
    return std::strstr(begin, "}}");
}

const char* findPlaceholderStart(const char* text) {
    if (text == nullptr) {
        return nullptr;
    }
    return std::strstr(text, "{{");
}

bool resolveFromWidgetBinding(const DisplayLayoutWidgetV1& widget, const MetricValueResolver& resolver, MetricValue& value) {
    return resolver.resolve(static_cast<MetricNamespace>(widget.metricNamespace), widget.sourceDeviceId, widget.metricId, value);
}

} // namespace

bool evaluateDisplayTextWidget(const DisplayLayoutWidgetV1& widget, const MetricValueResolver& resolver,
                               DisplayTextEvaluationResult& result) {
    result = {};
    if (static_cast<DisplayLayoutWidgetType>(widget.type) != DisplayLayoutWidgetType::Text) {
        result.available = false;
        result.status = DisplayTextEvaluationStatus::InvalidWidget;
        return false;
    }

    const char* text = widget.text;
    const char* placeholderStart = findPlaceholderStart(text);
    if (placeholderStart == nullptr) {
        if (static_cast<DisplayLayoutBindingKind>(widget.bindingKind) == DisplayLayoutBindingKind::Metric) {
            MetricValue value{};
            result.dynamic = true;
            if (!resolveFromWidgetBinding(widget, resolver, value) || !value.available) {
                result.available = false;
                result.status = DisplayTextEvaluationStatus::MissingMetric;
                result.text[0] = '\0';
                return false;
            }
            result.available = true;
            result.status = DisplayTextEvaluationStatus::Resolved;
            return copyEvaluatedText(result, value.text);
        }
        result.status = DisplayTextEvaluationStatus::Static;
        result.available = true;
        return copyEvaluatedText(result, text);
    }

    const char* placeholderEnd = findPlaceholderEnd(placeholderStart + 2);
    if (placeholderEnd == nullptr) {
        result.available = false;
        result.status = DisplayTextEvaluationStatus::InvalidPlaceholder;
        (void)copyEvaluatedText(result, text);
        return false;
    }
    if (findPlaceholderStart(placeholderEnd + 2) != nullptr) {
        result.available = false;
        result.status = DisplayTextEvaluationStatus::TooManyPlaceholders;
        (void)copyEvaluatedText(result, text);
        return false;
    }

    ParsedPlaceholder parsed{};
    if (!parsePlaceholderBody(placeholderStart + 2, placeholderEnd, parsed)) {
        result.available = false;
        result.status = DisplayTextEvaluationStatus::InvalidPlaceholder;
        (void)copyEvaluatedText(result, text);
        return false;
    }

    MetricValue value{};
    result.dynamic = true;
    const bool resolved = resolver.resolve(parsed.ns, parsed.sourceId, parsed.metricId, value) && value.available;
    result.available = resolved;

    result.text[0] = '\0';
    bool truncated = false;
    size_t length = 0U;
    appendText(result.text, sizeof(result.text), length, text, static_cast<size_t>(placeholderStart - text), truncated);
    if (resolved) {
        appendText(result.text, sizeof(result.text), length, value.text, std::strlen(value.text), truncated);
    }
    appendText(result.text, sizeof(result.text), length, placeholderEnd + 2, std::strlen(placeholderEnd + 2), truncated);
    if (truncated) {
        result.status = DisplayTextEvaluationStatus::Truncated;
        return false;
    }
    result.status = resolved ? DisplayTextEvaluationStatus::Resolved : DisplayTextEvaluationStatus::MissingMetric;
    return resolved;
}

} // namespace ewfm
