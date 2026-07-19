#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorConfig.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "integrations/common/DeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeAdcInputDriver final : public IAdcInputDriver {
public:
    bool configurePin(uint8_t pin, AdcAttenuation attenuation) override {
        lastPin = pin;
        lastAttenuation = attenuation;
        ++configureCalls;
        return configureOk;
    }

    uint32_t readMilliVolts(uint8_t pin) override {
        (void)pin;
        return milliVolts;
    }

    void release(uint8_t pin) override {
        (void)pin;
        released = true;
    }

    bool configureOk{true};
    uint8_t lastPin{0};
    AdcAttenuation lastAttenuation{AdcAttenuation::Db11};
    int configureCalls{0};
    uint32_t milliVolts{3000};
    bool released{false};
};

NtcThermistorTemperatureSensorConfigV1 makeSensorConfig() {
    NtcThermistorTemperatureSensorConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", "thermistor");
    config.gpioPin = 34;
    config.attenuation = static_cast<uint8_t>(AdcAttenuation::Db11);
    config.seriesResistorOhms = 10000;
    config.nominalResistanceOhms = 100000;
    config.nominalTempCentiCelsius = 2500;
    config.betaCoefficient = 3950;
    config.adcSamples = 4;
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

void driveSensorUntilReading(NtcThermistorTemperatureSensorDevice& sensor, uint32_t startNow = 10) {
    sensor.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 20U && !sensor.reading().valid; ++now) {
        sensor.tick1s(now);
    }
}

} // namespace

void test_ntc_thermistor_config_codec_json_and_validation() {
    NtcThermistorTemperatureSensorConfigV1 config = makeSensorConfig();
    config.attenuation = static_cast<uint8_t>(AdcAttenuation::Db6);
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    const BoundedBlob<kMaxDeviceConfigBytes> blob = encodeNtcPayload(config);

    NtcThermistorTemperatureSensorConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(blob.data()), blob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(34, decoded.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db6), decoded.attenuation);
    TEST_ASSERT_EQUAL_UINT16(10000, decoded.seriesResistorOhms);
    TEST_ASSERT_EQUAL_UINT32(100000, decoded.nominalResistanceOhms);

    StaticJsonDocument<1024> doc;
    JsonObject json = doc.to<JsonObject>();
    writeNtcThermistorTemperatureSensorConfigJson(config, json);
    TEST_ASSERT_EQUAL_STRING("6db", json["attenuation"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("fahrenheit", json["unit"].as<const char*>());
    TEST_ASSERT_EQUAL_FLOAT(1.0F, json["smoothingWeight"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(1.0F, json["calibrationFactor"].as<float>());

    NtcThermistorTemperatureSensorConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(parseNtcThermistorTemperatureSensorConfigJson(json, parsed, error), error);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db6), parsed.attenuation);

    json["gpioPin"] = 4; // not an ADC1 pin
    TEST_ASSERT_FALSE(parseNtcThermistorTemperatureSensorConfigJson(json, parsed, error));
    TEST_ASSERT_NOT_NULL(error);
}

void test_ntc_thermistor_config_rejects_invalid_fields() {
    NtcThermistorTemperatureSensorConfigV1 badPin = makeSensorConfig();
    badPin.gpioPin = 2; // ADC2 pin, excluded because it conflicts with WiFi
    TEST_ASSERT_FALSE(badPin.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badResistor = makeSensorConfig();
    badResistor.seriesResistorOhms = 0;
    TEST_ASSERT_FALSE(badResistor.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badNominal = makeSensorConfig();
    badNominal.nominalResistanceOhms = 0;
    TEST_ASSERT_FALSE(badNominal.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badBeta = makeSensorConfig();
    badBeta.betaCoefficient = 0;
    TEST_ASSERT_FALSE(badBeta.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badSamplesLow = makeSensorConfig();
    badSamplesLow.adcSamples = 0;
    TEST_ASSERT_FALSE(badSamplesLow.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badSamplesHigh = makeSensorConfig();
    badSamplesHigh.adcSamples = 200;
    TEST_ASSERT_FALSE(badSamplesHigh.validate().ok());

    NtcThermistorTemperatureSensorConfigV1 badAttenuation = makeSensorConfig();
    badAttenuation.attenuation = 200;
    TEST_ASSERT_FALSE(badAttenuation.validate().ok());

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
    TEST_ASSERT_EQUAL_UINT32(0, descriptor->dependencyRequirements.size());
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::TemperatureSensor));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(kNtcThermistorTemperatureSensorTypeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("ntc_thermistor_temperature_sensor"));
}

void test_ntc_thermistor_beta_equation_matches_nominal_point_and_direction() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 3000; // Rntc == nominalResistanceOhms at this divider voltage
    NtcThermistorTemperatureSensorDevice atNominal(makeSensorConfig(), driver);
    driveSensorUntilReading(atNominal);
    TEST_ASSERT_TRUE(atNominal.reading().valid);
    TEST_ASSERT_INT32_WITHIN(5, 25000, atNominal.reading().milliCelsius);

    FakeAdcInputDriver hotterDriver;
    hotterDriver.milliVolts = 2000; // lower Vout -> lower Rntc -> hotter than nominal
    NtcThermistorTemperatureSensorDevice hotter(makeSensorConfig(), hotterDriver);
    driveSensorUntilReading(hotter);
    TEST_ASSERT_TRUE(hotter.reading().valid);
    TEST_ASSERT_TRUE(hotter.reading().milliCelsius > atNominal.reading().milliCelsius);

    FakeAdcInputDriver coolerDriver;
    coolerDriver.milliVolts = 3200; // higher Vout -> higher Rntc -> cooler than nominal
    NtcThermistorTemperatureSensorDevice cooler(makeSensorConfig(), coolerDriver);
    driveSensorUntilReading(cooler);
    TEST_ASSERT_TRUE(cooler.reading().valid);
    TEST_ASSERT_TRUE(cooler.reading().milliCelsius < atNominal.reading().milliCelsius);
}

void test_ntc_thermistor_runtime_configures_pin_and_reports_out_of_range() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 3300; // equal to supply voltage - divider math is undefined here
    NtcThermistorTemperatureSensorDevice sensor(makeSensorConfig(), driver);
    sensor.begin(10);
    for (uint32_t now = 11; now < 30; ++now) {
        sensor.tick1s(now);
    }

    TEST_ASSERT_EQUAL_UINT8(34, driver.lastPin);
    TEST_ASSERT_TRUE(driver.configureCalls > 0);
    TEST_ASSERT_FALSE(sensor.reading().valid);
    TEST_ASSERT_EQUAL_STRING("out_of_range", sensor.outputStatus());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
}

void test_ntc_thermistor_runtime_faults_when_pin_cannot_be_configured() {
    FakeAdcInputDriver driver;
    driver.configureOk = false;
    NtcThermistorTemperatureSensorDevice sensor(makeSensorConfig(), driver);
    sensor.begin(10);
    sensor.tick1s(11);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(sensor.status()));
    TEST_ASSERT_FALSE(sensor.reading().valid);
    TEST_ASSERT_EQUAL_STRING("not_found", sensor.outputStatus());
}

void test_ntc_thermistor_runtime_disable_reconfigure_and_delete_lifecycle() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 3000;
    NtcThermistorTemperatureSensorDevice sensor(makeSensorConfig(), driver);
    driveSensorUntilReading(sensor);
    TEST_ASSERT_TRUE(sensor.reading().valid);

    sensor.requestDisable();
    sensor.tick1s(100);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(sensor.status()));
    TEST_ASSERT_FALSE(sensor.reading().valid);

    sensor.requestReconfigure();
    for (uint32_t now = 101; now < 130 && !sensor.reading().valid; ++now) {
        sensor.tick1s(now);
    }
    TEST_ASSERT_TRUE(sensor.reading().valid);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));

    sensor.requestDelete();
    sensor.tick1s(200);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Deleting), static_cast<int>(sensor.status()));
    TEST_ASSERT_TRUE(driver.released);
}

void test_ntc_thermistor_runtime_smoothing_filter_dampens_step_change_and_no_op_passes_through() {
    NtcThermistorTemperatureSensorConfigV1 config = makeSensorConfig();
    config.filter.smoothingWeight = 0.5F;
    config.pollMs = 1; // bypasses the REST-layer minimum so the second poll happens on the next tick
    FakeAdcInputDriver driver;
    driver.milliVolts = 3000;
    NtcThermistorTemperatureSensorDevice smoothed(config, driver);
    driveSensorUntilReading(smoothed);
    TEST_ASSERT_TRUE(smoothed.reading().valid);
    const int32_t firstReading = smoothed.reading().milliCelsius;

    driver.milliVolts = 2000; // step change to a much hotter reading
    const uint32_t previousMeasuredAt = smoothed.reading().measuredAtMs;
    for (uint32_t now = previousMeasuredAt + 1U; now < previousMeasuredAt + 20U && smoothed.reading().measuredAtMs == previousMeasuredAt;
         ++now) {
        smoothed.tick1s(now);
    }
    TEST_ASSERT_TRUE(smoothed.reading().valid);
    // With smoothing enabled the immediate jump toward the new (much hotter) raw value should be
    // damped, landing somewhere between the old and the fully-converged raw reading.
    TEST_ASSERT_TRUE(smoothed.reading().milliCelsius > firstReading);

    FakeAdcInputDriver rawDriver;
    rawDriver.milliVolts = 2000;
    NtcThermistorTemperatureSensorConfigV1 rawConfig = makeSensorConfig();
    NtcThermistorTemperatureSensorDevice raw(rawConfig, rawDriver);
    driveSensorUntilReading(raw);
    TEST_ASSERT_TRUE(raw.reading().valid);
    TEST_ASSERT_TRUE_MESSAGE(smoothed.reading().milliCelsius < raw.reading().milliCelsius,
                             "smoothed reading should lag behind the fully-settled raw reading after a single step");
}
