#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDevice.h"
#include "integrations/common/DeviceApiAdapter.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeGpioOutputDriver final : public IGpioOutputDriver {
public:
    bool configureOutput(uint8_t pin, bool initialLevel) override {
        (void)initialLevel;
        ++configureCalls;
        (void)pin;
        return configureOk;
    }

    bool write(uint8_t pin, bool level) override {
        levels[pin] = level;
        ++writeCalls;
        return true;
    }

    void release(uint8_t pin) override {
        (void)pin;
        ++releaseCalls;
    }

    bool configureOk{true};
    int configureCalls{0};
    int writeCalls{0};
    int releaseCalls{0};
    bool levels[64]{};
};

class FakeAdcInputDriver final : public IAdcInputDriver {
public:
    bool configurePin(uint8_t pin, AdcAttenuation attenuation) override {
        (void)pin;
        (void)attenuation;
        ++configureCalls;
        return configureOk;
    }

    uint32_t readMilliVolts(uint8_t pin) override {
        (void)pin;
        ++readCalls;
        return milliVolts;
    }

    void release(uint8_t pin) override {
        (void)pin;
        ++releaseCalls;
    }

    bool configureOk{true};
    int configureCalls{0};
    int readCalls{0};
    int releaseCalls{0};
    uint32_t milliVolts{1650};
};

Cd74hc4067HubDeviceConfigV1 makeHubConfig() {
    Cd74hc4067HubDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", "mux hub");
    config.selectPins[0] = 16;
    config.selectPins[1] = 17;
    config.selectPins[2] = 18;
    config.selectPins[3] = 19;
    config.enablePin = kGpioPinUnset;
    config.sigPin = 34;
    config.sigAttenuation = static_cast<uint8_t>(AdcAttenuation::Db11);
    return config;
}

void bringUpHub(Cd74hc4067HubDevice& hub, uint32_t startNow = 10) {
    hub.begin(startNow);
    hub.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(hub.status()));
}

} // namespace

void test_cd74hc4067_hub_config_rejects_invalid_and_overlapping_pins() {
    Cd74hc4067HubDeviceConfigV1 badSelect = makeHubConfig();
    badSelect.selectPins[0] = 250; // not a valid GPIO output pin
    TEST_ASSERT_FALSE(badSelect.validate().ok());

    Cd74hc4067HubDeviceConfigV1 badSig = makeHubConfig();
    badSig.sigPin = 4; // not an ADC1 pin
    TEST_ASSERT_FALSE(badSig.validate().ok());

    Cd74hc4067HubDeviceConfigV1 overlap = makeHubConfig();
    overlap.selectPins[1] = overlap.selectPins[0]; // duplicate pin use
    TEST_ASSERT_FALSE(overlap.validate().ok());

    Cd74hc4067HubDeviceConfigV1 good = makeHubConfig();
    TEST_ASSERT_TRUE_MESSAGE(good.validate().ok(), good.validate().message);
}

void test_cd74hc4067_hub_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kCd74hc4067HubTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::AnalogInputHub));
    TEST_ASSERT_EQUAL_UINT32(0, descriptor->dependencyRequirements.size());

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("cd74hc4067_hub"));
}

void test_cd74hc4067_hub_switches_channel_before_reading_and_sets_select_lines() {
    FakeGpioOutputDriver gpio;
    FakeAdcInputDriver adc;
    adc.milliVolts = 1800;
    Cd74hc4067HubDevice hub(makeHubConfig(), gpio, adc);
    bringUpHub(hub);

    AnalogInputReading reading{};
    const char* status = nullptr;
    // channel 5 == 0b0101: S0=1, S1=0, S2=1, S3=0
    const AnalogInputHubPollResult first = hub.pollChannelReading(5, 1001, 20, reading, status);
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Pending), static_cast<int>(first));
    TEST_ASSERT_TRUE(gpio.levels[16]);
    TEST_ASSERT_FALSE(gpio.levels[17]);
    TEST_ASSERT_TRUE(gpio.levels[18]);
    TEST_ASSERT_FALSE(gpio.levels[19]);
    TEST_ASSERT_EQUAL(0, adc.readCalls);

    const AnalogInputHubPollResult second = hub.pollChannelReading(5, 1001, 21, reading, status);
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Ready), static_cast<int>(second));
    TEST_ASSERT_EQUAL(1, adc.readCalls);
    TEST_ASSERT_TRUE(reading.valid);
    TEST_ASSERT_EQUAL_INT32(1800, reading.milliVolts);
}

void test_cd74hc4067_hub_rejects_concurrent_requesters_and_frees_ownership_after_ready() {
    FakeGpioOutputDriver gpio;
    FakeAdcInputDriver adc;
    Cd74hc4067HubDevice hub(makeHubConfig(), gpio, adc);
    bringUpHub(hub);

    AnalogInputReading reading{};
    const char* status = nullptr;
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Pending),
                      static_cast<int>(hub.pollChannelReading(2, 1001, 20, reading, status)));

    // A different requester on a different channel is refused while channel 2 is owned by 1001.
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Busy),
                      static_cast<int>(hub.pollChannelReading(3, 2002, 21, reading, status)));

    // The owning requester settles the read...
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Ready),
                      static_cast<int>(hub.pollChannelReading(2, 1001, 22, reading, status)));

    // ...and the hub is now free for the other requester.
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Pending),
                      static_cast<int>(hub.pollChannelReading(3, 2002, 23, reading, status)));
}

void test_cd74hc4067_hub_release_channel_request_frees_a_stuck_claim() {
    FakeGpioOutputDriver gpio;
    FakeAdcInputDriver adc;
    Cd74hc4067HubDevice hub(makeHubConfig(), gpio, adc);
    bringUpHub(hub);

    AnalogInputReading reading{};
    const char* status = nullptr;
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Pending),
                      static_cast<int>(hub.pollChannelReading(2, 1001, 20, reading, status)));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Busy),
                      static_cast<int>(hub.pollChannelReading(3, 2002, 21, reading, status)));

    hub.releaseChannelRequest(2, 1001);

    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Pending),
                      static_cast<int>(hub.pollChannelReading(3, 2002, 22, reading, status)));
}

void test_cd74hc4067_hub_faults_when_pins_cannot_be_configured() {
    FakeGpioOutputDriver gpio;
    gpio.configureOk = false;
    FakeAdcInputDriver adc;
    Cd74hc4067HubDevice hub(makeHubConfig(), gpio, adc);
    hub.begin(10);
    hub.tick100ms(11);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(hub.status()));
}
