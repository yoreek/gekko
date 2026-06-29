#include "metrics/MetricValueResolver.h"

#include "devices/sensors/temperature/TemperatureSensorTypes.h"
#include "devices/switch/OutputState.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace ewfm {

namespace {

void formatDuration(const uint32_t durationMs, char* output, const size_t capacity);
void setNull(MetricValue& value);
void setText(MetricValue& value, const char* text);
void setInt(MetricValue& value, const int32_t intValue, const char* text);
void setFloat(MetricValue& value, const float floatValue, const char* text);

class SystemMetricSource final {
public:
    SystemMetricSource(const IWifiDriver& wifiDriver, const uint32_t now) : wifiDriver_(wifiDriver), now_(now) {}

    bool resolve(const int32_t metricId, MetricValue& value) const {
        switch (metricId) {
        case kSystemMetricTime: {
            char text[sizeof(value.text)]{};
            formatDuration(now_, text, sizeof(text));
            setText(value, text);
            return true;
        }
        case kSystemMetricUptime: {
            char text[sizeof(value.text)]{};
            std::snprintf(text, sizeof(text), "%lu ms", static_cast<unsigned long>(now_));
            setInt(value, static_cast<int32_t>(now_), text);
            return true;
        }
        case kSystemMetricWifiStationIp: {
            const std::string ip = wifiDriver_.stationIp();
            if (ip.empty()) {
                return false;
            }
            setText(value, ip.c_str());
            return true;
        }
        case kSystemMetricWifiSetupApIp: {
            const std::string ip = wifiDriver_.setupApIp();
            if (ip.empty()) {
                return false;
            }
            setText(value, ip.c_str());
            return true;
        }
        default:
            return false;
        }
    }

private:
    const IWifiDriver& wifiDriver_;
    uint32_t now_;
};

void formatDuration(const uint32_t durationMs, char* output, const size_t capacity) {
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

void setNull(MetricValue& value) {
    value.valueType = MetricValueType::Null;
    std::memset(&value.number, 0, sizeof(value.number));
    value.text[0] = '\0';
}

void setText(MetricValue& value, const char* text) {
    value.valueType = MetricValueType::String;
    std::memset(&value.number, 0, sizeof(value.number));
    std::snprintf(value.text, sizeof(value.text), "%s", text != nullptr ? text : "");
}

void setInt(MetricValue& value, const int32_t intValue, const char* text) {
    value.valueType = MetricValueType::Int;
    value.number.intValue = intValue;
    std::snprintf(value.text, sizeof(value.text), "%s", text != nullptr ? text : "");
}

void setFloat(MetricValue& value, const float floatValue, const char* text) {
    value.valueType = MetricValueType::Float;
    value.number.floatValue = floatValue;
    std::snprintf(value.text, sizeof(value.text), "%s", text != nullptr ? text : "");
}

} // namespace

bool MetricValueResolver::resolve(const MetricNamespace ns, const DeviceId sourceId, const int32_t metricId, MetricValue& value) const {
    setNull(value);
    switch (ns) {
    case MetricNamespace::Device:
        if (sourceId == 0U) {
            return false;
        }
        return resolveDeviceMetric(sourceId, metricId, value);
    case MetricNamespace::System:
        return resolveSystemMetric(metricId, value);
    case MetricNamespace::Wifi:
        return false;
    }
    return false;
}

bool MetricValueResolver::resolveDeviceMetric(const DeviceId sourceId, const int32_t metricId, MetricValue& value) const {
    if (registry_ == nullptr) {
        return false;
    }
    const IDeviceRuntime* runtime = registry_->runtime(sourceId);
    if (runtime == nullptr) {
        return false;
    }

    switch (metricId) {
    case kDeviceMetricTemperature: {
        const ITemperatureReadingRuntime* temperature = runtime->temperatureReadingRuntime();
        if (temperature == nullptr) {
            return false;
        }
        TemperatureReading reading{};
        if (!temperature->latestTemperatureReading(reading) || !reading.valid) {
            return false;
        }
        const float celsius = static_cast<float>(reading.milliCelsius) / 1000.0f;
        char text[sizeof(value.text)]{};
        std::snprintf(text, sizeof(text), "%.2f C", static_cast<double>(celsius));
        setFloat(value, celsius, text);
        return true;
    }
    case kDeviceMetricSwitchState: {
        const ISwitchOutputRuntime* switchOutput = runtime->switchOutputRuntime();
        if (switchOutput == nullptr) {
            return false;
        }
        setText(value, outputStateName(switchOutput->currentOutputState()));
        return true;
    }
    default:
        return false;
    }
}

bool MetricValueResolver::resolveSystemMetric(const int32_t metricId, MetricValue& value) const {
    return SystemMetricSource(wifiDriver_, now_).resolve(metricId, value);
}

} // namespace ewfm
