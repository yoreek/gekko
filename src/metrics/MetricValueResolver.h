#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"
#include "metrics/MetricTypes.h"
#include "time/DateTime.h"
#include "wifi/WifiDriver.h"

#include <cstddef>
#include <cstdint>
#include <variant>

namespace ewfm {

constexpr size_t kMetricValueTextCapacity = 48;

// bool/Int/Float back MetricValueType::Bool/Int/Float; DateTime backs MetricValueType::DateTime
// (a real calendar timestamp, tzInfo included - see DateTime.h); Duration values reuse the
// int32_t alternative (an elapsed millisecond count has no calendar semantics of its own).
using MetricNumber = std::variant<bool, int32_t, float, DateTime>;

struct MetricValue {
    MetricValueType valueType{MetricValueType::Null};
    MetricNumber number{};
    char text[kMetricValueTextCapacity]{};
};

inline bool metricValueHasValue(const MetricValue& value) {
    return value.valueType != MetricValueType::Null;
}

class MetricValueResolver {
public:
    MetricValueResolver(const DeviceRegistry* registry, const IWifiDriver& wifiDriver, uint32_t now = 0, DateTime localTime = DateTime())
        : registry_(registry), wifiDriver_(wifiDriver), now_(now), localTime_(localTime) {}

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
    DateTime localTime_;
};

} // namespace ewfm
