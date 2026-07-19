// Characterization tests pinning the observable REST-adapter behavior (parsed fields, exact
// error strings, partial-update semantics) before the adapters migrate onto the shared
// TypedDeviceApiAdapter base. Any assertion change here means the refactor changed behavior.
#include "devices/analog/ledc/LedcAnalogOutputDevice.h"
#include "devices/dosing/DosingPumpDevice.h"
#include "devices/schedule/ScheduleDevice.h"
#include "devices/sensors/binary/BinarySensorDevice.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/analog_output/LedcAnalogOutputDeviceApiAdapter.h"
#include "integrations/rest/binary_sensor/BinarySensorDeviceApiAdapter.h"
#include "integrations/rest/dosing_pump/DosingPumpDeviceApiAdapter.h"
#include "integrations/rest/ntc_thermistor/NtcThermistorTemperatureSensorDeviceApiAdapter.h"
#include "integrations/rest/schedule/ScheduleDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <unity.h>

using namespace ewfm;

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

void test_schedule_api_adapter_parses_create_request() {
    StaticJsonDocument<1024> doc;
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

void test_ntc_thermistor_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "ntc_thermistor_temperature_sensor";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "boiler probe";
    config["enabled"] = true;
    config["gpioPin"] = 34;
    config["attenuation"] = "11db";
    config["seriesResistorOhms"] = 9800;
    config["nominalResistanceOhms"] = 100000;
    config["betaCoefficient"] = 3950;
    config["adcSamples"] = 16;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kNtcThermistorTemperatureSensorTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kNtcThermistorTemperatureSensorConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("boiler probe", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    NtcThermistorTemperatureSensorConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(34, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT16(9800, parsed.seriesResistorOhms);
    TEST_ASSERT_EQUAL_UINT32(100000, parsed.nominalResistanceOhms);
    TEST_ASSERT_EQUAL_UINT16(3950, parsed.betaCoefficient);
    TEST_ASSERT_EQUAL_UINT8(16, parsed.adcSamples);
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
    config["gpioPin"] = 5; // not an ADC1 pin; parseJson runs validate() itself and surfaces its message

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(
        NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("ntc thermistor gpio pin is invalid", error);
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

void test_dosing_pump_api_adapter_rejects_missing_switch_dep() {
    StaticJsonDocument<1024> doc;
    doc["typeName"] = "dosing_pump";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "Calcium";
    JsonArray deps = config.createNestedArray("deps");
    JsonObject sensorDep = deps.createNestedObject();
    sensorDep["role"] = "condition";
    sensorDep["deviceId"] = 8;

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

void test_ntc_thermistor_api_adapter_partial_update_preserves_calibration() {
    NtcThermistorTemperatureSensorConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "boiler probe");
    current.gpioPin = 35;
    current.betaCoefficient = 4100;
    current.seriesResistorOhms = 9800;
    NtcThermistorTemperatureSensorDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["adcSamples"] = 32;

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
    TEST_ASSERT_EQUAL_UINT8(32, parsed.adcSamples);
    TEST_ASSERT_EQUAL_UINT8(35, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT16(4100, parsed.betaCoefficient);
    TEST_ASSERT_EQUAL_UINT16(9800, parsed.seriesResistorOhms);
}
