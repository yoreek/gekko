#include "portal/controllers/MetricsController.h"

#include "metrics/MetricValueFormat.h"
#include "metrics/MetricValueResolver.h"
#include "time/DateTime.h"

#include <cstdio>
#include <memory>
#include <vector>

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

// Print sink that appends into an Arduino String, so the existing Print-based metric writers can
// build one chunked piece (one device's metrics, or the global block) into a buffer without change.
class StringPrint : public Print {
public:
    explicit StringPrint(String& out) : out_(out) {}
    size_t write(uint8_t c) override {
        out_ += static_cast<char>(c);
        return 1;
    }
    size_t write(const uint8_t* buffer, size_t size) override {
        out_.concat(reinterpret_cast<const char*>(buffer), size);
        return size;
    }

private:
    String& out_;
};

void writeJsonString(String& output, const char* value) {
    StaticJsonDocument<64> doc;
    doc.set(value != nullptr ? value : "");
    serializeJson(doc, output);
}

void writeDescriptor(Print& response, bool& first, const MetricNamespace ns, const DeviceId sourceId, const char* sourceLabel,
                     const int32_t metricId, const char* metricKey, const char* label, const MetricValueType valueType,
                     const bool available, const char* preview, const bool hasPreviewNumber, const double previewNumber) {
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
    if (hasPreviewNumber) {
        response.print(",\"previewNumber\":");
        response.print(previewNumber, 2);
    }
    response.print('}');
}

void writeMetricValueJson(Print& response, bool& first, const MetricNamespace ns, const DeviceId sourceId, const char* sourceLabel,
                          const int32_t metricId, const char* metricKey, const MetricValue& value) {
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
    response.print(metricValueHasValue(value) ? "true" : "false");
    response.print(",\"value\":");
    String valueJson;
    writeJsonString(valueJson, value.text);
    response.print(valueJson);
    response.print('}');
}

void writeDeviceDescriptor(Print& response, bool& first, const IDeviceRuntime& runtime, const int32_t metricId, const char* metricKey,
                           const char* metricLabel, const MetricValueType valueType, const bool available, const char* preview,
                           const bool hasPreviewNumber, const double previewNumber) {
    char label[96]{};
    std::snprintf(label, sizeof(label), "%s %s", runtime.name() != nullptr ? runtime.name() : "Device", metricLabel);
    writeDescriptor(response, first, MetricNamespace::Device, runtime.deviceId(), runtime.name(), metricId, metricKey, label, valueType,
                    available, preview, hasPreviewNumber, previewNumber);
}

void writeDeviceMetricDescriptor(Print& response, bool& first, const MetricValueResolver& resolver, const IDeviceRuntime& runtime,
                                 const int32_t metricId, const char* metricKey, const char* metricLabel, const MetricValueType valueType) {
    MetricValue value{};
    (void)resolver.resolve(MetricNamespace::Device, runtime.deviceId(), metricId, value);
    writeDeviceDescriptor(response, first, runtime, metricId, metricKey, metricLabel, valueType, metricValueHasValue(value), value.text,
                          metricValueHasNumericPreview(value), metricValueNumericPreview(value));
}

void writeDeviceMetrics(Print& response, bool& first, const MetricValueResolver& resolver, const IDeviceRuntime& runtime) {
    if (runtime.temperatureReadingRuntime() != nullptr) {
        writeDeviceMetricDescriptor(response, first, resolver, runtime, kDeviceMetricTemperature, "temperature", "temperature",
                                    MetricValueType::Float);
    }

    if (runtime.humidityReadingRuntime() != nullptr) {
        writeDeviceMetricDescriptor(response, first, resolver, runtime, kDeviceMetricHumidity, "humidity", "humidity",
                                    MetricValueType::Float);
    }

    if (runtime.switchOutputRuntime() != nullptr) {
        writeDeviceMetricDescriptor(response, first, resolver, runtime, kDeviceMetricSwitchState, "state", "state",
                                    MetricValueType::String);
    }
}

void writeDeviceMetricValue(Print& response, bool& first, const MetricValueResolver& resolver, const IDeviceRuntime& runtime,
                            const int32_t metricId, const char* metricKey) {
    MetricValue value{};
    if (resolver.resolve(MetricNamespace::Device, runtime.deviceId(), metricId, value)) {
        writeMetricValueJson(response, first, MetricNamespace::Device, runtime.deviceId(), runtime.name(), metricId, metricKey, value);
    }
}

void writeDeviceMetricValues(Print& response, bool& first, const MetricValueResolver& resolver, const IDeviceRuntime& runtime) {
    if (runtime.temperatureReadingRuntime() != nullptr) {
        writeDeviceMetricValue(response, first, resolver, runtime, kDeviceMetricTemperature, "temperature");
    }
    if (runtime.humidityReadingRuntime() != nullptr) {
        writeDeviceMetricValue(response, first, resolver, runtime, kDeviceMetricHumidity, "humidity");
    }
    if (runtime.switchOutputRuntime() != nullptr) {
        writeDeviceMetricValue(response, first, resolver, runtime, kDeviceMetricSwitchState, "state");
    }
}

void writeGlobalMetricDescriptor(Print& response, bool& first, const MetricValueResolver& resolver, const MetricNamespace ns,
                                 const int32_t metricId, const char* metricKey, const char* label, const MetricValueType fallbackType) {
    MetricValue value{};
    const bool resolved = resolver.resolve(ns, 0, metricId, value);
    if (!resolved) {
        value.valueType = fallbackType;
    }
    // Only a genuinely resolved value has a real typed number behind it - a fallback-typed but
    // unresolved MetricValue still carries the Null variant content, not an instance of
    // fallbackType, so a numeric preview would be meaningless.
    const bool hasPreviewNumber = resolved && metricValueHasNumericPreview(value);
    const double previewNumber = hasPreviewNumber ? metricValueNumericPreview(value) : 0.0;
    writeDescriptor(response, first, ns, 0, nullptr, metricId, metricKey, label, value.valueType, metricValueHasValue(value), value.text,
                    hasPreviewNumber, previewNumber);
}

void writeGlobalMetricValue(Print& response, bool& first, const MetricValueResolver& resolver, const MetricNamespace ns,
                            const int32_t metricId, const char* metricKey) {
    MetricValue value{};
    if (resolver.resolve(ns, 0, metricId, value)) {
        writeMetricValueJson(response, first, ns, 0, nullptr, metricId, metricKey, value);
    }
}

void writeGlobalDescriptors(Print& response, bool& first, const MetricValueResolver& resolver) {
    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::System, kSystemMetricTime, "time", "System time",
                                MetricValueType::DateTime);
    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::System, kSystemMetricUptime, "uptime", "System uptime",
                                MetricValueType::Duration);
    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::System, kSystemMetricWifiStationIp, "wifi.station_ip",
                                "WiFi station IP", MetricValueType::String);
    writeGlobalMetricDescriptor(response, first, resolver, MetricNamespace::System, kSystemMetricWifiSetupApIp, "wifi.setup_ap_ip",
                                "WiFi AP IP", MetricValueType::String);
}

void writeGlobalValues(Print& response, bool& first, const MetricValueResolver& resolver) {
    writeGlobalMetricValue(response, first, resolver, MetricNamespace::System, kSystemMetricTime, "time");
    writeGlobalMetricValue(response, first, resolver, MetricNamespace::System, kSystemMetricUptime, "uptime");
    writeGlobalMetricValue(response, first, resolver, MetricNamespace::System, kSystemMetricWifiStationIp, "wifi.station_ip");
    writeGlobalMetricValue(response, first, resolver, MetricNamespace::System, kSystemMetricWifiSetupApIp, "wifi.setup_ap_ip");
}

// Producer for /api/metrics/{placeholders,values}: streams one device's metrics (or the global
// block) per piece into a reused GrowableBuffer, keeping peak allocation bounded to one small piece
// regardless of device count. `values` selects between the value and descriptor payloads; the two
// endpoints differ only in that flag and the top-level array key.
struct MetricsProducerState {
    const DeviceRegistry* registry = nullptr;
    std::unique_ptr<MetricValueResolver> resolver;
    std::vector<DeviceId> deviceIds; // snapshot taken up front
    size_t cursor = 0;
    uint32_t registryRevision = 0;
    bool values = false;
    bool anyEmitted = false;
    int phase = 0; // 0 = prefix, 1 = devices, 2 = globals, 3 = suffix, 4 = done
};

// Serialize one Print-built piece into `piece`, prefixing an inter-element comma when earlier
// pieces already emitted (writers add commas between units within the piece themselves via `first`).
void buildDevicePiece(MetricsProducerState& s, const IDeviceRuntime& runtime, String& piece) {
    StringPrint sink(piece);
    bool first = !s.anyEmitted;
    if (s.values) {
        writeDeviceMetricValues(sink, first, *s.resolver, runtime);
    } else {
        writeDeviceMetrics(sink, first, *s.resolver, runtime);
    }
}

bool produceMetricsPiece(MetricsProducerState& s, BaseController::ChunkedBody& body) {
    if (s.phase == 0) {
        char header[80];
        const int n = std::snprintf(header, sizeof(header), "{\"success\":true,\"registryRevision\":%lu,\"%s\":[",
                                    static_cast<unsigned long>(s.registryRevision), s.values ? "values" : "placeholders");
        s.phase = 1;
        return n > 0 && body.emit(header, static_cast<size_t>(n));
    }

    if (s.phase == 1) {
        while (s.cursor < s.deviceIds.size()) {
            const IDeviceRuntime* runtime = s.registry != nullptr ? s.registry->runtime(s.deviceIds[s.cursor++]) : nullptr;
            if (runtime == nullptr) {
                continue; // deleted between snapshot and now, or no registry
            }
            String piece;
            buildDevicePiece(s, *runtime, piece);
            if (piece.length() == 0U) {
                continue; // device exposes no metrics
            }
            s.anyEmitted = true;
            return body.emit(piece.c_str(), piece.length());
        }
        s.phase = 2; // fall through to globals
    }

    if (s.phase == 2) {
        s.phase = 3;
        String piece;
        StringPrint sink(piece);
        bool first = !s.anyEmitted;
        if (s.values) {
            writeGlobalValues(sink, first, *s.resolver);
        } else {
            writeGlobalDescriptors(sink, first, *s.resolver);
        }
        if (piece.length() != 0U) {
            s.anyEmitted = true;
            return body.emit(piece.c_str(), piece.length());
        }
        // else fall through to suffix
    }

    if (s.phase == 3) {
        s.phase = 4;
        return body.emit("]}", 2);
    }

    return false;
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

#if defined(ARDUINO) && !defined(UNIT_TEST)
void MetricsController::streamMetrics(bool values) {
    // registry_ and wifiDriver_ outlive the request (owned by App), so the resolver held in the
    // shared state stays valid on the async_tcp task after this controller is destroyed.
    auto state = std::make_shared<MetricsProducerState>();
    state->registry = registry_;
    state->resolver = std::make_unique<MetricValueResolver>(registry_, wifiDriver_, ::millis(), DateTime::current());
    state->registryRevision = registry_ != nullptr ? registry_->registryRevision() : 0U;
    state->values = values;
    if (registry_ != nullptr) {
        registry_->forEachRuntime([&state](const IDeviceRuntime& runtime) { state->deviceIds.push_back(runtime.deviceId()); });
    }
    sendChunked("application/json", [state](ChunkedBody& body) { return produceMetricsPiece(*state, body); });
}
#endif

void MetricsController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    streamMetrics(false);
#endif
}

void MetricsController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    streamMetrics(true);
#endif
}

void MetricsController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

} // namespace ewfm
