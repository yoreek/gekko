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
    MetricValueType valueType{MetricValueType::Null};
    union {
        bool boolValue;
        int32_t intValue;
        float floatValue;
    } number{};
    char text[kMetricValueTextCapacity]{};
};

inline bool metricValueHasValue(const MetricValue& value) {
    return value.valueType != MetricValueType::Null;
}

class MetricValueResolver {
public:
    MetricValueResolver(const DeviceRegistry* registry, const IWifiDriver& wifiDriver, uint32_t now = 0)
        : registry_(registry), wifiDriver_(wifiDriver), now_(now) {}

    bool resolve(MetricNamespace ns, DeviceId sourceId, int32_t metricId, MetricValue& value) const;
    const DeviceRegistry* registry() const {
        return registry_;
    }

private:
    bool resolveDeviceMetric(DeviceId sourceId, int32_t metricId, MetricValue& value) const;
    bool resolveSystemMetric(int32_t metricId, MetricValue& value) const;

    const DeviceRegistry* registry_{nullptr};
    const IWifiDriver& wifiDriver_;
    uint32_t now_{0};
};

} // namespace ewfm
