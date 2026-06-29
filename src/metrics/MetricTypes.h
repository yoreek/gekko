#pragma once

#include "devices/core/DeviceTypes.h"

#include <cstdint>

namespace ewfm {

enum class MetricNamespace : uint8_t {
    Device = 0,
    System = 1,
    Wifi = 2,
};

enum class MetricValueType : uint8_t {
    Null = 0,
    Bool = 1,
    Int = 2,
    Float = 3,
    String = 4,
};

constexpr int32_t kDeviceMetricTemperature = 100;
constexpr int32_t kDeviceMetricSwitchState = 200;
constexpr int32_t kSystemMetricTime = 1;
constexpr int32_t kSystemMetricUptime = 2;
constexpr int32_t kSystemMetricWifiStationIp = 3;
constexpr int32_t kSystemMetricWifiSetupApIp = 4;

const char* metricNamespaceName(MetricNamespace ns);
bool parseMetricNamespace(const char* value, MetricNamespace& ns);
const char* metricValueTypeName(MetricValueType type);

} // namespace ewfm
