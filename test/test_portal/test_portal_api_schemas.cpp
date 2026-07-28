#include "../test_devices/JsonSchemaSmokeValidator.h"

#include <ArduinoJson.h>
#include <string>
#include <unity.h>

namespace {
void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}
} // namespace

void test_system_version_response_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["version"] = "1.2.3";
    doc["buildDate"] = "2026-07-23";
    assertMatchesJsonSchema("schemas/rest/v1/responses/system-version.response.schema.json", doc.as<JsonVariantConst>());
}

void test_ota_status_response_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["enabled"] = true;
    doc["freeSketchSpace"] = 123456;
    doc["hasError"] = false;
    assertMatchesJsonSchema("schemas/rest/v1/responses/ota-status.response.schema.json", doc.as<JsonVariantConst>());
}

void test_mqtt_status_response_schema_smoke() {
    StaticJsonDocument<384> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["enabled"] = true;
    doc["connected"] = false;
    doc["waitingForStation"] = false;
    doc["host"] = "broker.local";
    doc["port"] = 1883;
    doc["useTls"] = false;
    doc["clientId"] = "gekko-1";
    doc["hasCaCert"] = false;
    assertMatchesJsonSchema("schemas/rest/v1/responses/mqtt-status.response.schema.json", doc.as<JsonVariantConst>());
}

void test_mqtt_settings_response_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["enabled"] = true;
    doc["host"] = "broker.local";
    doc["port"] = 8883;
    doc["useTls"] = true;
    doc["clientId"] = "gekko-1";
    doc["username"] = "user";
    doc["passwordRedacted"] = true;
    doc["haDiscoveryPrefix"] = "homeassistant";
    doc["haNodeId"] = "gekko-1";
    doc["haNodeName"] = "Gekko";
    doc["hasCaCert"] = true;
    assertMatchesJsonSchema("schemas/rest/v1/responses/mqtt-settings.response.schema.json", doc.as<JsonVariantConst>());
}

void test_time_settings_response_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["enabled"] = true;
    doc["ntpServer"] = "pool.ntp.org";
    doc["timezoneId"] = "Europe/Kyiv";
    doc["syncIntervalSeconds"] = 3600;
    assertMatchesJsonSchema("schemas/rest/v1/responses/time-settings.response.schema.json", doc.as<JsonVariantConst>());
}

void test_persistence_settings_response_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["debounceMs"] = 1000;
    doc["maxDelayMs"] = 30000;
    assertMatchesJsonSchema("schemas/rest/v1/responses/persistence-settings.response.schema.json", doc.as<JsonVariantConst>());
}

void test_time_status_response_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["enabled"] = true;
    doc["synced"] = true;
    doc["waitingForStation"] = false;
    doc["ntpServer"] = "pool.ntp.org";
    doc["timezoneId"] = "Europe/Kyiv";
    doc["syncIntervalSeconds"] = 3600;
    doc["source"] = "ntp";
    doc["currentEpochUtc"] = 1711111111UL;
    doc["lastSyncEpochUtc"] = 1711110000UL;
    doc["localTimeIso8601"] = "2026-07-23T12:34:56+03:00";
    doc["utcOffsetMinutes"] = 180;
    doc["timezoneAbbrev"] = "EEST";
    assertMatchesJsonSchema("schemas/rest/v1/responses/time-status.response.schema.json", doc.as<JsonVariantConst>());
}

void test_dosejournal_response_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    JsonArray entries = doc.createNestedArray("entries");
    JsonObject entry = entries.createNestedObject();
    entry["at"] = 1711111111UL;
    entry["type"] = "schedule";
    entry["amountMl"] = 12.5;
    assertMatchesJsonSchema("schemas/rest/v1/responses/dosejournal.response.schema.json", doc.as<JsonVariantConst>());
}

void test_system_restart_response_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc["success"] = true;
    doc["rebooting"] = true;
    assertMatchesJsonSchema("schemas/rest/v1/responses/system-restart.response.schema.json", doc.as<JsonVariantConst>());
}

void test_device_setup_import_response_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["deviceCount"] = 2;
    doc["registryRevision"] = 7;
    JsonArray warnings = doc.createNestedArray("warnings");
    warnings.add("dashboard layout skipped: store unavailable");
    assertMatchesJsonSchema("schemas/rest/v1/responses/device-setup-import.response.schema.json", doc.as<JsonVariantConst>());
}

void test_wifi_scan_response_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    JsonArray networks = doc.createNestedArray("networks");
    JsonObject network = networks.createNestedObject();
    network["ssid"] = "office";
    network["rssi"] = -42;
    network["channel"] = 6;
    assertMatchesJsonSchema("schemas/rest/v1/responses/wifi-scan.response.schema.json", doc.as<JsonVariantConst>());
}

void test_wifi_configure_response_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc["success"] = true;
    doc["status"] = "accepted";
    doc["action"] = "start_ble_config";
    assertMatchesJsonSchema("schemas/rest/v1/responses/wifi-configure.response.schema.json", doc.as<JsonVariantConst>());
}

void test_metrics_placeholders_response_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["registryRevision"] = 17;
    JsonArray placeholders = doc.createNestedArray("placeholders");
    JsonObject placeholder = placeholders.createNestedObject();
    placeholder["placeholder"] = "{{dev.42.temperature}}";
    placeholder["namespace"] = "dev";
    placeholder["sourceId"] = 42;
    placeholder["sourceLabel"] = "Tank";
    placeholder["metricId"] = 1001;
    placeholder["metricKey"] = "temperature";
    placeholder["label"] = "Tank temperature";
    placeholder["valueType"] = "float";
    placeholder["available"] = true;
    placeholder["preview"] = "25.34";
    placeholder["previewNumber"] = 25.34;
    assertMatchesJsonSchema("schemas/rest/v1/responses/metrics-placeholders.response.schema.json", doc.as<JsonVariantConst>());
}

void test_metrics_values_response_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["registryRevision"] = 17;
    JsonArray values = doc.createNestedArray("values");
    JsonObject value = values.createNestedObject();
    value["namespace"] = "system";
    value["sourceId"] = 0;
    value["metricId"] = 1;
    value["metricKey"] = "uptime";
    value["valueType"] = "duration";
    value["available"] = true;
    value["value"] = "01:02:03";
    assertMatchesJsonSchema("schemas/rest/v1/responses/metrics-values.response.schema.json", doc.as<JsonVariantConst>());
}

void test_schedulepresets_response_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["deviceId"] = 17;
    JsonArray presets = doc.createNestedArray("presets");
    JsonObject filled = presets.createNestedObject();
    filled["slot"] = 0;
    filled["filled"] = true;
    filled["name"] = "reef";
    JsonArray points = filled.createNestedArray("points");
    JsonObject point = points.createNestedObject();
    point["minuteOfDay"] = 480;
    point["state"] = 75;
    JsonObject empty = presets.createNestedObject();
    empty["slot"] = 1;
    empty["filled"] = false;
    assertMatchesJsonSchema("schemas/rest/v1/responses/schedulepresets.response.schema.json", doc.as<JsonVariantConst>());
}

void test_schedulepresets_request_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["name"] = "reef";
    JsonArray points = doc.createNestedArray("points");
    JsonObject point = points.createNestedObject();
    point["minuteOfDay"] = 480;
    point["state"] = 75;
    assertMatchesJsonSchema("schemas/rest/v1/requests/schedulepresets.request.schema.json", doc.as<JsonVariantConst>());
}

void test_devices_index_response_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["registryRevision"] = 41;
    doc["pendingPersistence"] = false;
    JsonArray devices = doc.createNestedArray("devices");
    JsonObject device = devices.createNestedObject();
    JsonObject record = device.createNestedObject("record");
    record["id"] = 7;
    record["typeName"] = "dummy";
    record["configRevision"] = 3;
    JsonObject config = device.createNestedObject("config");
    config["name"] = "Lamp";
    config["enabled"] = true;
    JsonObject runtime = device.createNestedObject("runtime");
    runtime["status"] = "ready";
    runtime["effectiveStatus"] = "ready";
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-index.response.schema.json", doc.as<JsonVariantConst>());
}

void test_device_response_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["registryRevision"] = 41;
    doc["pendingPersistence"] = true;
    JsonObject device = doc.createNestedObject("device");
    JsonObject record = device.createNestedObject("record");
    record["id"] = 7;
    record["typeName"] = "dummy";
    record["configRevision"] = 3;
    JsonObject config = device.createNestedObject("config");
    config["name"] = "Lamp";
    config["enabled"] = true;
    JsonObject runtime = device.createNestedObject("runtime");
    runtime["status"] = "ready";
    runtime["effectiveStatus"] = "ready";
    assertMatchesJsonSchema("schemas/rest/v1/responses/device-response.schema.json", doc.as<JsonVariantConst>());
}

void test_device_revision_response_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["registryRevision"] = 42;
    doc["pendingPersistence"] = false;
    assertMatchesJsonSchema("schemas/rest/v1/responses/device-revision.response.schema.json", doc.as<JsonVariantConst>());
}
