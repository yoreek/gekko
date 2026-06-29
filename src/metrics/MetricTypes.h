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
    Text = 0,
    Status = 1,
    Temperature = 2,
    SwitchState = 3,
    Time = 4,
};

constexpr int32_t kDeviceMetricStatus = 1;
constexpr int32_t kDeviceMetricEffectiveStatus = 2;
constexpr int32_t kDeviceMetricTemperature = 100;
constexpr int32_t kDeviceMetricSwitchState = 200;
constexpr int32_t kSystemMetricTime = 1;
constexpr int32_t kSystemMetricUptime = 2;
constexpr int32_t kWifiMetricStatus = 1;
constexpr int32_t kWifiMetricStationIp = 2;
constexpr int32_t kWifiMetricSetupApIp = 3;

const char* metricNamespaceName(MetricNamespace ns);
bool parseMetricNamespace(const char* value, MetricNamespace& ns);
const char* metricValueTypeName(MetricValueType type);

} // namespace ewfm
