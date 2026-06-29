#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"
#include "metrics/MetricTypes.h"
#include "wifi/WifiDriver.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr size_t kMetricValueTextCapacity = 48;

struct MetricValue {
    MetricValueType valueType{MetricValueType::Text};
    bool known{false};
    bool available{false};
    char text[kMetricValueTextCapacity]{};
};

class MetricValueResolver {
public:
    MetricValueResolver(const DeviceRegistry* registry, const IWifiDriver& wifiDriver, uint32_t now = 0)
        : registry_(registry), wifiDriver_(wifiDriver), now_(now) {}

    bool resolve(MetricNamespace ns, DeviceId sourceId, int32_t metricId, MetricValue& value) const;

    static const char* deviceStatusName(DeviceStatus status);
    static const char* wifiStatusName(WifiDriverStatus status);

private:
    bool resolveDeviceMetric(DeviceId sourceId, int32_t metricId, MetricValue& value) const;
    bool resolveSystemMetric(int32_t metricId, MetricValue& value) const;
    bool resolveWifiMetric(int32_t metricId, MetricValue& value) const;
    static void formatDuration(uint32_t durationMs, char* output, size_t capacity);

    const DeviceRegistry* registry_{nullptr};
    const IWifiDriver& wifiDriver_;
    uint32_t now_{0};
};

} // namespace ewfm
