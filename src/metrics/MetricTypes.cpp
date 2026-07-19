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
    case MetricValueType::Null:
        return "null";
    case MetricValueType::Bool:
        return "bool";
    case MetricValueType::Int:
        return "int";
    case MetricValueType::Float:
        return "float";
    case MetricValueType::String:
        return "string";
    case MetricValueType::DateTime:
        return "datetime";
    case MetricValueType::Duration:
        return "duration";
    }
    return "null";
}

} // namespace ewfm
