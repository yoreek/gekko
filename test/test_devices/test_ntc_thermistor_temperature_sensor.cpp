#include "JsonSchemaSmokeValidator.h"
#include "devices/analog/input/port/AnalogPortInputDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/ntc_thermistor/NtcCurve.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorConfig.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/ntc_thermistor/NtcThermistorTemperatureSensorDeviceApiAdapter.h"

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

class FakeAdcInputDriver final : public IAdcInputDriver {
public:
    bool configurePin(uint8_t pin, AdcAttenuation attenuation) override {
        (void)pin;
        (void)attenuation;
        return true;
    }
    uint32_t readMilliVolts(uint8_t pin) override {
        (void)pin;
        return milliVolts;
    }
    void release(uint8_t pin) override {
        (void)pin;
    }

    uint32_t milliVolts{3000};
};

AnalogPortInputDeviceConfigV1 makeAnalogPortConfig() {
    AnalogPortInputDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", "ntc analog port");
    config.gpioPin = 34;
    config.attenuation = static_cast<uint8_t>(AdcAttenuation::Db11);
    config.poll.adcSamples = 1;
    config.poll.reportAlways = 1; // always publish so the sensor sees every driver.milliVolts change promptly
    config.poll.reportDeltaMilliVolts = 1;
    // Below the REST-layer minimum, but the constructor never calls validate() -- keeps the
    // dependency repolling fast enough for tests that change driver.milliVolts mid-run.
    config.poll.pollMs = 1;
    return config;
}

NtcThermistorTemperatureSensorConfigV1 makeSensorConfig() {
    NtcThermistorTemperatureSensorConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", "thermistor");
    config.formulaMode = static_cast<uint8_t>(NtcFormulaMode::Beta);
    config.seriesResistorOhms = 10000;
    config.supplyMilliVolts = 3300;
    config.nominalResistanceOhms = 100000;
    config.nominalTempCentiCelsius = 2500;
    config.betaCoefficient = 3950;
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Celsius);
    config.reportAlways = 0;
    config.reportDeltaCentiCelsius = 10;
    config.pollMs = 1000;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeNtcPayload(const NtcThermistorTemperatureSensorConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic, config, buffer,
                                           ntcThermistorTemperatureSensorConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, ntcThermistorTemperatureSensorConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeAnalogPortPayload(const AnalogPortInputDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic, config, buffer, analogPortInputDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, analogPortInputDeviceConfigSize(config)));
    return payload;
}

void driveAnalogPortReady(AnalogPortInputDevice& port, uint32_t startNow) {
    port.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 20U && !port.reading().valid; ++now) {
        port.tick100ms(now);
    }
    TEST_ASSERT_TRUE(port.reading().valid);
}

void bindAnalogPortIdentity(AnalogPortInputDevice& port, const AnalogPortInputDeviceConfigV1& config, DeviceId id) {
    DeviceRegistryEntry record{};
    record.header.deviceId = id;
    record.header.typeId = AnalogPortInputDevice::descriptor().typeId;
    record.header.configVersion = AnalogPortInputDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.header.payloadLength = static_cast<uint32_t>(encodeAnalogPortPayload(config).size());
    record.status = DeviceStatus::Ready;
    port.bindDeviceIdentity(record, encodeAnalogPortPayload(config));
}

void bindNtcDependency(NtcThermistorTemperatureSensorDevice& sensor, DeviceId sensorId, DeviceId analogInputId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = sensorId;
    record.deps[0] = {DeviceRole::AnalogInput, analogInputId, false};
    record.depCount = 1;
    sensor.bindDeviceIdentity(record, encodeNtcPayload(sensor.config()));
}

// Drives both the analog input dependency and the sensor together until a reading settles (or
// the tick budget is exhausted, in which case the caller's own assertion reports the failure).
void driveSensorUntilReading(NtcThermistorTemperatureSensorDevice& sensor, AnalogPortInputDevice& port, uint32_t startNow = 10) {
    sensor.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 20U && !sensor.reading().valid; ++now) {
        port.tick100ms(now);
        sensor.tick1s(now);
    }
}

} // namespace

void test_ntc_thermistor_config_codec_json_and_validation() {
    NtcThermistorTemperatureSensorConfigV1 config = makeSensorConfig();
    config.formulaMode = static_cast<uint8_t>(NtcFormulaMode::SteinhartHart);
    config.steinhartA = 0.001F;
    config.steinhartB = 0.0002F;
    config.steinhartC = 0.00000008F;
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    const BoundedBlob<kMaxDeviceConfigBytes> blob = encodeNtcPayload(config);

    NtcThermistorTemperatureSensorConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(blob.data()), blob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NtcFormulaMode::SteinhartHart), decoded.formulaMode);
    TEST_ASSERT_EQUAL_UINT16(10000, decoded.seriesResistorOhms);
    TEST_ASSERT_EQUAL_UINT16(3300, decoded.supplyMilliVolts);
    TEST_ASSERT_EQUAL_FLOAT(0.001F, decoded.steinhartA);

    StaticJsonDocument<1024> doc;
    JsonObject json = doc.to<JsonObject>();
    writeNtcThermistorTemperatureSensorConfigJson(config, json);
    TEST_ASSERT_EQUAL_STRING("steinhart_hart", json["formulaMode"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("fahrenheit", json["unit"].as<const char*>());
    TEST_ASSERT_EQUAL_FLOAT(1.0F, json["smoothingWeight"].as<float>());

    NtcThermistorTemperatureSensorConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(parseNtcThermistorTemperatureSensorConfigJson(json, parsed, error), error);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NtcFormulaMode::SteinhartHart), parsed.formulaMode);
    TEST_ASSERT_EQUAL_FLOAT(0.001F, parsed.steinhartA);

    json["formulaMode"] = "not_a_real_mode";
    TEST_ASSERT_FALSE(parseNtcThermistorTemperatureSensorConfigJson(json, parsed, error));
    TEST_ASSERT_NOT_NULL(error);
}

void test_ntc_thermistor_config_rejects_invalid_fields() {
    NtcThermistorTemperatureSensorConfigV1 badResistor = makeSensorConfig();
    badResistor.seriesResistorOhms = 0;
    TEST_ASSERT_FALSE(badResistor.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badSupply = makeSensorConfig();
    badSupply.supplyMilliVolts = 0;
    TEST_ASSERT_FALSE(badSupply.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badNominal = makeSensorConfig();
    badNominal.nominalResistanceOhms = 0;
    TEST_ASSERT_FALSE(badNominal.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badBeta = makeSensorConfig();
    badBeta.betaCoefficient = 0;
    TEST_ASSERT_FALSE(badBeta.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badFormula = makeSensorConfig();
    badFormula.formulaMode = 200;
    TEST_ASSERT_FALSE(badFormula.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badSteinhart = makeSensorConfig();
    badSteinhart.formulaMode = static_cast<uint8_t>(NtcFormulaMode::SteinhartHart);
    badSteinhart.steinhartA = 0.0F;
    badSteinhart.steinhartB = 0.0F;
    badSteinhart.steinhartC = 0.0F;
    TEST_ASSERT_FALSE(badSteinhart.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 goodSteinhart = makeSensorConfig();
    goodSteinhart.formulaMode = static_cast<uint8_t>(NtcFormulaMode::SteinhartHart);
    goodSteinhart.steinhartA = 0.001F;
    TEST_ASSERT_TRUE_MESSAGE(goodSteinhart.validate().ok(), goodSteinhart.validate().message);

    NtcThermistorTemperatureSensorConfigV1 badFilter = makeSensorConfig();
    badFilter.filter.smoothingWeight = 0.0F;
    TEST_ASSERT_FALSE(badFilter.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 good = makeSensorConfig();
    TEST_ASSERT_TRUE_MESSAGE(good.validate().ok(), good.validate().message);
}

void test_ntc_thermistor_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kNtcThermistorTemperatureSensorTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("NtcThermistorTemperatureSensorDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->ticks1s);
    TEST_ASSERT_EQUAL_UINT32(1, descriptor->dependencyRequirements.size());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceRole::AnalogInput), static_cast<int>(descriptor->dependencyRequirements[0].role));
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::TemperatureSensor));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(kNtcThermistorTemperatureSensorTypeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("ntc_thermistor_temperature_sensor"));
}

void test_ntc_thermistor_beta_curve_matches_nominal_point_and_direction() {
    // Rntc == nominalResistanceOhms when Vout=3000mV (10000 * 3000 / (3300-3000) = 100000).
    TEST_ASSERT_INT32_WITHIN(5, 25000, ntcBetaMilliCelsius(100000.0, 100000, 2500, 3950));
    // Lower Vout -> lower Rntc -> hotter than nominal.
    double hotterR = 0.0;
    TEST_ASSERT_TRUE(ntcDividerResistanceOhms(2000, 10000, 3300, hotterR));
    const int32_t hotter = ntcBetaMilliCelsius(hotterR, 100000, 2500, 3950);
    // Higher Vout -> higher Rntc -> cooler than nominal.
    double coolerR = 0.0;
    TEST_ASSERT_TRUE(ntcDividerResistanceOhms(3200, 10000, 3300, coolerR));
    const int32_t cooler = ntcBetaMilliCelsius(coolerR, 100000, 2500, 3950);
    TEST_ASSERT_TRUE(hotter > 25000);
    TEST_ASSERT_TRUE(cooler < 25000);
}

void test_ntc_thermistor_steinhart_hart_curve_matches_classic_10k_coefficients() {
    // Widely published Steinhart-Hart coefficients for a generic 10k/25C NTC probe.
    constexpr float kA = 1.129148e-3F;
    constexpr float kB = 2.34125e-4F;
    constexpr float kC = 8.76741e-8F;
    const int32_t atNominal = ntcSteinhartHartMilliCelsius(10000.0, kA, kB, kC);
    TEST_ASSERT_INT32_WITHIN(1000, 25000, atNominal);

    const int32_t hotter = ntcSteinhartHartMilliCelsius(3000.0, kA, kB, kC);
    const int32_t cooler = ntcSteinhartHartMilliCelsius(30000.0, kA, kB, kC);
    TEST_ASSERT_TRUE(hotter > atNominal);
    TEST_ASSERT_TRUE(cooler < atNominal);
}

void test_ntc_thermistor_curve_helpers_reject_invalid_inputs() {
    double resistance = 0.0;
    TEST_ASSERT_FALSE(ntcDividerResistanceOhms(0, 10000, 3300, resistance));
    TEST_ASSERT_FALSE(ntcDividerResistanceOhms(3300, 10000, 3300, resistance));
    TEST_ASSERT_TRUE(ntcDividerResistanceOhms(1650, 10000, 3300, resistance));

    TEST_ASSERT_EQUAL(INT32_MIN, ntcBetaMilliCelsius(0.0, 100000, 2500, 3950));
    TEST_ASSERT_EQUAL(INT32_MIN, ntcBetaMilliCelsius(100000.0, 0, 2500, 3950));
    TEST_ASSERT_EQUAL(INT32_MIN, ntcSteinhartHartMilliCelsius(0.0, 1.0F, 1.0F, 1.0F));
}

void test_ntc_thermistor_runtime_reads_through_analog_input_dependency() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 3000; // Rntc == nominalResistanceOhms at this divider voltage
    const AnalogPortInputDeviceConfigV1 portConfig = makeAnalogPortConfig();
    AnalogPortInputDevice port(portConfig, driver);
    bindAnalogPortIdentity(port, portConfig, 42);
    driveAnalogPortReady(port, 5);

    NtcThermistorTemperatureSensorDevice sensor(makeSensorConfig());
    bindNtcDependency(sensor, 6001, port.deviceId());
    sensor.setDependencyRuntime(DeviceRole::AnalogInput, &port);

    driveSensorUntilReading(sensor, port);
    TEST_ASSERT_TRUE(sensor.reading().valid);
    TEST_ASSERT_INT32_WITHIN(5, 25000, sensor.reading().milliCelsius);
    TEST_ASSERT_EQUAL_STRING("ok", sensor.outputStatus());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));

    StaticJsonDocument<2048> response;
    JsonObject output = response.to<JsonObject>();
    NtcThermistorTemperatureSensorDeviceApiAdapter::instance().writeDeviceJson(sensor, sensor.status(), output);
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-ntc_thermistor_temperature_sensor.response.schema.json",
                            response.as<JsonVariantConst>());
}

void test_ntc_thermistor_runtime_reports_out_of_range_at_divider_extreme() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 3300; // equal to supply voltage - divider math is undefined here
    AnalogPortInputDevice port(makeAnalogPortConfig(), driver);
    driveAnalogPortReady(port, 5);

    NtcThermistorTemperatureSensorDevice sensor(makeSensorConfig());
    bindNtcDependency(sensor, 6002, port.deviceId());
    sensor.setDependencyRuntime(DeviceRole::AnalogInput, &port);

    sensor.begin(10);
    for (uint32_t now = 11; now < 30; ++now) {
        port.tick100ms(now);
        sensor.tick1s(now);
    }

    TEST_ASSERT_FALSE(sensor.reading().valid);
    TEST_ASSERT_EQUAL_STRING("out_of_range", sensor.outputStatus());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
}

void test_ntc_thermistor_runtime_reports_dependency_blocked_without_analog_input() {
    NtcThermistorTemperatureSensorDevice sensor(makeSensorConfig());
    bindNtcDependency(sensor, 6003, 9999);
    // No setDependencyRuntime call -- the AnalogInput link is never resolved.

    sensor.begin(10);
    sensor.tick1s(11);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(sensor.status()));
    TEST_ASSERT_FALSE(sensor.reading().valid);
}

void test_ntc_thermistor_runtime_disable_reconfigure_and_delete_lifecycle() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 3000;
    AnalogPortInputDevice port(makeAnalogPortConfig(), driver);
    driveAnalogPortReady(port, 5);

    NtcThermistorTemperatureSensorDevice sensor(makeSensorConfig());
    bindNtcDependency(sensor, 6004, port.deviceId());
    sensor.setDependencyRuntime(DeviceRole::AnalogInput, &port);

    driveSensorUntilReading(sensor, port);
    TEST_ASSERT_TRUE(sensor.reading().valid);

    sensor.requestDisable();
    sensor.tick1s(100);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(sensor.status()));
    TEST_ASSERT_FALSE(sensor.reading().valid);

    sensor.requestReconfigure();
    for (uint32_t now = 101; now < 130 && !sensor.reading().valid; ++now) {
        port.tick100ms(now);
        sensor.tick1s(now);
    }
    TEST_ASSERT_TRUE(sensor.reading().valid);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));

    sensor.requestDelete();
    sensor.tick1s(200);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Deleting), static_cast<int>(sensor.status()));
}

void test_ntc_thermistor_runtime_smoothing_filter_dampens_step_change_and_no_op_passes_through() {
    NtcThermistorTemperatureSensorConfigV1 config = makeSensorConfig();
    config.filter.smoothingWeight = 0.5F;
    config.pollMs = 1; // bypasses the REST-layer minimum so the second poll happens on the next tick

    FakeAdcInputDriver driver;
    driver.milliVolts = 3000;
    AnalogPortInputDevice port(makeAnalogPortConfig(), driver);
    driveAnalogPortReady(port, 5);

    NtcThermistorTemperatureSensorDevice smoothed(config);
    bindNtcDependency(smoothed, 6005, port.deviceId());
    smoothed.setDependencyRuntime(DeviceRole::AnalogInput, &port);
    driveSensorUntilReading(smoothed, port);
    TEST_ASSERT_TRUE(smoothed.reading().valid);
    const int32_t firstReading = smoothed.reading().milliCelsius;

    driver.milliVolts = 2000; // step change to a much hotter reading
    const uint32_t previousMeasuredAt = smoothed.reading().measuredAtMs;
    for (uint32_t now = previousMeasuredAt + 1U; now < previousMeasuredAt + 20U && smoothed.reading().measuredAtMs == previousMeasuredAt;
         ++now) {
        port.tick100ms(now);
        smoothed.tick1s(now);
    }
    TEST_ASSERT_TRUE(smoothed.reading().valid);
    // With smoothing enabled the immediate jump toward the new (much hotter) raw value should be
    // damped, landing somewhere between the old and the fully-converged raw reading.
    TEST_ASSERT_TRUE(smoothed.reading().milliCelsius > firstReading);

    FakeAdcInputDriver rawDriver;
    rawDriver.milliVolts = 2000;
    AnalogPortInputDevice rawPort(makeAnalogPortConfig(), rawDriver);
    driveAnalogPortReady(rawPort, 5);
    NtcThermistorTemperatureSensorConfigV1 rawConfig = makeSensorConfig();
    NtcThermistorTemperatureSensorDevice raw(rawConfig);
    bindNtcDependency(raw, 6006, rawPort.deviceId());
    raw.setDependencyRuntime(DeviceRole::AnalogInput, &rawPort);
    driveSensorUntilReading(raw, rawPort);
    TEST_ASSERT_TRUE(raw.reading().valid);
    TEST_ASSERT_TRUE_MESSAGE(smoothed.reading().milliCelsius < raw.reading().milliCelsius,
                             "smoothed reading should lag behind the fully-settled raw reading after a single step");
}
