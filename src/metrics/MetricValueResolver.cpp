#include "metrics/MetricValueResolver.h"

#include "devices/sensors/temperature/TemperatureSensorTypes.h"
#include "devices/switch/OutputState.h"

#include <cstdio>
#include <cstring>

namespace ewfm {

namespace {

void setText(MetricValue& value, const char* text) {
    std::snprintf(value.text, sizeof(value.text), "%s", text != nullptr ? text : "");
}

} // namespace

void MetricValueResolver::formatDuration(const uint32_t durationMs, char* output, const size_t capacity) {
    if (output == nullptr || capacity == 0U) {
        return;
    }

    const uint32_t totalSeconds = durationMs / 1000U;
    const uint32_t hours = totalSeconds / 3600U;
    const uint32_t minutes = (totalSeconds / 60U) % 60U;
    const uint32_t seconds = totalSeconds % 60U;
    if (hours > 0U) {
        std::snprintf(output, capacity, "%lu:%02lu:%02lu", static_cast<unsigned long>(hours), static_cast<unsigned long>(minutes),
                      static_cast<unsigned long>(seconds));
        return;
    }

    std::snprintf(output, capacity, "%lu:%02lu", static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
}

const char* MetricValueResolver::deviceStatusName(const DeviceStatus status) {
    switch (status) {
    case DeviceStatus::Creating:
        return "creating";
    case DeviceStatus::Starting:
        return "starting";
    case DeviceStatus::Ready:
        return "ready";
    case DeviceStatus::Disabled:
        return "disabled";
    case DeviceStatus::Faulted:
        return "faulted";
    case DeviceStatus::DependencyBlocked:
        return "dependency_blocked";
    case DeviceStatus::Reconfiguring:
        return "reconfiguring";
    case DeviceStatus::Stopping:
        return "stopping";
    case DeviceStatus::Deleting:
        return "deleting";
    case DeviceStatus::Unknown:
    default:
        return "unknown";
    }
}

const char* MetricValueResolver::wifiStatusName(const WifiDriverStatus status) {
    switch (status) {
    case WifiDriverStatus::Connected:
        return "connected";
    case WifiDriverStatus::Connecting:
        return "connecting";
    case WifiDriverStatus::Failed:
        return "failed";
    case WifiDriverStatus::Disconnected:
        return "disconnected";
    case WifiDriverStatus::Idle:
    default:
        return "idle";
    }
}

bool MetricValueResolver::resolve(const MetricNamespace ns, const DeviceId sourceId, const int32_t metricId, MetricValue& value) const {
    value = {};
    switch (ns) {
    case MetricNamespace::Device:
        return resolveDeviceMetric(sourceId, metricId, value);
    case MetricNamespace::System:
        return resolveSystemMetric(metricId, value);
    case MetricNamespace::Wifi:
        return resolveWifiMetric(metricId, value);
    }
    return false;
}

bool MetricValueResolver::resolveDeviceMetric(const DeviceId sourceId, const int32_t metricId, MetricValue& value) const {
    if (registry_ == nullptr || sourceId == 0U) {
        return false;
    }
    const IDeviceRuntime* runtime = registry_->runtime(sourceId);
    if (runtime == nullptr) {
        return false;
    }

    switch (metricId) {
    case kDeviceMetricStatus:
        value.known = true;
        value.available = true;
        value.valueType = MetricValueType::Status;
        setText(value, deviceStatusName(runtime->status()));
        return true;
    case kDeviceMetricEffectiveStatus:
        value.known = true;
        value.available = true;
        value.valueType = MetricValueType::Status;
        setText(value, deviceStatusName(registry_->effectiveStatus(sourceId)));
        return true;
    case kDeviceMetricTemperature: {
        value.known = true;
        value.valueType = MetricValueType::Temperature;
        const ITemperatureReadingRuntime* temperature = runtime->temperatureReadingRuntime();
        if (temperature == nullptr) {
            value.available = false;
            return false;
        }
        TemperatureReading reading{};
        value.available = temperature->latestTemperatureReading(reading) && reading.valid;
        if (value.available) {
            std::snprintf(value.text, sizeof(value.text), "%.2f C", static_cast<double>(reading.milliCelsius) / 1000.0);
        }
        return true;
    }
    case kDeviceMetricSwitchState: {
        value.known = true;
        value.valueType = MetricValueType::SwitchState;
        const ISwitchOutputRuntime* switchOutput = runtime->switchOutputRuntime();
        if (switchOutput == nullptr) {
            value.available = false;
            return false;
        }
        value.available = true;
        setText(value, outputStateName(switchOutput->currentOutputState()));
        return true;
    }
    default:
        return false;
    }
}

bool MetricValueResolver::resolveSystemMetric(const int32_t metricId, MetricValue& value) const {
    switch (metricId) {
    case kSystemMetricTime:
        value.known = true;
        value.available = true;
        value.valueType = MetricValueType::Time;
        formatDuration(now_, value.text, sizeof(value.text));
        return true;
    case kSystemMetricUptime:
        value.known = true;
        value.available = true;
        value.valueType = MetricValueType::Text;
        std::snprintf(value.text, sizeof(value.text), "%lu ms", static_cast<unsigned long>(now_));
        return true;
    default:
        return false;
    }
}

bool MetricValueResolver::resolveWifiMetric(const int32_t metricId, MetricValue& value) const {
    switch (metricId) {
    case kWifiMetricStatus:
        value.known = true;
        value.available = true;
        value.valueType = MetricValueType::Status;
        setText(value, wifiStatusName(wifiDriver_.status()));
        return true;
    case kWifiMetricStationIp: {
        value.known = true;
        value.valueType = MetricValueType::Text;
        const std::string ip = wifiDriver_.stationIp();
        value.available = !ip.empty();
        setText(value, ip.c_str());
        return true;
    }
    case kWifiMetricSetupApIp: {
        value.known = true;
        value.valueType = MetricValueType::Text;
        const std::string ip = wifiDriver_.setupApIp();
        value.available = !ip.empty();
        setText(value, ip.c_str());
        return true;
    }
    default:
        return false;
    }
}

} // namespace ewfm
