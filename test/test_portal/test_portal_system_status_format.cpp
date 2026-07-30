#include "../test_devices/JsonSchemaSmokeValidator.h"
#include "portal/SystemStatusFormat.h"

#include <ArduinoJson.h>
#include <string>
#include <unity.h>

using ewfm::partitionSubtypeToString;
using ewfm::partitionTypeToString;
using ewfm::resetReasonToString;

namespace {
void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}
} // namespace

void test_system_status_format_maps_reset_reasons() {
    TEST_ASSERT_EQUAL_STRING("poweron", resetReasonToString(1));
    TEST_ASSERT_EQUAL_STRING("external", resetReasonToString(2));
    TEST_ASSERT_EQUAL_STRING("software", resetReasonToString(3));
    TEST_ASSERT_EQUAL_STRING("panic", resetReasonToString(4));
    TEST_ASSERT_EQUAL_STRING("interruptWatchdog", resetReasonToString(5));
    TEST_ASSERT_EQUAL_STRING("taskWatchdog", resetReasonToString(6));
    TEST_ASSERT_EQUAL_STRING("otherWatchdog", resetReasonToString(7));
    TEST_ASSERT_EQUAL_STRING("deepsleep", resetReasonToString(8));
    TEST_ASSERT_EQUAL_STRING("brownout", resetReasonToString(9));
    TEST_ASSERT_EQUAL_STRING("sdio", resetReasonToString(10));
    TEST_ASSERT_EQUAL_STRING("unknown", resetReasonToString(0));
    TEST_ASSERT_EQUAL_STRING("unknown", resetReasonToString(-1));
    TEST_ASSERT_EQUAL_STRING("unknown", resetReasonToString(99));
}

void test_system_status_format_maps_partition_types() {
    TEST_ASSERT_EQUAL_STRING("app", partitionTypeToString(0x00));
    TEST_ASSERT_EQUAL_STRING("data", partitionTypeToString(0x01));
    TEST_ASSERT_EQUAL_STRING("other", partitionTypeToString(0x40));
    TEST_ASSERT_EQUAL_STRING("other", partitionTypeToString(-1));
}

void test_system_status_format_maps_partition_subtypes() {
    TEST_ASSERT_EQUAL_STRING("factory", partitionSubtypeToString(0x00, 0x00));
    TEST_ASSERT_EQUAL_STRING("ota", partitionSubtypeToString(0x00, 0x10));
    TEST_ASSERT_EQUAL_STRING("ota", partitionSubtypeToString(0x00, 0x1F));
    TEST_ASSERT_EQUAL_STRING("test", partitionSubtypeToString(0x00, 0x20));
    TEST_ASSERT_EQUAL_STRING("other", partitionSubtypeToString(0x00, 0x21));

    TEST_ASSERT_EQUAL_STRING("otadata", partitionSubtypeToString(0x01, 0x00));
    TEST_ASSERT_EQUAL_STRING("phy", partitionSubtypeToString(0x01, 0x01));
    TEST_ASSERT_EQUAL_STRING("nvs", partitionSubtypeToString(0x01, 0x02));
    TEST_ASSERT_EQUAL_STRING("coredump", partitionSubtypeToString(0x01, 0x03));
    TEST_ASSERT_EQUAL_STRING("nvsKeys", partitionSubtypeToString(0x01, 0x04));
    TEST_ASSERT_EQUAL_STRING("efuse", partitionSubtypeToString(0x01, 0x05));
    TEST_ASSERT_EQUAL_STRING("fat", partitionSubtypeToString(0x01, 0x81));
    TEST_ASSERT_EQUAL_STRING("spiffs", partitionSubtypeToString(0x01, 0x82));
    TEST_ASSERT_EQUAL_STRING("other", partitionSubtypeToString(0x01, 0x7F));

    TEST_ASSERT_EQUAL_STRING("other", partitionSubtypeToString(0x40, 0x00));
}

void test_system_status_response_schema_smoke() {
    StaticJsonDocument<2048> doc;
    doc["success"] = true;
    doc["status"] = "ok";
    JsonObject chip = doc.createNestedObject("chip");
    chip["model"] = "ESP32";
    chip["revision"] = 1;
    chip["cores"] = 2;
    chip["cpuFreqMhz"] = 240;
    chip["flashSizeBytes"] = 4194304;
    JsonObject capabilities = doc.createNestedObject("capabilities");
    capabilities["rmtPulseCapture"] = true;
    doc["uptimeSeconds"] = 1234;
    doc["resetReason"] = "poweron";
    JsonObject heap = doc.createNestedObject("heap");
    heap["totalBytes"] = 327680;
    heap["freeBytes"] = 200000;
    heap["minFreeBytes"] = 150000;
    heap["maxAllocBytes"] = 120000;
    JsonObject sketch = doc.createNestedObject("sketch");
    sketch["usedBytes"] = 2450000;
    sketch["partitionBytes"] = 3145728;
    JsonArray partitions = doc.createNestedArray("partitions");
    JsonObject partition = partitions.createNestedObject();
    partition["label"] = "app0";
    partition["type"] = "app";
    partition["subtype"] = "factory";
    partition["offset"] = 65536;
    partition["sizeBytes"] = 1310720;
    JsonArray filesystems = doc.createNestedArray("filesystems");
    JsonObject fs = filesystems.createNestedObject();
    fs["label"] = "littlefs";
    fs["mounted"] = true;
    fs["totalBytes"] = 500000;
    fs["usedBytes"] = 12345;
    JsonObject nvs = doc.createNestedObject("nvs");
    nvs["usedEntries"] = 10;
    nvs["freeEntries"] = 90;
    nvs["totalEntries"] = 100;
    nvs["namespaceCount"] = 3;

    assertMatchesJsonSchema("schemas/rest/v1/responses/system-status.response.schema.json", doc.as<JsonVariantConst>());
}
