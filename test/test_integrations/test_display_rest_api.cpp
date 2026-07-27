#include "../test_devices/JsonSchemaSmokeValidator.h"
#include "integrations/rest/lcd1602/Lcd1602DeviceApiAdapter.h"
#include "integrations/rest/lcd2004/Lcd2004DeviceApiAdapter.h"
#include "integrations/rest/tm1637/Tm1637DeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <string>
#include <unity.h>

using namespace ewfm;

namespace {
void assertSchema(const char* path, const JsonVariantConst& value) {
    std::string error;
    const bool valid = json_schema_smoke::validateFile(path, value, error);
    const std::string message = std::string(path) + ": " + error;
    TEST_ASSERT_TRUE_MESSAGE(valid, message.c_str());
}

void addDependency(JsonArray deps, const char* role, const uint32_t deviceId) {
    JsonObject dependency = deps.createNestedObject();
    dependency["role"] = role;
    dependency["deviceId"] = deviceId;
}

void addCharacterLayout(JsonObject config) {
    JsonObject layout = config.createNestedObject("layout");
    layout["schemaVersion"] = 3;
    layout["activePageId"] = "main";
    JsonArray pages = layout.createNestedArray("pages");
    JsonObject page = pages.createNestedObject();
    page["id"] = "main";
    JsonArray widgets = page.createNestedArray("widgets");
    JsonObject widget = widgets.createNestedObject();
    widget["id"] = "status";
    widget["type"] = "character";
    widget["x"] = 0;
    widget["y"] = 0;
    widget["width"] = 8;
    widget["height"] = 2;
    widget["text"] = "0123456789";
    widget["wrap"] = true;
    widget["autoSize"] = true;
}

void addDigitalLayout(JsonObject config) {
    JsonObject layout = config.createNestedObject("layout");
    layout["schemaVersion"] = 3;
    layout["activePageId"] = "main";
    JsonArray pages = layout.createNestedArray("pages");
    JsonObject page = pages.createNestedObject();
    page["id"] = "main";
    JsonArray widgets = page.createNestedArray("widgets");
    JsonObject widget = widgets.createNestedObject();
    widget["id"] = "value";
    widget["type"] = "digital";
    widget["x"] = 0;
    widget["y"] = 0;
    widget["width"] = 4;
    widget["height"] = 1;
    widget["text"] = "12.34";
}

void addLcdConfig(JsonObject config, const char* name, const uint8_t width, const uint8_t height) {
    config["name"] = name;
    config["enabled"] = true;
    config["rsChannel"] = 0;
    config["eChannel"] = 1;
    config["d4Channel"] = 2;
    config["d5Channel"] = 3;
    config["d6Channel"] = 4;
    config["d7Channel"] = 5;
    config["backlightChannel"] = 6;
    JsonArray deps = config.createNestedArray("deps");
    for (uint8_t index = 0; index < 7; ++index) {
        addDependency(deps, "switch", 100U + index);
    }
    addCharacterLayout(config);
    (void)width;
    (void)height;
}

void assertCommonResponseSchema(const char* path, const char* typeName, JsonObject sourceConfig, const uint8_t width, const uint8_t height,
                                const uint32_t widgetMask) {
    StaticJsonDocument<4096> response;
    JsonObject record = response.createNestedObject("record");
    record["id"] = 42;
    record["typeName"] = typeName;
    record["configRevision"] = 1;
    JsonObject config = response.createNestedObject("config");
    config["name"] = sourceConfig["name"];
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    const uint8_t dependencyCount = typeName[0] == 't' ? 2U : 7U;
    for (uint8_t index = 0; index < dependencyCount; ++index) {
        addDependency(deps, "switch", 100U + index);
    }
    if (typeName[0] == 't') {
        config["panel"] = "four_digit_decimal_036";
        config["brightness"] = 5;
        config["rotation"] = 0;
    } else {
        config["rsChannel"] = 0;
        config["eChannel"] = 1;
        config["d4Channel"] = 2;
        config["d5Channel"] = 3;
        config["d6Channel"] = 4;
        config["d7Channel"] = 5;
        config["backlightChannel"] = 6;
    }
    JsonObject runtime = response.createNestedObject("runtime");
    runtime["status"] = "ready";
    runtime["effectiveStatus"] = "ready";
    JsonObject profile = runtime.createNestedObject("displayProfile");
    profile["coordinateUnit"] = typeName[0] == 't' ? 2 : 1;
    profile["logicalWidth"] = width;
    profile["logicalHeight"] = height;
    profile["supportedWidgetMask"] = widgetMask;
    profile["supportedRotationsMask"] = typeName[0] == 't' ? 5 : 1;
    profile["maxPages"] = 2;
    profile["maxWidgetsPerPage"] = 10;
    profile["pageIdCapacity"] = 16;
    profile["textCapacity"] = 128;
    profile["maxBitmapBytes"] = 0;
    profile["supportsColor"] = false;
    profile["supportsBitmap"] = false;
    profile["auxSegmentMode"] = typeName[0] == 't' ? 2 : 0;
    assertSchema(path, response.as<JsonVariantConst>());
}
} // namespace

void test_new_display_rest_api_schemas_and_parsers() {
    StaticJsonDocument<4096> lcd1602;
    lcd1602["typeName"] = "lcd1602";
    addLcdConfig(lcd1602.createNestedObject("config"), "lcd1602", 16, 2);
    assertSchema("schemas/rest/v1/requests/devices-create-lcd1602.request.schema.json", lcd1602.as<JsonVariantConst>());
    StaticJsonDocument<1024> lcd1602Update;
    lcd1602Update["config"]["name"] = "lcd1602-updated";
    assertSchema("schemas/rest/v1/requests/devices-update-lcd1602.request.schema.json", lcd1602Update.as<JsonVariantConst>());
    DeviceCreateRequest lcd1602Request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(Lcd1602DeviceApiAdapter::instance().parseCreateRequest(lcd1602.as<JsonObjectConst>(), lcd1602Request, error),
                             error);

    StaticJsonDocument<4096> lcd2004;
    lcd2004["typeName"] = "lcd2004";
    addLcdConfig(lcd2004.createNestedObject("config"), "lcd2004", 20, 4);
    assertSchema("schemas/rest/v1/requests/devices-create-lcd2004.request.schema.json", lcd2004.as<JsonVariantConst>());
    DeviceCreateRequest lcd2004Request{};
    error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(Lcd2004DeviceApiAdapter::instance().parseCreateRequest(lcd2004.as<JsonObjectConst>(), lcd2004Request, error),
                             error);

    StaticJsonDocument<4096> tm1637;
    tm1637["typeName"] = "tm1637";
    JsonObject tmConfig = tm1637.createNestedObject("config");
    tmConfig["name"] = "tm1637";
    tmConfig["enabled"] = true;
    tmConfig["panel"] = "four_digit_decimal_036";
    tmConfig["brightness"] = 5;
    tmConfig["rotation"] = 0;
    JsonArray tmDeps = tmConfig.createNestedArray("deps");
    addDependency(tmDeps, "switch", 201);
    addDependency(tmDeps, "switch", 202);
    addDigitalLayout(tmConfig);
    assertSchema("schemas/rest/v1/requests/devices-create-tm1637.request.schema.json", tm1637.as<JsonVariantConst>());
    StaticJsonDocument<1024> tmUpdate;
    tmUpdate["config"]["brightness"] = 7;
    assertSchema("schemas/rest/v1/requests/devices-update-tm1637.request.schema.json", tmUpdate.as<JsonVariantConst>());
    DeviceCreateRequest tmRequest{};
    error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(Tm1637DeviceApiAdapter::instance().parseCreateRequest(tm1637.as<JsonObjectConst>(), tmRequest, error), error);

    assertCommonResponseSchema("schemas/rest/v1/responses/devices-lcd1602.response.schema.json", "lcd1602", lcd1602["config"], 16, 2,
                               1U << 7);
    assertCommonResponseSchema("schemas/rest/v1/responses/devices-lcd2004.response.schema.json", "lcd2004", lcd2004["config"], 20, 4,
                               1U << 7);
    assertCommonResponseSchema("schemas/rest/v1/responses/devices-tm1637.response.schema.json", "tm1637", tm1637["config"], 4, 1, 1U << 1);
}
