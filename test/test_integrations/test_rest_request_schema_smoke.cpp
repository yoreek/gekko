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

void test_wifi_configure_request_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["ssid"] = "office";
    doc["password"] = "secret";
    assertMatchesJsonSchema("schemas/rest/v1/requests/wifi-configure.request.schema.json", doc.as<JsonVariantConst>());
}

void test_mqtt_settings_request_schema_smoke() {
    StaticJsonDocument<512> doc;
    doc["enabled"] = true;
    doc["host"] = "broker.local";
    doc["port"] = 8883;
    doc["useTls"] = true;
    doc["clientId"] = "gekko-1";
    doc["username"] = "user";
    doc["password"] = "secret";
    doc["haDiscoveryPrefix"] = "homeassistant";
    doc["haNodeId"] = "gekko-1";
    doc["haNodeName"] = "Gekko";
    assertMatchesJsonSchema("schemas/rest/v1/requests/mqtt-settings.request.schema.json", doc.as<JsonVariantConst>());
}

void test_time_settings_request_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["enabled"] = true;
    doc["ntpServer"] = "pool.ntp.org";
    doc["timezoneId"] = "Europe/Kyiv";
    doc["syncIntervalSeconds"] = 3600;
    assertMatchesJsonSchema("schemas/rest/v1/requests/time-settings.request.schema.json", doc.as<JsonVariantConst>());
}

void test_persistence_settings_request_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc["debounceMs"] = 1000;
    doc["maxDelayMs"] = 30000;
    assertMatchesJsonSchema("schemas/rest/v1/requests/persistence-settings.request.schema.json", doc.as<JsonVariantConst>());
}

void test_manual_time_request_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc["iso8601"] = "2026-07-23T12:34:56+03:00";
    assertMatchesJsonSchema("schemas/rest/v1/requests/manual-time.request.schema.json", doc.as<JsonVariantConst>());
}

void test_device_setup_import_request_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc["bundle"] = "device-setup.ndjson";
    assertMatchesJsonSchema("schemas/rest/v1/requests/device-setup-import.request.schema.json", doc.as<JsonVariantConst>());
}

void test_mqtt_ca_cert_request_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc["cert"] = "-----BEGIN CERTIFICATE-----";
    assertMatchesJsonSchema("schemas/rest/v1/requests/mqtt-ca-cert.request.schema.json", doc.as<JsonVariantConst>());
}

void test_ota_upload_request_schema_smoke() {
    StaticJsonDocument<128> doc;
    doc.set("firmware-image");
    assertMatchesJsonSchema("schemas/rest/v1/requests/ota-upload.request.schema.json", doc.as<JsonVariantConst>());
}
