#include "devices/analog/input/ads1115/Ads1115HubDevice.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "integrations/common/DeviceApiAdapter.h"

#include <cstdio>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

// Simulates the ADS1115 register protocol over the II2cBusDriver primitives: a pointer-register
// write selects config (0x01, 3 bytes: pointer+MSB+LSB) or conversion (0x00, pointer only followed
// by a 2-byte read).
class FakeAds1115I2cDriver final : public II2cBusDriver {
public:
    bool begin(uint8_t, uint8_t, uint32_t, bool) override {
        return true;
    }
    bool end() override {
        return true;
    }
    bool setClock(uint32_t) override {
        return true;
    }
    uint32_t getClock() const override {
        return 400000U;
    }

    void beginTransmission(uint8_t address) override {
        lastAddress = address;
        pendingWrite.clear();
    }

    uint8_t endTransmission(bool) override {
        if (!present) {
            return 2U;
        }
        if (pendingWrite.size() == 3U) {
            lastConfigRegister = static_cast<uint16_t>((static_cast<uint16_t>(pendingWrite[1]) << 8) | pendingWrite[2]);
            ++configWriteCount;
        }
        return 0U;
    }

    size_t requestFrom(uint8_t, size_t size, bool) override {
        readBuffer.clear();
        readPos = 0;
        if (!present) {
            return 0U;
        }
        readBuffer.push_back(static_cast<uint8_t>(static_cast<uint16_t>(conversionRaw) >> 8));
        readBuffer.push_back(static_cast<uint8_t>(static_cast<uint16_t>(conversionRaw) & 0xFFU));
        ++conversionReadCount;
        return size <= readBuffer.size() ? size : readBuffer.size();
    }

    size_t write(uint8_t data) override {
        pendingWrite.push_back(data);
        return 1U;
    }

    size_t write(const uint8_t* data, size_t quantity) override {
        pendingWrite.insert(pendingWrite.end(), data, data + quantity);
        return quantity;
    }

    int available() override {
        return static_cast<int>(readBuffer.size() - readPos);
    }

    int read() override {
        if (readPos >= readBuffer.size()) {
            return -1;
        }
        return readBuffer[readPos++];
    }

    void flush() override {}

    bool present{true};
    int16_t conversionRaw{8192}; // 8192/32768 * FSR -> quarter-scale
    uint8_t lastAddress{0};
    uint16_t lastConfigRegister{0};
    int configWriteCount{0};
    int conversionReadCount{0};

private:
    std::vector<uint8_t> pendingWrite;
    std::vector<uint8_t> readBuffer;
    size_t readPos{0};
};

I2cBusDeviceConfigV1 makeBusConfig() {
    I2cBusDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "i2c-bus");
    config.sdaPin = 18;
    config.sclPin = 19;
    config.internalPullup = 1U;
    config.frequencyHz = 400000U;
    return config;
}

Ads1115HubDeviceConfigV1 makeHubConfig() {
    Ads1115HubDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "ads1115");
    config.i2cAddress = kAds1115DefaultI2cAddress;
    config.gain = static_cast<uint8_t>(Ads1115Gain::Fsr2048);
    config.dataRateSps = static_cast<uint8_t>(Ads1115DataRate::Sps860); // fastest -> short conversion wait in tests
    return config;
}

void driveBusReady(I2cBusDevice& bus, uint32_t startNow = 1U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
}

void bindHubDependency(Ads1115HubDevice& hub, DeviceId hubId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = hubId;
    record.deps[0] = {DeviceRole::I2CBus, busId, false};
    record.depCount = 1;
    hub.bindDeviceIdentity(record, BoundedBlob<kMaxDeviceConfigBytes>{});
}

void driveHubReady(Ads1115HubDevice& hub, uint32_t startNow) {
    hub.begin(startNow);
    hub.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(hub.status()));
}

} // namespace

void test_ads1115_hub_config_rejects_invalid_fields() {
    Ads1115HubDeviceConfigV1 badAddress = makeHubConfig();
    badAddress.i2cAddress = 0x02; // reserved address range
    TEST_ASSERT_FALSE(badAddress.validate().ok());

    Ads1115HubDeviceConfigV1 badGain = makeHubConfig();
    badGain.gain = 200;
    TEST_ASSERT_FALSE(badGain.validate().ok());

    Ads1115HubDeviceConfigV1 badRate = makeHubConfig();
    badRate.dataRateSps = 200;
    TEST_ASSERT_FALSE(badRate.validate().ok());

    Ads1115HubDeviceConfigV1 good = makeHubConfig();
    TEST_ASSERT_TRUE_MESSAGE(good.validate().ok(), good.validate().message);
}

void test_ads1115_hub_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kAds1115HubTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::AnalogInputHub));
    TEST_ASSERT_EQUAL_UINT32(1, descriptor->dependencyRequirements.size());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceRole::I2CBus), static_cast<int>(descriptor->dependencyRequirements[0].role));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("ads1115_hub"));
}

void test_ads1115_hub_single_shot_conversion_completes_after_conversion_time() {
    FakeAds1115I2cDriver i2c;
    I2cBusDevice bus(makeBusConfig(), i2c);
    driveBusReady(bus);

    Ads1115HubDevice hub(makeHubConfig());
    bindHubDependency(hub, 8001, bus.deviceId());
    hub.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveHubReady(hub, 10);

    i2c.conversionRaw = 16384; // half-scale -> FSR/2 = 1024 mV at +-2.048V gain
    AnalogInputReading reading{};
    const char* status = nullptr;
    const AnalogInputHubPollResult first = hub.pollChannelReading(1, 9001, 20, reading, status);
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Pending), static_cast<int>(first));
    TEST_ASSERT_EQUAL(1, i2c.configWriteCount);
    // MUX field (bits 14-12) selects AIN1 vs GND: 0b101 = 5
    TEST_ASSERT_EQUAL_UINT16(5, (i2c.lastConfigRegister >> 12) & 0x7U);

    // Still within the conversion window -> stays Pending without re-touching the bus.
    const AnalogInputHubPollResult second = hub.pollChannelReading(1, 9001, 21, reading, status);
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Pending), static_cast<int>(second));
    TEST_ASSERT_EQUAL(0, i2c.conversionReadCount);

    // Sps860 => ~2ms conversion time; by now+5 the deadline has certainly passed.
    const AnalogInputHubPollResult third = hub.pollChannelReading(1, 9001, 25, reading, status);
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Ready), static_cast<int>(third));
    TEST_ASSERT_EQUAL(1, i2c.conversionReadCount);
    TEST_ASSERT_TRUE(reading.valid);
    TEST_ASSERT_EQUAL_INT32(1024, reading.milliVolts);
    TEST_ASSERT_EQUAL_STRING("ok", status);
}

void test_ads1115_hub_rejects_concurrent_requesters_while_conversion_in_flight() {
    FakeAds1115I2cDriver i2c;
    I2cBusDevice bus(makeBusConfig(), i2c);
    driveBusReady(bus);

    Ads1115HubDevice hub(makeHubConfig());
    bindHubDependency(hub, 8002, bus.deviceId());
    hub.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveHubReady(hub, 10);

    AnalogInputReading reading{};
    const char* status = nullptr;
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Pending),
                      static_cast<int>(hub.pollChannelReading(0, 111, 20, reading, status)));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Busy),
                      static_cast<int>(hub.pollChannelReading(2, 222, 21, reading, status)));
}

void test_ads1115_hub_rejects_out_of_range_channel() {
    FakeAds1115I2cDriver i2c;
    I2cBusDevice bus(makeBusConfig(), i2c);
    driveBusReady(bus);

    Ads1115HubDevice hub(makeHubConfig());
    bindHubDependency(hub, 8003, bus.deviceId());
    hub.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveHubReady(hub, 10);

    AnalogInputReading reading{};
    const char* status = nullptr;
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Fault),
                      static_cast<int>(hub.pollChannelReading(4, 111, 20, reading, status)));
}

void test_ads1115_hub_faults_after_repeated_i2c_failures() {
    FakeAds1115I2cDriver i2c;
    i2c.present = false;
    I2cBusDevice bus(makeBusConfig(), i2c);
    driveBusReady(bus);

    Ads1115HubDevice hub(makeHubConfig());
    bindHubDependency(hub, 8004, bus.deviceId());
    hub.setDependencyRuntime(DeviceRole::I2CBus, &bus);

    // Each failed probe backs off ~1s before retrying (RetryBackoff -> Starting), and the hub
    // faults on the 3rd consecutive failure -- advance in 100ms steps for several seconds of
    // simulated time so those backoffs actually elapse.
    hub.begin(10);
    for (uint32_t now = 110; now < 5000 && hub.status() != DeviceStatus::Faulted; now += 100U) {
        hub.tick100ms(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(hub.status()));
}

void test_ads1115_hub_generation_changes_on_reconfigure() {
    FakeAds1115I2cDriver i2c;
    I2cBusDevice bus(makeBusConfig(), i2c);
    driveBusReady(bus);

    Ads1115HubDevice hub(makeHubConfig());
    bindHubDependency(hub, 8005, bus.deviceId());
    hub.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveHubReady(hub, 10);

    const uint32_t firstGeneration = hub.generation();
    TEST_ASSERT_NOT_EQUAL(0U, firstGeneration);

    hub.requestReconfigure();
    for (uint32_t now = 100; now < 140 && hub.status() != DeviceStatus::Ready; ++now) {
        hub.tick100ms(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(hub.status()));
    TEST_ASSERT_NOT_EQUAL(firstGeneration, hub.generation());
}
