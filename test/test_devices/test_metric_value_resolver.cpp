#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/core/DeviceTypes.h"
#include "devices/display/DisplayTextEvaluator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "metrics/MetricTypes.h"
#include "metrics/MetricValueResolver.h"
#include "wifi/WifiDriver.h"

#include <cstring>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeWifiDriver final : public IWifiDriver {
public:
    bool begin() override {
        return true;
    }
    bool beginStation(const WiFiCredentials& credentials) override {
        (void)credentials;
        return true;
    }
    void disconnect() override {}
    void clearStationCredentials() override {}
    bool startSetupAp(const std::string& ssid, const std::string& password) override {
        (void)ssid;
        (void)password;
        return true;
    }
    void stopSetupAp() override {}
    WifiDriverStatus status() const override {
        return WifiDriverStatus::Connected;
    }
    bool networkStackReady() const override {
        return true;
    }
    bool stationReady() const override {
        return true;
    }
    bool setupApReady() const override {
        return false;
    }
    std::string stationIp() const override {
        return "192.168.1.50";
    }
    std::string setupApIp() const override {
        return "";
    }
    bool startScan() override {
        return true;
    }
    bool scanComplete(std::vector<WifiNetwork>& networks, size_t maxResults) override {
        (void)networks;
        (void)maxResults;
        return false;
    }
    std::string macSuffix() const override {
        return "abcd";
    }
};

DeviceId createDummy(DeviceRegistry& registry, const char* name, uint32_t now) {
    DummyDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));

    DeviceCreateRequest request{};
    request.typeId = DummyDevice::descriptor().typeId;
    request.name = name;
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    TEST_ASSERT_TRUE(request.configBlob.assign(buffer, dummyDeviceConfigSize(config)));
    const DeviceCreateResult result = registry.create(request, now);
    TEST_ASSERT_TRUE(result.ok());
    registry.tickFastLoop(now + 1U);
    return result.deviceId;
}

} // namespace

void test_metric_value_resolver_resolves_device_temperature_and_missing_sources() {
    MemoryConfigStorage registryStorage;
    DeviceRegistryStore registryStore(registryStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    SequentialDeviceIdSource idSource(20);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    const DeviceId dummyId = createDummy(registry, "dummy", 10);
    FakeWifiDriver wifi;
    MetricValueResolver resolver(&registry, wifi);

    MetricValue value{};
    TEST_ASSERT_FALSE(resolver.resolve(MetricNamespace::Device, dummyId, kDeviceMetricTemperature, value));
    TEST_ASSERT_FALSE(resolver.resolve(MetricNamespace::Device, 999, kDeviceMetricTemperature, value));
}

void test_metric_value_resolver_resolves_system_values() {
    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi, 3723000U);

    MetricValue value{};
    TEST_ASSERT_TRUE(resolver.resolve(MetricNamespace::System, 0, kSystemMetricWifiStationIp, value));
    TEST_ASSERT_EQUAL(MetricValueType::String, value.valueType);
    TEST_ASSERT_EQUAL_STRING("192.168.1.50", value.text);

    TEST_ASSERT_FALSE(resolver.resolve(MetricNamespace::System, 0, kSystemMetricWifiSetupApIp, value));
    TEST_ASSERT_EQUAL(MetricValueType::Null, value.valueType);
    TEST_ASSERT_EQUAL_STRING("", value.text);

    TEST_ASSERT_TRUE(resolver.resolve(MetricNamespace::System, 0, kSystemMetricTime, value));
    TEST_ASSERT_EQUAL(MetricValueType::String, value.valueType);
    TEST_ASSERT_EQUAL_STRING("1:02:03", value.text);

    TEST_ASSERT_TRUE(resolver.resolve(MetricNamespace::System, 0, kSystemMetricUptime, value));
    TEST_ASSERT_EQUAL(MetricValueType::Int, value.valueType);
    TEST_ASSERT_EQUAL_INT32(3723000, value.number.intValue);
    TEST_ASSERT_EQUAL_STRING("3723000 ms", value.text);
}

void test_display_text_evaluator_resolves_static_and_placeholder_text() {
    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi);

    DisplayLayoutWidgetV1 widget{};
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Text);
    std::snprintf(widget.text, sizeof(widget.text), "%s", "IP {{system.wifi.station_ip}}");

    DisplayTextEvaluationResult result{};
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::Resolved, result.status);
    TEST_ASSERT_EQUAL_STRING("IP 192.168.1.50", result.text);

    MemoryConfigStorage registryStorage;
    DeviceRegistryStore registryStore(registryStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    SequentialDeviceIdSource idSource(55);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    MetricValueResolver timeResolver(nullptr, wifi, 3723000U);
    std::snprintf(widget.text, sizeof(widget.text), "%s", "Time {{system.time}}");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, timeResolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::Resolved, result.status);
    TEST_ASSERT_EQUAL_STRING("Time 1:02:03", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "Static label");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_FALSE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::Static, result.status);
    TEST_ASSERT_EQUAL_STRING("Static label", result.text);
}

void test_display_text_evaluator_handles_missing_and_binding_only_metrics() {
    MemoryConfigStorage registryStorage;
    DeviceRegistryStore registryStore(registryStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    SequentialDeviceIdSource idSource(30);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    FakeWifiDriver wifi;
    MetricValueResolver resolver(&registry, wifi);

    DisplayLayoutWidgetV1 widget{};
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Text);
    std::snprintf(widget.text, sizeof(widget.text), "%s", "Sensor {{dev.999.temperature}}");

    DisplayTextEvaluationResult result{};
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::MissingMetric, result.status);
    TEST_ASSERT_EQUAL_STRING("Sensor ", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "AP {{system.wifi.setup_ap_ip}}");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::MissingMetric, result.status);
    TEST_ASSERT_EQUAL_STRING("AP ", result.text);

    MetricValueResolver timeResolver(&registry, wifi, 3723000U);
    widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::Metric);
    widget.metricNamespace = static_cast<uint8_t>(MetricNamespace::System);
    widget.sourceDeviceId = 0;
    widget.metricId = kSystemMetricTime;
    widget.text[0] = '\0';
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, timeResolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL_STRING("1:02:03", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "Room {{system.wifi.station_ip | upper}}");
    const DeviceValidationResult validation = validateDisplayTextWidget(widget, registry);
    TEST_ASSERT_TRUE(validation.ok());

    std::snprintf(widget.text, sizeof(widget.text), "%s", "Room {{system.wifi.station_ip |   }}");
    const DeviceValidationResult validationWithEmptyFilter = validateDisplayTextWidget(widget, registry);
    TEST_ASSERT_TRUE(validationWithEmptyFilter.ok());
}

void test_display_text_evaluator_reports_invalid_placeholders() {
    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi);

    DisplayLayoutWidgetV1 widget{};
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Text);
    std::snprintf(widget.text, sizeof(widget.text), "%s", "{{system.unknown}}");

    DisplayTextEvaluationResult result{};
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::InvalidPlaceholder, result.status);
    TEST_ASSERT_EQUAL_STRING("", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "{{system.wifi.station_ip}}{{system.wifi.station_ip}}");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::Resolved, result.status);
    TEST_ASSERT_EQUAL_STRING("192.168.1.50192.168.1.50", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "{{system.wifi.station_ip | upper}}");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::Resolved, result.status);
    TEST_ASSERT_EQUAL_STRING("192.168.1.50", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "{{system.wifi.station_ip | lower}}");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::Resolved, result.status);
    TEST_ASSERT_EQUAL_STRING("192.168.1.50", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "{{ system.wifi.station_ip | text }}");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::Resolved, result.status);
    TEST_ASSERT_EQUAL_STRING("192.168.1.50", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "{{system.bad}} {{system.wifi.station_ip}}");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.dynamic);
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::InvalidPlaceholder, result.status);
    TEST_ASSERT_EQUAL_STRING(" 192.168.1.50", result.text);

    std::snprintf(widget.text, sizeof(widget.text), "%s", "{{system.wifi.station_ip | nope}}");
    TEST_ASSERT_TRUE(evaluateDisplayTextWidget(widget, resolver, result));
    TEST_ASSERT_TRUE(result.available);
    TEST_ASSERT_EQUAL(DisplayTextEvaluationStatus::InvalidPlaceholder, result.status);
}
