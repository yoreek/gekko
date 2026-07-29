#include "metrics/MetricValueResolver.h"

#include "devices/sensors/temperature/TemperatureSensorTypes.h"
#include "devices/switch/SwitchOutputState.h"
#include "metrics/MetricValueFormat.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace ewfm {

namespace {

void setNull(MetricValue& value);
void setText(MetricValue& value, const char* text);
void setFloat(MetricValue& value, float floatValue, const char* text);
void setDuration(MetricValue& value, uint32_t durationMs, const char* text);
void setDateTime(MetricValue& value, const DateTime& dateTime, const char* text);

class SystemMetricSource final {
public:
    SystemMetricSource(const IWifiDriver& wifiDriver, const uint32_t now, const DateTime& localTime)
        : wifiDriver_(wifiDriver), now_(now), localTime_(localTime) {}

    bool resolve(const int32_t metricId, MetricValue& value) const {
        switch (metricId) {
        case kSystemMetricTime: {
            if (localTime_.year() < kMetricTimeMinValidYear) {
                return false;
            }
            char text[sizeof(value.text)]{};
            formatDateTimePattern(localTime_, "HH:mm:ss", text, sizeof(text));
            setDateTime(value, localTime_, text);
            return true;
        }
        case kSystemMetricUptime: {
            char text[sizeof(value.text)]{};
            formatDurationDefault(now_, text, sizeof(text));
            setDuration(value, now_, text);
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
    DateTime localTime_;
};

void setNull(MetricValue& value) {
    value.valueType = MetricValueType::Null;
    value.number = MetricNumber{};
    value.text[0] = '\0';
}

void setText(MetricValue& value, const char* text) {
    value.valueType = MetricValueType::String;
    value.number = MetricNumber{};
    std::snprintf(value.text, sizeof(value.text), "%s", text != nullptr ? text : "");
}

void setFloat(MetricValue& value, const float floatValue, const char* text) {
    value.valueType = MetricValueType::Float;
    value.number = floatValue;
    std::snprintf(value.text, sizeof(value.text), "%s", text != nullptr ? text : "");
}

void setDuration(MetricValue& value, const uint32_t durationMs, const char* text) {
    value.valueType = MetricValueType::Duration;
    value.number = static_cast<int32_t>(durationMs);
    std::snprintf(value.text, sizeof(value.text), "%s", text != nullptr ? text : "");
}

void setDateTime(MetricValue& value, const DateTime& dateTime, const char* text) {
    value.valueType = MetricValueType::DateTime;
    value.number = dateTime;
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
    case kDeviceMetricHumidity: {
        const IHumidityReadingRuntime* humidity = runtime->humidityReadingRuntime();
        if (humidity == nullptr) {
            return false;
        }
        HumidityReading reading{};
        if (!humidity->latestHumidityReading(reading) || !reading.valid) {
            return false;
        }
        const float percent = static_cast<float>(reading.milliPercent) / 1000.0f;
        char text[sizeof(value.text)]{};
        std::snprintf(text, sizeof(text), "%.1f %%", static_cast<double>(percent));
        setFloat(value, percent, text);
        return true;
    }
    case kDeviceMetricSwitchState: {
        const ISwitchOutputRuntime* switchOutput = runtime->switchOutputRuntime();
        if (switchOutput == nullptr) {
            return false;
        }
        setText(value, switchOutputStateName(switchOutput->currentOutputState()));
        return true;
    }
    default:
        return false;
    }
}

bool MetricValueResolver::resolveSystemMetric(const int32_t metricId, MetricValue& value) const {
    return SystemMetricSource(wifiDriver_, now_, localTime_).resolve(metricId, value);
}

} // namespace ewfm
