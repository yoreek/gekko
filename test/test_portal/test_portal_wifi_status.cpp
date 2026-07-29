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

void test_wifi_status_response_schema_smoke() {
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    doc["wifiStatus"] = "idle";
    doc["stationIp"] = "192.168.1.10";
    doc["setupApIp"] = "192.168.4.1";
    doc["bleProvisioningSupported"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/responses/wifi-status.response.schema.json", doc.as<JsonVariantConst>());
}
