// Characterization tests pinning the observable REST-adapter behavior (parsed fields, exact
// error strings, partial-update semantics) before the adapters migrate onto the shared
// TypedDeviceApiAdapter base. Any assertion change here means the refactor changed behavior.
#include "../test_devices/JsonSchemaSmokeValidator.h"
#include "devices/analog/composer/AnalogOutputComposerDevice.h"
#include "devices/analog/input/ads1115/Ads1115HubDevice.h"
#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDevice.h"
#include "devices/analog/ledc/LedcAnalogOutputDevice.h"
#include "devices/dosing/DosingPumpDevice.h"
#include "devices/expander/Pcf8574ExpanderDevice.h"
#include "devices/expander/Pcf8575ExpanderDevice.h"
#include "devices/schedule/ScheduleDevice.h"
#include "devices/sensors/binary/BinarySensorDevice.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "devices/switch/auto/AutoSwitchDevice.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "devices/thermostat/ThermostatDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/analog_input/Ads1115HubDeviceApiAdapter.h"
#include "integrations/rest/analog_input/Cd74hc4067HubDeviceApiAdapter.h"
#include "integrations/rest/analog_output/AnalogOutputComposerDeviceApiAdapter.h"
#include "integrations/rest/analog_output/LedcAnalogOutputDeviceApiAdapter.h"
#include "integrations/rest/auto_switch/AutoSwitchDeviceApiAdapter.h"
#include "integrations/rest/binary_sensor/BinarySensorDeviceApiAdapter.h"
#include "integrations/rest/dosing_pump/DosingPumpDeviceApiAdapter.h"
#include "integrations/rest/expander/Pcf8574ExpanderDeviceApiAdapter.h"
#include "integrations/rest/expander/Pcf8575ExpanderDeviceApiAdapter.h"
#include "integrations/rest/expander/PortExpanderSwitchDeviceApiAdapter.h"
#include "integrations/rest/ntc_thermistor/NtcThermistorTemperatureSensorDeviceApiAdapter.h"
#include "integrations/rest/schedule/ScheduleDeviceApiAdapter.h"
#include "integrations/rest/thermostat/ThermostatDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <string>
#include <unity.h>

using namespace ewfm;

namespace {
void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}

class FakeBinarySensorDriver final : public IGpioInputDriver {
public:
    bool configureInput(uint8_t, GpioInputPullMode) override {
        return true;
    }
    bool read(uint8_t, bool& outLevel) override {
        outLevel = level;
        return true;
    }
    void release(uint8_t) override {}

    bool level{false};
};

BinarySensorDeviceConfigV1 makeBinarySensorConfig() {
    BinarySensorDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "float switch");
    config.gpioPin = 27U;
    config.pullMode = static_cast<uint8_t>(GpioInputPullMode::PullDown);
    config.inverted = 0U;
    config.debounceMs = 120U;
    return config;
}

AutoSwitchDeviceConfigV1 makeAutoSwitchConfig() {
    AutoSwitchDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "auto switch");
    config.pauseDurationSeconds = 900U;
    return config;
}

DeviceConfigBlob encodeAutoSwitchBlob(const AutoSwitchDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, config, buffer, autoSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, autoSwitchDeviceConfigSize(config)));
    return blob;
}

DosingPumpDeviceConfigV1 makeDosingPumpConfig() {
    DosingPumpDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "Calcium");
    config.speedMilliMlPerSec = 1250U;
    config.containerCapacityMl = 500U;
    config.thresholdPercent = 15U;
    config.blockAutoWhenEmpty = 1U;
    config.scheduleMode = static_cast<uint8_t>(DosingScheduleMode::Daily);
    config.everyDays = 2U;
    config.anchorDay = 20647U;
    config.doseCount = 1U;
    config.doses[0] = DosingPumpDoseV1{8U * 60U + 30U, 1230U};
    return config;
}

DeviceConfigBlob encodeDosingPumpBlob(const DosingPumpDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, config, buffer, dosingPumpDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, dosingPumpDeviceConfigSize(config)));
    return blob;
}

ScheduleDeviceConfigV1 makeScheduleConfig() {
    ScheduleDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "irrigation");
    config.ruleCount = 1U;
    config.rules[0].enabled = 1U;
    config.rules[0].weekDays = 0x3EU;
    config.rules[0].startMinuteOfDay = 480U;
    config.rules[0].endMinuteOfDay = 1200U;
    config.rules[0].mode = static_cast<uint8_t>(ScheduleRuleMode::Interval);
    config.rules[0].intervalsPerWindow = 3U;
    config.rules[0].durationMinutes = 10U;
    return config;
}

DeviceConfigBlob encodeScheduleBlob(const ScheduleDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, config, buffer, scheduleDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, scheduleDeviceConfigSize(config)));
    return blob;
}

ThermostatDeviceConfigV1 makeThermostatConfig() {
    ThermostatDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "Tank");
    config.mode = static_cast<uint8_t>(ThermostatMode::Heat);
    config.algorithm = static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis);
    config.targetMilliCelsius = 25000;
    config.minSafeMilliCelsius = 0;
    config.maxSafeMilliCelsius = 50000;
    config.hysteresisCentiCelsius = 50;
    config.checkIntervalMs = 1000U;
    config.sensorTimeoutMs = 3000U;
    config.retryAfterErrorMs = 30000U;
    config.minSwitchIntervalMs = 5000U;
    return config;
}

DeviceConfigBlob encodeThermostatBlob(const ThermostatDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, config, buffer, thermostatDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, thermostatDeviceConfigSize(config)));
    return blob;
}

Ads1115HubDeviceConfigV1 makeAds1115HubConfig() {
    Ads1115HubDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "ADS1115");
    config.i2cAddress = 0x48U;
    config.gain = static_cast<uint8_t>(Ads1115Gain::Fsr4096);
    config.dataRateSps = static_cast<uint8_t>(Ads1115DataRate::Sps250);
    return config;
}

DeviceConfigBlob encodeAds1115HubBlob(const Ads1115HubDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ads1115HubDeviceConfigV1::kMagic, config, buffer, ads1115HubDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, ads1115HubDeviceConfigSize(config)));
    return blob;
}

Cd74hc4067HubDeviceConfigV1 makeCd74hc4067HubConfig() {
    Cd74hc4067HubDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "MUX");
    config.selectPins[0] = 16U;
    config.selectPins[1] = 17U;
    config.selectPins[2] = 18U;
    config.selectPins[3] = 19U;
    config.enablePin = kCd74hc4067UnusedPin;
    config.sigPin = 34U;
    config.sigAttenuation = static_cast<uint8_t>(AdcAttenuation::Db11);
    return config;
}

DeviceConfigBlob encodeCd74hc4067HubBlob(const Cd74hc4067HubDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic, config, buffer, cd74hc4067HubDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, cd74hc4067HubDeviceConfigSize(config)));
    return blob;
}

Pcf857xExpanderConfigV2 makePcf8574ExpanderConfig() {
    Pcf857xExpanderConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "PCF8574");
    config.i2cAddress = 0x20U;
    config.inverted = 1U;
    return config;
}

DeviceConfigBlob encodePcf8574ExpanderBlob(const Pcf857xExpanderConfigV2& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, config, buffer, pcf857xExpanderConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, pcf857xExpanderConfigSize(config)));
    return blob;
}

Pcf857xExpanderConfigV2 makePcf8575ExpanderConfig() {
    Pcf857xExpanderConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "PCF8575");
    config.i2cAddress = 0x20U;
    config.inverted = 0U;
    return config;
}

DeviceConfigBlob encodePcf8575ExpanderBlob(const Pcf857xExpanderConfigV2& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, config, buffer, pcf857xExpanderConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, pcf857xExpanderConfigSize(config)));
    return blob;
}

PortExpanderSwitchDeviceConfigV3 makePortExpanderSwitchConfig() {
    PortExpanderSwitchDeviceConfigV3 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "ch5");
    config.restorePreviousState = true;
    config.startupState = true;
    config.safeState = false;
    config.inverted = false;
    config.channel = 5U;
    return config;
}

DeviceConfigBlob encodePortExpanderSwitchBlob(const PortExpanderSwitchDeviceConfigV3& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, config, buffer, portExpanderSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, portExpanderSwitchDeviceConfigSize(config)));
    return blob;
}
} // namespace

void test_ledc_analog_output_api_adapter_parses_single_output_config() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "analog_output";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "dimmable output";
    config["enabled"] = true;
    config["restorePreviousState"] = true;
    config["startupState"] = 35;
    config["safeState"] = 10;
    config["pin"] = 13;
    config["ledcChannel"] = 2;
    config["frequencyHz"] = 5000;
    config["dutyBits"] = 12;
    config["inverted"] = false;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(LedcAnalogOutputDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kLedcAnalogOutputDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kLedcAnalogOutputDeviceConfigVersion, request.configVersion);

    LedcAnalogOutputDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeLedcAnalogOutputDeviceConfig(request.configBlob.data(), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(35U), parsed.startupState);
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(10U), parsed.safeState);
    TEST_ASSERT_EQUAL_UINT8(13U, parsed.pin);
    TEST_ASSERT_EQUAL_UINT8(2U, parsed.ledcChannel);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
}

void test_analog_output_composer_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "analog_output_composer";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "group mode";
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject firstDep = deps.createNestedObject();
    firstDep["role"] = "analog_output";
    firstDep["deviceId"] = 41;
    JsonObject secondDep = deps.createNestedObject();
    secondDep["role"] = "analog_output";
    secondDep["deviceId"] = 42;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-analog_output_composer.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(AnalogOutputComposerDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(23U, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(1U, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("group mode", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(2U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(41U, request.dependencyLinks()[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(42U, request.dependencyLinks()[1].deviceId);

    AnalogOutputComposerDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeAnalogOutputComposerDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                            request.configBlob.size(), parsed));
    TEST_ASSERT_TRUE(parsed.enabled);
}

void test_analog_output_composer_api_adapter_partial_update_preserves_deps() {
    AnalogOutputComposerDeviceConfigV1 current{};
    current.enabled = true;
    std::snprintf(current.name, sizeof(current.name), "%s", "group mode");
    AnalogOutputComposerDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "group mode 2";

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-analog_output_composer.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        AnalogOutputComposerDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(1U, request.configVersion);
    TEST_ASSERT_FALSE(request.depsProvided);

    AnalogOutputComposerDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeAnalogOutputComposerDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                            request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_STRING("group mode 2", parsed.name);
}

void test_analog_output_composer_api_adapter_serializes_record() {
    AnalogOutputComposerDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "group mode");
    const DeviceConfigBlob blob = [&]() {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        DeviceConfigBlob out{};
        TEST_ASSERT_TRUE(encodeFixedConfigBlob(AnalogOutputComposerDeviceConfigV1::kMagic, config, buffer,
                                               analogOutputComposerDeviceConfigSize(config)));
        TEST_ASSERT_TRUE(out.assign(buffer, analogOutputComposerDeviceConfigSize(config)));
        return out;
    }();
    DeviceRegistryEntry record{};
    record.header.deviceId = 70U;
    record.header.typeId = 23U;
    record.header.configVersion = 1U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    AnalogOutputComposerDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    AnalogOutputComposerDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-analog_output_composer.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(70U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("analog_output_composer", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("group mode", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("scheduled", output["runtime"]["output"]["mode"].as<const char*>());
}

void test_schedule_api_adapter_parses_create_request() {
    StaticJsonDocument<2048> doc;
    doc["typeName"] = "schedule";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "irrigation";
    config["enabled"] = true;
    JsonArray rules = config.createNestedArray("rules");
    JsonObject rule = rules.createNestedObject();
    rule["enabled"] = true;
    JsonArray weekDays = rule.createNestedArray("weekDays");
    for (int day = 1; day <= 5; ++day) { // Monday..Friday -> mask 0x3E
        weekDays.add(day);
    }
    rule["startMinuteOfDay"] = 8 * 60;
    rule["endMinuteOfDay"] = 20 * 60;
    rule["mode"] = "interval";
    rule["intervalsPerWindow"] = 3;
    rule["durationMinutes"] = 10;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-schedule.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(ScheduleDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kScheduleDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kScheduleDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("irrigation", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    ScheduleDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        ScheduleDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(1, parsed.ruleCount);
    TEST_ASSERT_EQUAL_UINT8(0x3E, parsed.rules[0].weekDays);
    TEST_ASSERT_EQUAL_UINT16(480, parsed.rules[0].startMinuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(1200, parsed.rules[0].endMinuteOfDay);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ScheduleRuleMode::Interval), parsed.rules[0].mode);
    TEST_ASSERT_EQUAL_UINT16(3, parsed.rules[0].intervalsPerWindow);
    TEST_ASSERT_EQUAL_UINT16(10, parsed.rules[0].durationMinutes);
}

void test_schedule_api_adapter_serializes_record() {
    const ScheduleDeviceConfigV1 config = makeScheduleConfig();
    const DeviceConfigBlob blob = encodeScheduleBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 23U;
    record.header.typeId = kScheduleDeviceTypeId;
    record.header.configVersion = kScheduleDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    ScheduleDevice device(config);
    device.bindDeviceIdentity(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    ScheduleDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-schedule.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(23U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("schedule", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("irrigation", output["config"]["name"].as<const char*>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_schedule_api_adapter_rejects_missing_config() {
    StaticJsonDocument<128> doc;
    doc["typeName"] = "schedule";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(ScheduleDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("device config is required", error);
}

void test_schedule_api_adapter_rejects_non_array_rules() {
    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "irrigation";
    config["rules"] = "bad";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(ScheduleDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("schedule rules must be an array", error);
}

void test_schedule_api_adapter_partial_update_preserves_rules() {
    ScheduleDeviceConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "irrigation");
    current.ruleCount = 1;
    current.rules[0].weekDays = 0x41; // Saturday + Sunday
    current.rules[0].startMinuteOfDay = 60;
    current.rules[0].endMinuteOfDay = 120;
    ScheduleDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "renamed";
    config["enabled"] = false;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-schedule.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(ScheduleDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kScheduleDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("renamed", request.baseConfig.name);
    TEST_ASSERT_FALSE(request.isEnabled());

    ScheduleDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        ScheduleDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_STRING("renamed", parsed.name);
    TEST_ASSERT_EQUAL_UINT8(1, parsed.ruleCount);
    TEST_ASSERT_EQUAL_UINT8(0x41, parsed.rules[0].weekDays);
    TEST_ASSERT_EQUAL_UINT16(60, parsed.rules[0].startMinuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(120, parsed.rules[0].endMinuteOfDay);
}

void test_schedule_api_adapter_rejects_missing_update_config() {
    ScheduleDeviceConfigV1 current{};
    ScheduleDevice runtime(current);

    StaticJsonDocument<64> doc;
    doc["command"] = "updateConfig";

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(ScheduleDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_STRING("device config is required", error);
}

void test_ads1115_hub_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "ads1115_hub";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "ADS1115";
    config["enabled"] = true;
    config["i2cAddress"] = 72;
    config["gain"] = "fsr4096";
    config["dataRateSps"] = "250";
    JsonArray deps = config.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-ads1115_hub.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Ads1115HubDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kAds1115HubTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kAds1115HubConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("ADS1115", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(17U, request.dependencyLinks()[0].deviceId);

    Ads1115HubDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        Ads1115HubDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(72U, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Ads1115Gain::Fsr4096), parsed.gain);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Ads1115DataRate::Sps250), parsed.dataRateSps);
}

void test_ads1115_hub_api_adapter_partial_update_preserves_bus_and_gain() {
    Ads1115HubDeviceConfigV1 current = makeAds1115HubConfig();
    Ads1115HubDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["gain"] = "fsr0256";

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-ads1115_hub.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Ads1115HubDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kAds1115HubConfigVersion, request.configVersion);
    TEST_ASSERT_FALSE(request.depsProvided);

    Ads1115HubDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        Ads1115HubDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(current.i2cAddress, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Ads1115Gain::Fsr0256), parsed.gain);
    TEST_ASSERT_EQUAL_UINT8(current.dataRateSps, parsed.dataRateSps);
}

void test_ads1115_hub_api_adapter_serializes_record() {
    const Ads1115HubDeviceConfigV1 config = makeAds1115HubConfig();
    const DeviceConfigBlob blob = encodeAds1115HubBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 55U;
    record.header.typeId = kAds1115HubTypeId;
    record.header.configVersion = kAds1115HubConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    Ads1115HubDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Ads1115HubDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-ads1115_hub.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(55U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("ads1115_hub", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ADS1115", output["config"]["name"].as<const char*>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_cd74hc4067_hub_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "cd74hc4067_hub";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "MUX";
    config["enabled"] = true;
    JsonArray selectPins = config.createNestedArray("selectPins");
    selectPins.add(16);
    selectPins.add(17);
    selectPins.add(18);
    selectPins.add(19);
    config["enablePin"] = 255;
    config["sigPin"] = 34;
    config["sigAttenuation"] = "11db";

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-cd74hc4067_hub.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Cd74hc4067HubDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kCd74hc4067HubTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kCd74hc4067HubConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("MUX", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    Cd74hc4067HubDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(16U, parsed.selectPins[0]);
    TEST_ASSERT_EQUAL_UINT8(17U, parsed.selectPins[1]);
    TEST_ASSERT_EQUAL_UINT8(18U, parsed.selectPins[2]);
    TEST_ASSERT_EQUAL_UINT8(19U, parsed.selectPins[3]);
    TEST_ASSERT_EQUAL_UINT8(kCd74hc4067UnusedPin, parsed.enablePin);
    TEST_ASSERT_EQUAL_UINT8(34U, parsed.sigPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db11), parsed.sigAttenuation);
}

void test_cd74hc4067_hub_api_adapter_partial_update_preserves_select_pins() {
    Cd74hc4067HubDeviceConfigV1 current = makeCd74hc4067HubConfig();
    Cd74hc4067HubDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["sigAttenuation"] = "6db";

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-cd74hc4067_hub.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        Cd74hc4067HubDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kCd74hc4067HubConfigVersion, request.configVersion);

    Cd74hc4067HubDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(current.selectPins[0], parsed.selectPins[0]);
    TEST_ASSERT_EQUAL_UINT8(current.selectPins[1], parsed.selectPins[1]);
    TEST_ASSERT_EQUAL_UINT8(current.selectPins[2], parsed.selectPins[2]);
    TEST_ASSERT_EQUAL_UINT8(current.selectPins[3], parsed.selectPins[3]);
    TEST_ASSERT_EQUAL_UINT8(current.enablePin, parsed.enablePin);
    TEST_ASSERT_EQUAL_UINT8(current.sigPin, parsed.sigPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db6), parsed.sigAttenuation);
}

void test_cd74hc4067_hub_api_adapter_serializes_record() {
    const Cd74hc4067HubDeviceConfigV1 config = makeCd74hc4067HubConfig();
    const DeviceConfigBlob blob = encodeCd74hc4067HubBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 57U;
    record.header.typeId = kCd74hc4067HubTypeId;
    record.header.configVersion = kCd74hc4067HubConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    Cd74hc4067HubDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Cd74hc4067HubDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-cd74hc4067_hub.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(57U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("cd74hc4067_hub", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("MUX", output["config"]["name"].as<const char*>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_pcf8574_expander_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "pcf8574_expander";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "PCF8574";
    config["enabled"] = true;
    config["i2cAddress"] = 32;
    config["inverted"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-pcf8574_expander.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Pcf8574ExpanderDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(12U, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kPcf857xExpanderConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("PCF8574", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(17U, request.dependencyLinks()[0].deviceId);

    Pcf857xExpanderConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(32U, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.inverted);
}

void test_pcf8574_expander_api_adapter_partial_update_preserves_address() {
    Pcf857xExpanderConfigV2 current = makePcf8574ExpanderConfig();
    Pcf8574ExpanderDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["inverted"] = false;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-pcf8574_expander.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        Pcf8574ExpanderDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kPcf857xExpanderConfigVersion, request.configVersion);
    TEST_ASSERT_TRUE(request.depsProvided);

    Pcf857xExpanderConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(current.i2cAddress, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(0U, parsed.inverted);
}

void test_pcf8574_expander_api_adapter_serializes_record() {
    const Pcf857xExpanderConfigV2 config = makePcf8574ExpanderConfig();
    const DeviceConfigBlob blob = encodePcf8574ExpanderBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 61U;
    record.header.typeId = 12U;
    record.header.configVersion = kPcf857xExpanderConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    Pcf8574ExpanderDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Pcf8574ExpanderDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-pcf8574_expander.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(61U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("pcf8574_expander", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("PCF8574", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(8U, output["runtime"]["channelCount"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(0U, output["runtime"]["channelStates"].as<uint32_t>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_pcf8575_expander_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "pcf8575_expander";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "PCF8575";
    config["enabled"] = true;
    config["i2cAddress"] = 32;
    config["inverted"] = false;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-pcf8575_expander.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Pcf8575ExpanderDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(13U, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kPcf857xExpanderConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("PCF8575", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(17U, request.dependencyLinks()[0].deviceId);

    Pcf857xExpanderConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(32U, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(0U, parsed.inverted);
}

void test_pcf8575_expander_api_adapter_partial_update_preserves_address() {
    Pcf857xExpanderConfigV2 current = makePcf8575ExpanderConfig();
    Pcf8575ExpanderDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["inverted"] = true;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-pcf8575_expander.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        Pcf8575ExpanderDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kPcf857xExpanderConfigVersion, request.configVersion);
    TEST_ASSERT_TRUE(request.depsProvided);

    Pcf857xExpanderConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(current.i2cAddress, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.inverted);
}

void test_pcf8575_expander_api_adapter_serializes_record() {
    const Pcf857xExpanderConfigV2 config = makePcf8575ExpanderConfig();
    const DeviceConfigBlob blob = encodePcf8575ExpanderBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 62U;
    record.header.typeId = 13U;
    record.header.configVersion = kPcf857xExpanderConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    Pcf8575ExpanderDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Pcf8575ExpanderDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-pcf8575_expander.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(62U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("pcf8575_expander", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("PCF8575", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(16U, output["runtime"]["channelCount"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(0U, output["runtime"]["channelStates"].as<uint32_t>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_port_expander_switch_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "port_expander_switch";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "ch5";
    config["enabled"] = true;
    config["restorePreviousState"] = true;
    config["startupState"] = true;
    config["safeState"] = false;
    config["inverted"] = false;
    config["channel"] = 5;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject expanderDep = deps.createNestedObject();
    expanderDep["role"] = "port_expander";
    expanderDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-port_expander_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(PortExpanderSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(14U, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(3U, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("ch5", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(17U, request.dependencyLinks()[0].deviceId);

    PortExpanderSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(decodePortExpanderSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                          request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(5U, parsed.channel);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
    TEST_ASSERT_TRUE(parsed.startupState);
    TEST_ASSERT_FALSE(parsed.safeState);
}

void test_port_expander_switch_api_adapter_partial_update_preserves_channel() {
    PortExpanderSwitchDeviceConfigV3 current = makePortExpanderSwitchConfig();
    PortExpanderSwitchDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["inverted"] = true;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject expanderDep = deps.createNestedObject();
    expanderDep["role"] = "port_expander";
    expanderDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-port_expander_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        PortExpanderSwitchDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(3U, request.configVersion);
    TEST_ASSERT_TRUE(request.depsProvided);

    PortExpanderSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(decodePortExpanderSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                          request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(current.channel, parsed.channel);
    TEST_ASSERT_TRUE(parsed.inverted);
}

void test_port_expander_switch_api_adapter_serializes_record() {
    const PortExpanderSwitchDeviceConfigV3 config = makePortExpanderSwitchConfig();
    const DeviceConfigBlob blob = encodePortExpanderSwitchBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 63U;
    record.header.typeId = 14U;
    record.header.configVersion = 3U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    PortExpanderSwitchDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    PortExpanderSwitchDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-port_expander_switch.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(63U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("port_expander_switch", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ch5", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(5U, output["config"]["channel"].as<uint32_t>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["state"].is<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["physicalLevel"].is<bool>());
}

void test_thermostat_api_adapter_parses_create_request() {
    StaticJsonDocument<2048> doc;
    doc["typeName"] = "thermostat";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "Tank";
    config["enabled"] = true;
    config["mode"] = "heat";
    config["algorithm"] = "hysteresis";
    config["targetCelsius"] = 25.0;
    config["minSafeCelsius"] = 0.0;
    config["maxSafeCelsius"] = 50.0;
    config["hysteresisCelsius"] = 0.5;
    config["checkIntervalMs"] = 1000;
    config["sensorTimeoutMs"] = 3000;
    config["retryAfterErrorMs"] = 30000;
    config["minSwitchIntervalMs"] = 5000;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject temperatureDep = deps.createNestedObject();
    temperatureDep["role"] = "temperature_sensor";
    temperatureDep["deviceId"] = 40;
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 41;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-thermostat.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(ThermostatDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kThermostatDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kThermostatDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("Tank", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(2U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(40U, request.dependencyLinks()[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(41U, request.dependencyLinks()[1].deviceId);

    ThermostatDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        ThermostatDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ThermostatMode::Heat), parsed.mode);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis), parsed.algorithm);
    TEST_ASSERT_EQUAL_INT32(25000, parsed.targetMilliCelsius);
    TEST_ASSERT_EQUAL_INT32(0, parsed.minSafeMilliCelsius);
    TEST_ASSERT_EQUAL_INT32(50000, parsed.maxSafeMilliCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, parsed.hysteresisCentiCelsius);
    TEST_ASSERT_EQUAL_UINT32(1000U, parsed.checkIntervalMs);
    TEST_ASSERT_EQUAL_UINT32(3000U, parsed.sensorTimeoutMs);
    TEST_ASSERT_EQUAL_UINT32(30000U, parsed.retryAfterErrorMs);
    TEST_ASSERT_EQUAL_UINT32(5000U, parsed.minSwitchIntervalMs);
}

void test_thermostat_api_adapter_validates_update_request_schema() {
    StaticJsonDocument<512> doc;
    JsonObject config = doc.createNestedObject("config");
    config["targetCelsius"] = 24.5;
    config["hysteresisCelsius"] = 0.75;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject temperatureDep = deps.createNestedObject();
    temperatureDep["role"] = "temperature_sensor";
    temperatureDep["deviceId"] = 40;
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 41;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-thermostat.request.schema.json", doc.as<JsonVariantConst>());
}

void test_thermostat_api_adapter_serializes_record() {
    const ThermostatDeviceConfigV1 config = makeThermostatConfig();
    const DeviceConfigBlob blob = encodeThermostatBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 44U;
    record.header.typeId = kThermostatDeviceTypeId;
    record.header.configVersion = kThermostatDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    ThermostatDevice device(config);
    device.bindDeviceIdentity(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    ThermostatDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-thermostat.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(44U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("thermostat", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Tank", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["temperature"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["controlStatus"].is<const char*>());
}

void test_ntc_thermistor_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "ntc_thermistor_temperature_sensor";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "boiler probe";
    config["enabled"] = true;
    config["formulaMode"] = "beta";
    config["seriesResistorOhms"] = 9800;
    config["supplyMilliVolts"] = 3300;
    config["nominalResistanceOhms"] = 100000;
    config["betaCoefficient"] = 3950;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject analogInputDep = deps.createNestedObject();
    analogInputDep["role"] = "analog_input";
    analogInputDep["deviceId"] = 42;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-ntc_thermistor_temperature_sensor.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(kNtcThermistorTemperatureSensorTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kNtcThermistorTemperatureSensorConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("boiler probe", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    NtcThermistorTemperatureSensorConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT16(9800, parsed.seriesResistorOhms);
    TEST_ASSERT_EQUAL_UINT16(3300, parsed.supplyMilliVolts);
    TEST_ASSERT_EQUAL_UINT32(100000, parsed.nominalResistanceOhms);
    TEST_ASSERT_EQUAL_UINT16(3950, parsed.betaCoefficient);
}

void test_ntc_thermistor_api_adapter_rejects_missing_config() {
    StaticJsonDocument<128> doc;
    doc["typeName"] = "ntc_thermistor_temperature_sensor";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(
        NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("device config is required", error);
}

void test_ntc_thermistor_api_adapter_rejects_invalid_config() {
    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "boiler probe";
    config["seriesResistorOhms"] = 0; // parseJson runs validate() itself and surfaces its message

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(
        NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("ntc thermistor series resistor is invalid", error);
}

void test_binary_sensor_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "binary_sensor";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "reagent low level";
    config["enabled"] = true;
    config["gpioPin"] = 27;
    config["pullMode"] = "pulldown";
    config["inverted"] = true;
    config["debounceMs"] = 120;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-binary_sensor.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(BinarySensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kBinarySensorDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kBinarySensorDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("reagent low level", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    BinarySensorDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(27, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(GpioInputPullMode::PullDown), parsed.pullMode);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.inverted);
    TEST_ASSERT_EQUAL_UINT16(120, parsed.debounceMs);
}

void test_binary_sensor_api_adapter_serializes_record() {
    FakeBinarySensorDriver driver;
    driver.level = true;
    const BinarySensorDeviceConfigV1 config = makeBinarySensorConfig();
    BinarySensorDevice device(config, driver);
    DeviceRegistryEntry record{};
    record.header.deviceId = 27U;
    record.header.typeId = kBinarySensorDeviceTypeId;
    record.header.configVersion = kBinarySensorDeviceConfigVersion;
    const DeviceConfigBlob blob = [&]() {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        DeviceConfigBlob out{};
        TEST_ASSERT_TRUE(encodeFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, config, buffer, binarySensorDeviceConfigSize(config)));
        TEST_ASSERT_TRUE(out.assign(buffer, binarySensorDeviceConfigSize(config)));
        return out;
    }();
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    device.bindDeviceIdentity(record, blob);
    device.begin(10U);
    for (int i = 0; i < 4 && device.status() != DeviceStatus::Ready; ++i) {
        device.tickFastLoop(10U);
    }
    device.tickFastLoop(11U);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    BinarySensorDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-binary_sensor.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(27U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("binary_sensor", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("float switch", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["active"].as<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["rawLevel"].as<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["hasReading"].as<bool>());
}

void test_binary_sensor_api_adapter_rejects_pull_on_input_only_pin() {
    StaticJsonDocument<256> doc;
    doc["typeName"] = "binary_sensor";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "reagent low level";
    config["gpioPin"] = 36;
    config["pullMode"] = "pullup";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(BinarySensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("device config is invalid", error);
}

void test_binary_sensor_api_adapter_partial_update_preserves_pin_and_debounce() {
    BinarySensorDeviceConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "reagent low level");
    current.gpioPin = 27U;
    current.pullMode = static_cast<uint8_t>(GpioInputPullMode::PullDown);
    current.debounceMs = 120U;
    class FakeDriver final : public IGpioInputDriver {
    public:
        bool configureInput(uint8_t, GpioInputPullMode) override {
            return true;
        }
        bool read(uint8_t, bool& level) override {
            level = false;
            return true;
        }
        void release(uint8_t) override {}
    } driver;
    BinarySensorDevice runtime(current, driver);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["inverted"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-binary_sensor.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(BinarySensorDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_STRING("reagent low level", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    BinarySensorDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.inverted);
    TEST_ASSERT_EQUAL_UINT8(27U, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(GpioInputPullMode::PullDown), parsed.pullMode);
    TEST_ASSERT_EQUAL_UINT16(120U, parsed.debounceMs);
}

void test_dosing_pump_api_adapter_parses_create_request_with_deps() {
    StaticJsonDocument<2048> doc;
    doc["typeName"] = "dosing_pump";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "Calcium";
    config["enabled"] = true;
    config["dosingSpeedMlPerSec"] = 1.25;
    JsonObject container = config.createNestedObject("container");
    container["capacityMl"] = 500;
    container["thresholdPercent"] = 15;
    container["blockAutoWhenEmpty"] = true;
    JsonObject schedule = config.createNestedObject("schedule");
    schedule["mode"] = "daily";
    schedule["everyDays"] = 2;
    schedule["anchorDay"] = 20647;
    JsonArray doses = schedule.createNestedArray("doses");
    JsonObject dose = doses.createNestedObject();
    dose["time"] = "08:30";
    dose["amountMl"] = 12.3;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 7;
    JsonObject sensorDep = deps.createNestedObject();
    sensorDep["role"] = "condition";
    sensorDep["deviceId"] = 8;
    sensorDep["invert"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-dosing_pump.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(DosingPumpDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kDosingPumpDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("Calcium", request.baseConfig.name);
    TEST_ASSERT_EQUAL_UINT8(2U, request.dependencyCount());
    TEST_ASSERT_TRUE(request.dependencyLinks()[0].role == DeviceRole::Switch);
    TEST_ASSERT_EQUAL_UINT32(7U, request.dependencyLinks()[0].deviceId);
    TEST_ASSERT_TRUE(request.dependencyLinks()[1].role == DeviceRole::Condition);
    TEST_ASSERT_TRUE(request.dependencyLinks()[1].invert);

    DosingPumpDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        DosingPumpDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT16(1250U, parsed.speedMilliMlPerSec);
    TEST_ASSERT_EQUAL_UINT16(500U, parsed.containerCapacityMl);
    TEST_ASSERT_EQUAL_UINT8(15U, parsed.thresholdPercent);
    TEST_ASSERT_EQUAL_UINT8(2U, parsed.everyDays);
    TEST_ASSERT_EQUAL_UINT16(20647U, parsed.anchorDay);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.doseCount);
    TEST_ASSERT_EQUAL_UINT16(8U * 60U + 30U, parsed.doses[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(1230U, parsed.doses[0].amountCentiMl);
}

void test_dosing_pump_api_adapter_serializes_record() {
    const DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    const DeviceConfigBlob blob = encodeDosingPumpBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 31U;
    record.header.typeId = kDosingPumpDeviceTypeId;
    record.header.configVersion = kDosingPumpDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    DosingPumpDevice device(config);
    device.bindDeviceIdentity(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    DosingPumpDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-dosing_pump.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(31U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("dosing_pump", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Calcium", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["container"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["skipNext"].is<JsonArrayConst>());
}

void test_dosing_pump_api_adapter_rejects_missing_switch_dep() {
    StaticJsonDocument<1024> doc;
    doc["typeName"] = "dosing_pump";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "Calcium";
    JsonArray deps = config.createNestedArray("deps");
    JsonObject sensorDep = deps.createNestedObject();
    sensorDep["role"] = "condition";
    sensorDep["deviceId"] = 8;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-dosing_pump.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(DosingPumpDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("dosing pump requires a switch dependency", error);
}

void test_dosing_pump_api_adapter_partial_update_preserves_schedule() {
    DosingPumpDeviceConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "Calcium");
    current.speedMilliMlPerSec = 1250U;
    current.containerCapacityMl = 500U;
    current.doses[0] = DosingPumpDoseV1{510U, 1230U};
    current.doseCount = 1U;
    DosingPumpDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["dosingSpeedMlPerSec"] = 1.3;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-dosing_pump.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(DosingPumpDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_STRING("Calcium", request.baseConfig.name);
    TEST_ASSERT_FALSE(request.depsProvided);

    DosingPumpDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        DosingPumpDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT16(1300U, parsed.speedMilliMlPerSec);
    TEST_ASSERT_EQUAL_UINT16(500U, parsed.containerCapacityMl);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.doseCount);
    TEST_ASSERT_EQUAL_UINT16(510U, parsed.doses[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(1230U, parsed.doses[0].amountCentiMl);
}

void test_auto_switch_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "auto_switch";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "auto switch";
    config["enabled"] = true;
    config["pauseDurationSeconds"] = 900;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 10;
    JsonObject conditionDep = deps.createNestedObject();
    conditionDep["role"] = "condition";
    conditionDep["deviceId"] = 11;
    conditionDep["invert"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-auto_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(AutoSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kAutoSwitchDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kAutoSwitchDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("auto switch", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(2U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(10U, request.dependencyLinks()[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(11U, request.dependencyLinks()[1].deviceId);

    AutoSwitchDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        AutoSwitchDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT32(900U, parsed.pauseDurationSeconds);
}

void test_auto_switch_api_adapter_parses_update_request() {
    AutoSwitchDeviceConfigV1 currentConfig = makeAutoSwitchConfig();
    AutoSwitchDevice runtime(currentConfig);

    StaticJsonDocument<512> doc;
    JsonObject config = doc.createNestedObject("config");
    config["pauseDurationSeconds"] = 1200;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 10;
    JsonObject conditionDep = deps.createNestedObject();
    conditionDep["role"] = "condition";
    conditionDep["deviceId"] = 11;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-auto_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(AutoSwitchDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kAutoSwitchDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_TRUE(request.depsProvided);
    TEST_ASSERT_EQUAL_UINT8(2U, request.depCount);

    AutoSwitchDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        AutoSwitchDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT32(1200U, parsed.pauseDurationSeconds);
    TEST_ASSERT_EQUAL_STRING("auto switch", request.baseConfig.name);
}

void test_auto_switch_api_adapter_serializes_record() {
    const AutoSwitchDeviceConfigV1 config = makeAutoSwitchConfig();
    const DeviceConfigBlob blob = encodeAutoSwitchBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 19U;
    record.header.typeId = kAutoSwitchDeviceTypeId;
    record.header.configVersion = kAutoSwitchDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    AutoSwitchDevice device(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    AutoSwitchDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-auto_switch.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(19U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("auto_switch", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("auto switch", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(900U, output["config"]["pauseDurationSeconds"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("auto", output["runtime"]["output"]["mode"].as<const char*>());
}

void test_ntc_thermistor_api_adapter_partial_update_preserves_calibration() {
    NtcThermistorTemperatureSensorConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "boiler probe");
    current.supplyMilliVolts = 3250;
    current.betaCoefficient = 4100;
    current.seriesResistorOhms = 9800;
    NtcThermistorTemperatureSensorDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["pollMs"] = 2000;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-ntc_thermistor_temperature_sensor.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime,
                                                                                                         request, error));
    TEST_ASSERT_EQUAL_STRING("boiler probe", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    NtcThermistorTemperatureSensorConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT32(2000, parsed.pollMs);
    TEST_ASSERT_EQUAL_UINT16(3250, parsed.supplyMilliVolts);
    TEST_ASSERT_EQUAL_UINT16(4100, parsed.betaCoefficient);
    TEST_ASSERT_EQUAL_UINT16(9800, parsed.seriesResistorOhms);
}
