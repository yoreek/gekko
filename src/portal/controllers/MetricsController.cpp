#include "portal/controllers/MetricsController.h"

#include "metrics/MetricValueResolver.h"

#include <cstdio>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

MetricsController::MetricsController(AsyncWebServerRequest* request, const Action action, DeviceRegistry* registry, IWifiDriver& wifiDriver)
    : BaseController(request, action), registry_(registry), wifiDriver_(wifiDriver) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {

void writeJsonString(String& output, const char* value) {
    StaticJsonDocument<64> doc;
    doc.set(value != nullptr ? value : "");
    serializeJson(doc, output);
}

void writeDescriptor(AsyncResponseStream& response, bool& first, const MetricNamespace ns, const DeviceId sourceId,
                     const char* sourceLabel, const int32_t metricId, const char* metricKey, const char* label,
                     const MetricValueType valueType, const bool available, const char* preview) {
    if (!first) {
        response.print(',');
    }
    first = false;

    response.print('{');
    response.print("\"placeholder\":\"{{");
    response.print(metricNamespaceName(ns));
    if (ns == MetricNamespace::Device) {
        response.print('.');
        response.print(static_cast<unsigned long>(sourceId));
    }
    response.print('.');
    response.print(metricKey);
    response.print("}}\",");
    response.print("\"namespace\":\"");
    response.print(metricNamespaceName(ns));
    response.print("\",");
    response.printf("\"sourceId\":%lu,", static_cast<unsigned long>(sourceId));
    if (ns == MetricNamespace::Device && sourceLabel != nullptr && sourceLabel[0] != '\0') {
        response.print("\"sourceLabel\":");
        String sourceLabelJson;
        writeJsonString(sourceLabelJson, sourceLabel);
        response.print(sourceLabelJson);
        response.print(',');
    }
    response.printf("\"metricId\":%ld,", static_cast<long>(metricId));
    response.print("\"metricKey\":\"");
    response.print(metricKey);
    response.print("\",");
    response.print("\"label\":");
    String labelJson;
    writeJsonString(labelJson, label);
    response.print(labelJson);
    response.print(',');
    response.print("\"valueType\":\"");
    response.print(metricValueTypeName(valueType));
    response.print("\",");
    response.print("\"available\":");
    response.print(available ? "true" : "false");
    response.print(",\"preview\":");
    String previewJson;
    writeJsonString(previewJson, preview);
    response.print(previewJson);
    response.print('}');
}

void writeMetricValueJson(AsyncResponseStream& response, bool& first, const MetricNamespace ns, const DeviceId sourceId,
                          const char* sourceLabel, const int32_t metricId, const char* metricKey, const MetricValue& value) {
    if (!first) {
        response.print(',');
    }
    first = false;

    response.print('{');
    response.print("\"namespace\":\"");
    response.print(metricNamespaceName(ns));
    response.print("\",");
    response.printf("\"sourceId\":%lu,", static_cast<unsigned long>(sourceId));
    if (ns == MetricNamespace::Device && sourceLabel != nullptr && sourceLabel[0] != '\0') {
        response.print("\"sourceLabel\":");
        String sourceLabelJson;
        writeJsonString(sourceLabelJson, sourceLabel);
        response.print(sourceLabelJson);
        response.print(',');
    }
    response.printf("\"metricId\":%ld,", static_cast<long>(metricId));
    response.print("\"metricKey\":\"");
    response.print(metricKey);
    response.print("\",");
    response.print("\"valueType\":\"");
    response.print(metricValueTypeName(value.valueType));
    response.print("\",");
    response.print("\"available\":");
    response.print(value.available ? "true" : "false");
    response.print(",\"value\":");
    String valueJson;
    writeJsonString(valueJson, value.text);
    response.print(valueJson);
    response.print('}');
}

void writeDeviceDescriptor(AsyncResponseStream& response, bool& first, const IDeviceRuntime& runtime, const int32_t metricId,
                           const char* metricKey, const char* metricLabel, const MetricValueType valueType, const bool available,
                           const char* preview) {
    char label[96]{};
    std::snprintf(label, sizeof(label), "%s %s", runtime.name() != nullptr ? runtime.name() : "Device", metricLabel);
    writeDescriptor(response, first, MetricNamespace::Device, runtime.deviceId(), runtime.name(), metricId, metricKey, label, valueType,
                    available, preview);
}

void writeDeviceMetricDescriptor(AsyncResponseStream& response, bool& first, const MetricValueResolver& resolver,
                                 const IDeviceRuntime& runtime, const int32_t metricId, const char* metricKey, const char* metricLabel,
                                 const MetricValueType valueType) {
    MetricValue value{};
    (void)resolver.resolve(MetricNamespace::Device, runtime.deviceId(), metricId, value);
    writeDeviceDescriptor(response, first, runtime, metricId, metricKey, metricLabel, valueType, value.available, value.text);
}

void writeDeviceMetrics(AsyncResponseStream& response, bool& first, const MetricValueResolver& resolver, const IDeviceRuntime& runtime) {
    writeDeviceMetricDescriptor(response, first, resolver, runtime, kDeviceMetricStatus, "status", "status", MetricValueType::Status);
    writeDeviceMetricDescriptor(response, first, resolver, runtime, kDeviceMetricEffectiveStatus, "effective_status", "effective status",
                                MetricValueType::Status);

    if (runtime.temperatureReadingRuntime() != nullptr) {
        writeDeviceMetricDescriptor(response, first, resolver, runtime, kDeviceMetricTemperature, "temperature", "temperature",
                                    MetricValueType::Temperature);
    }

    if (runtime.switchOutputRuntime() != nullptr) {
        writeDeviceMetricDescriptor(response, first, resolver, runtime, kDeviceMetricSwitchState, "state", "state",
                                    MetricValueType::SwitchState);
    }
}

void writeDeviceMetricValue(AsyncResponseStream& response, bool& first, const MetricValueResolver& resolver, const IDeviceRuntime& runtime,
                            const int32_t metricId, const char* metricKey) {
    MetricValue value{};
    if (resolver.resolve(MetricNamespace::Device, runtime.deviceId(), metricId, value)) {
        writeMetricValueJson(response, first, MetricNamespace::Device, runtime.deviceId(), runtime.name(), metricId, metricKey, value);
    }
}

void writeDeviceMetricValues(AsyncResponseStream& response, bool& first, const MetricValueResolver& resolver,
                             const IDeviceRuntime& runtime) {
    writeDeviceMetricValue(response, first, resolver, runtime, kDeviceMetricStatus, "status");
    writeDeviceMetricValue(response, first, resolver, runtime, kDeviceMetricEffectiveStatus, "effective_status");
    if (runtime.temperatureReadingRuntime() != nullptr) {
        writeDeviceMetricValue(response, first, resolver, runtime, kDeviceMetricTemperature, "temperature");
    }
    if (runtime.switchOutputRuntime() != nullptr) {
        writeDeviceMetricValue(response, first, resolver, runtime, kDeviceMetricSwitchState, "state");
    }
}

void writeGlobalMetricDescriptor(AsyncResponseStream& response, bool& first, const MetricValueResolver& resolver, const MetricNamespace ns,
                                 const int32_t metricId, const char* metricKey, const char* label, const MetricValueType fallbackType) {
    MetricValue value{};
    if (!resolver.resolve(ns, 0, metricId, value)) {
        value.valueType = fallbackType;
    }
    writeDescriptor(response, first, ns, 0, nullptr, metricId, metricKey, label, value.valueType, value.available, value.text);
}

void writeGlobalMetricValue(AsyncResponseStream& response, bool& first, const MetricValueResolver& resolver, const MetricNamespace ns,
                            const int32_t metricId, const char* metricKey) {
    MetricValue value{};
    if (resolver.resolve(ns, 0, metricId, value)) {
        writeMetricValueJson(response, first, ns, 0, nullptr, metricId, metricKey, value);
    }
}

void writeMetricDescriptors(AsyncResponseStream& response, DeviceRegistry* registry, const MetricValueResolver& resolver) {
    bool first = true;
    if (registry != nullptr) {
        registry->forEachRuntime([&](const IDeviceRuntime& runtime) { writeDeviceMetrics(response, first, resolver, runtime); });
    }

    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::System, kSystemMetricTime, "time", "System time",
                                MetricValueType::Time);
    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::System, kSystemMetricUptime, "uptime", "System uptime",
                                MetricValueType::Text);
    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::Wifi, kWifiMetricStatus, "status", "WiFi status",
                                MetricValueType::Status);
    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::Wifi, kWifiMetricStationIp, "station_ip", "WiFi station IP",
                                MetricValueType::Text);
    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::Wifi, kWifiMetricSetupApIp, "setup_ap_ip", "WiFi AP IP",
                                MetricValueType::Text);
}

void writeMetricValues(AsyncResponseStream& response, DeviceRegistry* registry, const MetricValueResolver& resolver) {
    bool first = true;
    if (registry != nullptr) {
        registry->forEachRuntime([&](const IDeviceRuntime& runtime) { writeDeviceMetricValues(response, first, resolver, runtime); });
    }

    writeGlobalMetricValue(response, first, resolver, MetricNamespace::System, kSystemMetricTime, "time");
    writeGlobalMetricValue(response, first, resolver, MetricNamespace::System, kSystemMetricUptime, "uptime");
    writeGlobalMetricValue(response, first, resolver, MetricNamespace::Wifi, kWifiMetricStatus, "status");
    writeGlobalMetricValue(response, first, resolver, MetricNamespace::Wifi, kWifiMetricStationIp, "station_ip");
    writeGlobalMetricValue(response, first, resolver, MetricNamespace::Wifi, kWifiMetricSetupApIp, "setup_ap_ip");
}

} // namespace

void MetricsController::registerRoutes(AsyncWebServer& server, DeviceRegistry* registry, IWifiDriver& wifiDriver) {
    server.on(AsyncURIMatcher::exact("/api/metrics/placeholders"), HTTP_GET, [&wifiDriver, registry](AsyncWebServerRequest* request) {
        MetricsController(request, Action::Index, registry, wifiDriver).dispatch();
    });
    server.on(AsyncURIMatcher::exact("/api/metrics/placeholders"), HTTP_OPTIONS, [&wifiDriver, registry](AsyncWebServerRequest* request) {
        MetricsController(request, Action::Options, registry, wifiDriver).dispatch();
    });
    server.on(AsyncURIMatcher::exact("/api/metrics/values"), HTTP_GET, [&wifiDriver, registry](AsyncWebServerRequest* request) {
        MetricsController(request, Action::Show, registry, wifiDriver).dispatch();
    });
    server.on(AsyncURIMatcher::exact("/api/metrics/values"), HTTP_OPTIONS, [&wifiDriver, registry](AsyncWebServerRequest* request) {
        MetricsController(request, Action::Options, registry, wifiDriver).dispatch();
    });
}
#endif

void MetricsController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    AsyncResponseStream* response = request_->beginResponseStream("application/json");
    response->print("{\"success\":true,");
    response->printf("\"registryRevision\":%lu,", static_cast<unsigned long>(registry_ != nullptr ? registry_->registryRevision() : 0U));
    response->print("\"placeholders\":[");
    const MetricValueResolver resolver(registry_, wifiDriver_, ::millis());
    writeMetricDescriptors(*response, registry_, resolver);
    response->print("]}");
    send(response);
#endif
}

void MetricsController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    AsyncResponseStream* response = request_->beginResponseStream("application/json");
    response->print("{\"success\":true,");
    response->printf("\"registryRevision\":%lu,", static_cast<unsigned long>(registry_ != nullptr ? registry_->registryRevision() : 0U));
    response->print("\"values\":[");
    const MetricValueResolver resolver(registry_, wifiDriver_, ::millis());
    writeMetricValues(*response, registry_, resolver);
    response->print("]}");
    send(response);
#endif
}

void MetricsController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

} // namespace ewfm
