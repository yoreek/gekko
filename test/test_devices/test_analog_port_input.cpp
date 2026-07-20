#include "devices/analog/input/port/AnalogPortInputDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
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
    uint32_t milliVolts{1650};
    bool released{false};
};

AnalogPortInputDeviceConfigV1 makePortConfig() {
    AnalogPortInputDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", "analog port");
    config.gpioPin = 34;
    config.attenuation = static_cast<uint8_t>(AdcAttenuation::Db11);
    config.poll.adcSamples = 4;
    config.poll.reportAlways = 0;
    config.poll.reportDeltaMilliVolts = 10;
    config.poll.pollMs = 100;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodePortPayload(const AnalogPortInputDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic, config, buffer, analogPortInputDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, analogPortInputDeviceConfigSize(config)));
    return payload;
}

void driveUntilReading(AnalogPortInputDevice& device, uint32_t startNow = 10) {
    device.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 40U && !device.reading().valid; ++now) {
        device.tick100ms(now);
    }
}

} // namespace

void test_analog_port_input_config_codec_json_and_validation() {
    AnalogPortInputDeviceConfigV1 config = makePortConfig();
    config.attenuation = static_cast<uint8_t>(AdcAttenuation::Db6);
    const BoundedBlob<kMaxDeviceConfigBytes> blob = encodePortPayload(config);

    AnalogPortInputDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(blob.data()),
                                                    blob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(34, decoded.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db6), decoded.attenuation);
    TEST_ASSERT_EQUAL_UINT8(4, decoded.poll.adcSamples);

    StaticJsonDocument<512> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_STRING("6db", json["attenuation"].as<const char*>());

    AnalogPortInputDeviceConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(parsed.parseJson(json, error), error);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db6), parsed.attenuation);

    json["gpioPin"] = 4; // not an ADC1 pin
    TEST_ASSERT_FALSE(parsed.parseJson(json, error));
    TEST_ASSERT_NOT_NULL(error);
}

void test_analog_port_input_config_rejects_invalid_fields() {
    AnalogPortInputDeviceConfigV1 badPin = makePortConfig();
    badPin.gpioPin = 2; // ADC2 pin, excluded because it conflicts with WiFi
    TEST_ASSERT_FALSE(badPin.validate().ok());

    AnalogPortInputDeviceConfigV1 badSamplesLow = makePortConfig();
    badSamplesLow.poll.adcSamples = 0;
    TEST_ASSERT_FALSE(badSamplesLow.validate().ok());

    AnalogPortInputDeviceConfigV1 badSamplesHigh = makePortConfig();
    badSamplesHigh.poll.adcSamples = 200;
    TEST_ASSERT_FALSE(badSamplesHigh.validate().ok());

    AnalogPortInputDeviceConfigV1 badAttenuation = makePortConfig();
    badAttenuation.attenuation = 200;
    TEST_ASSERT_FALSE(badAttenuation.validate().ok());

    AnalogPortInputDeviceConfigV1 badPoll = makePortConfig();
    badPoll.poll.pollMs = 1;
    TEST_ASSERT_FALSE(badPoll.validate().ok());

    AnalogPortInputDeviceConfigV1 good = makePortConfig();
    TEST_ASSERT_TRUE_MESSAGE(good.validate().ok(), good.validate().message);
}

void test_analog_port_input_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kAnalogPortInputTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("AnalogPortInputDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->ticks100ms);
    TEST_ASSERT_EQUAL_UINT32(0, descriptor->dependencyRequirements.size());
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::AnalogInput));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(kAnalogPortInputTypeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("analog_port_input"));
}

void test_analog_port_input_runtime_averages_samples_and_reports_reading() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 1650; // roughly mid-scale for the 0..3300mV nominal reference
    AnalogPortInputDevice device(makePortConfig(), driver);
    driveUntilReading(device);

    TEST_ASSERT_EQUAL_UINT8(34, driver.lastPin);
    TEST_ASSERT_TRUE(driver.configureCalls > 0);
    TEST_ASSERT_TRUE(device.reading().valid);
    TEST_ASSERT_EQUAL_INT32(1650, device.reading().milliVolts);
    TEST_ASSERT_INT_WITHIN(2, 2047, device.reading().rawCode);
    TEST_ASSERT_EQUAL_STRING("ok", device.outputStatus());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
}

void test_analog_port_input_runtime_faults_when_pin_cannot_be_configured() {
    FakeAdcInputDriver driver;
    driver.configureOk = false;
    AnalogPortInputDevice device(makePortConfig(), driver);
    device.begin(10);
    device.tick100ms(11);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(device.status()));
    TEST_ASSERT_FALSE(device.reading().valid);
    TEST_ASSERT_EQUAL_STRING("not_found", device.latestAnalogInputStatus());
}

void test_analog_port_input_runtime_disable_reconfigure_and_delete_lifecycle() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 1650;
    AnalogPortInputDevice device(makePortConfig(), driver);
    driveUntilReading(device);
    TEST_ASSERT_TRUE(device.reading().valid);

    device.requestDisable();
    device.tick100ms(100);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_FALSE(device.reading().valid);

    device.requestReconfigure();
    for (uint32_t now = 101; now < 140 && !device.reading().valid; ++now) {
        device.tick100ms(now);
    }
    TEST_ASSERT_TRUE(device.reading().valid);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));

    device.requestDelete();
    device.tick100ms(200);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Deleting), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(driver.released);
}
