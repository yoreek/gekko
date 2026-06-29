#include "metrics/MetricTypes.h"

#include <cstring>

namespace ewfm {

const char* metricNamespaceName(const MetricNamespace ns) {
    switch (ns) {
    case MetricNamespace::Device:
        return "dev";
    case MetricNamespace::System:
        return "system";
    case MetricNamespace::Wifi:
        return "wifi";
    }
    return "dev";
}

bool parseMetricNamespace(const char* value, MetricNamespace& ns) {
    if (value == nullptr || std::strcmp(value, "dev") == 0 || std::strcmp(value, "device") == 0) {
        ns = MetricNamespace::Device;
        return true;
    }
    if (std::strcmp(value, "system") == 0) {
        ns = MetricNamespace::System;
        return true;
    }
    if (std::strcmp(value, "wifi") == 0) {
        ns = MetricNamespace::Wifi;
        return true;
    }
    return false;
}

const char* metricValueTypeName(const MetricValueType type) {
    switch (type) {
    case MetricValueType::Text:
        return "text";
    case MetricValueType::Status:
        return "status";
    case MetricValueType::Temperature:
        return "temperature";
    case MetricValueType::SwitchState:
        return "switch_state";
    case MetricValueType::Time:
        return "time";
    }
    return "text";
}

} // namespace ewfm
