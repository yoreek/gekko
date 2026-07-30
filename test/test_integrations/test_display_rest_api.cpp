#include "../test_devices/JsonSchemaSmokeValidator.h"
#include "integrations/rest/lcd1602/Lcd1602DeviceApiAdapter.h"
#include "integrations/rest/lcd1602_pin/Lcd1602PinDeviceApiAdapter.h"
#include "integrations/rest/lcd2004/Lcd2004DeviceApiAdapter.h"
#include "integrations/rest/lcd2004_pin/Lcd2004PinDeviceApiAdapter.h"
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
    config["i2cAddress"] = 0x27;
    config["rsChannel"] = 0;
    config["eChannel"] = 2;
    config["d4Channel"] = 4;
    config["d5Channel"] = 5;
    config["d6Channel"] = 6;
    config["d7Channel"] = 7;
    config["backlightChannel"] = 3;
    JsonArray deps = config.createNestedArray("deps");
    addDependency(deps, "i2c_bus", 100U);
    addCharacterLayout(config);
    (void)width;
    (void)height;
}

void addLcdPinConfig(JsonObject config) {
    config["name"] = "lcd-pin";
    config["enabled"] = true;
    config["rsPin"] = 12;
    config["ePin"] = 13;
    config["d4Pin"] = 14;
    config["d5Pin"] = 15;
    config["d6Pin"] = 16;
    config["d7Pin"] = 17;
    config["backlightPin"] = 18;
    addCharacterLayout(config);
}

enum class DisplayBackend { I2cBus, PinsDirect };

void assertCommonResponseSchema(const char* path, const char* typeName, JsonObject sourceConfig, const uint8_t width, const uint8_t height,
                                const uint32_t widgetMask, const DisplayBackend backend) {
    const bool ownsPinsDirectly = backend == DisplayBackend::PinsDirect;
    const bool isTm1637 = std::string(typeName) == "tm1637";

    StaticJsonDocument<4096> response;
    JsonObject record = response.createNestedObject("record");
    record["id"] = 42;
    record["typeName"] = typeName;
    record["configRevision"] = 1;
    JsonObject config = response.createNestedObject("config");
    config["name"] = sourceConfig["name"];
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    if (isTm1637) {
        config["panel"] = "four_digit_decimal_036";
        config["brightness"] = 5;
        config["rotation"] = 0;
        config["clkPin"] = 18;
        config["dioPin"] = 19;
    } else if (ownsPinsDirectly) {
        config["rsPin"] = 12;
        config["ePin"] = 13;
        config["d4Pin"] = 14;
        config["d5Pin"] = 15;
        config["d6Pin"] = 16;
        config["d7Pin"] = 17;
        config["backlightPin"] = 18;
    } else {
        addDependency(deps, "i2c_bus", 100U);
        config["i2cAddress"] = 0x27;
        config["rsChannel"] = 0;
        config["eChannel"] = 2;
        config["d4Channel"] = 4;
        config["d5Channel"] = 5;
        config["d6Channel"] = 6;
        config["d7Channel"] = 7;
        config["backlightChannel"] = 3;
    }
    JsonObject runtime = response.createNestedObject("runtime");
    runtime["status"] = "ready";
    runtime["effectiveStatus"] = "ready";
    JsonObject profile = runtime.createNestedObject("displayProfile");
    profile["coordinateUnit"] = isTm1637 ? 2 : 1;
    profile["logicalWidth"] = width;
    profile["logicalHeight"] = height;
    profile["supportedWidgetMask"] = widgetMask;
    profile["supportedRotationsMask"] = isTm1637 ? 5 : 1;
    profile["maxPages"] = 2;
    profile["maxWidgetsPerPage"] = 10;
    profile["pageIdCapacity"] = 16;
    profile["textCapacity"] = 128;
    profile["maxBitmapBytes"] = 0;
    profile["supportsColor"] = false;
    profile["supportsBitmap"] = false;
    profile["auxSegmentMode"] = isTm1637 ? 2 : 0;
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

    StaticJsonDocument<4096> lcd1602Pin;
    lcd1602Pin["typeName"] = "lcd1602_pin";
    addLcdPinConfig(lcd1602Pin.createNestedObject("config"));
    assertSchema("schemas/rest/v1/requests/devices-create-lcd1602_pin.request.schema.json", lcd1602Pin.as<JsonVariantConst>());
    StaticJsonDocument<1024> lcd1602PinUpdate;
    lcd1602PinUpdate["config"]["name"] = "lcd1602-pin-updated";
    assertSchema("schemas/rest/v1/requests/devices-update-lcd1602_pin.request.schema.json", lcd1602PinUpdate.as<JsonVariantConst>());
    DeviceCreateRequest lcd1602PinRequest{};
    error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Lcd1602PinDeviceApiAdapter::instance().parseCreateRequest(lcd1602Pin.as<JsonObjectConst>(), lcd1602PinRequest, error), error);

    StaticJsonDocument<4096> lcd2004Pin;
    lcd2004Pin["typeName"] = "lcd2004_pin";
    addLcdPinConfig(lcd2004Pin.createNestedObject("config"));
    assertSchema("schemas/rest/v1/requests/devices-create-lcd2004_pin.request.schema.json", lcd2004Pin.as<JsonVariantConst>());
    DeviceCreateRequest lcd2004PinRequest{};
    error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Lcd2004PinDeviceApiAdapter::instance().parseCreateRequest(lcd2004Pin.as<JsonObjectConst>(), lcd2004PinRequest, error), error);

    StaticJsonDocument<4096> tm1637;
    tm1637["typeName"] = "tm1637";
    JsonObject tmConfig = tm1637.createNestedObject("config");
    tmConfig["name"] = "tm1637";
    tmConfig["enabled"] = true;
    tmConfig["panel"] = "four_digit_decimal_036";
    tmConfig["brightness"] = 5;
    tmConfig["rotation"] = 0;
    tmConfig["clkPin"] = 18;
    tmConfig["dioPin"] = 19;
    addDigitalLayout(tmConfig);
    assertSchema("schemas/rest/v1/requests/devices-create-tm1637.request.schema.json", tm1637.as<JsonVariantConst>());
    StaticJsonDocument<1024> tmUpdate;
    tmUpdate["config"]["brightness"] = 7;
    assertSchema("schemas/rest/v1/requests/devices-update-tm1637.request.schema.json", tmUpdate.as<JsonVariantConst>());
    DeviceCreateRequest tmRequest{};
    error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(Tm1637DeviceApiAdapter::instance().parseCreateRequest(tm1637.as<JsonObjectConst>(), tmRequest, error), error);

    assertCommonResponseSchema("schemas/rest/v1/responses/devices-lcd1602.response.schema.json", "lcd1602", lcd1602["config"], 16, 2,
                               1U << 7, DisplayBackend::I2cBus);
    assertCommonResponseSchema("schemas/rest/v1/responses/devices-lcd2004.response.schema.json", "lcd2004", lcd2004["config"], 20, 4,
                               1U << 7, DisplayBackend::I2cBus);
    assertCommonResponseSchema("schemas/rest/v1/responses/devices-lcd1602_pin.response.schema.json", "lcd1602_pin", lcd1602Pin["config"],
                               16, 2, 1U << 7, DisplayBackend::PinsDirect);
    assertCommonResponseSchema("schemas/rest/v1/responses/devices-lcd2004_pin.response.schema.json", "lcd2004_pin", lcd2004Pin["config"],
                               20, 4, 1U << 7, DisplayBackend::PinsDirect);
    assertCommonResponseSchema("schemas/rest/v1/responses/devices-tm1637.response.schema.json", "tm1637", tm1637["config"], 4, 1, 1U << 1,
                               DisplayBackend::PinsDirect);
}
