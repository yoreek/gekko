#include "JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/analog/input/ads1115/Ads1115HubDevice.h"
#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDevice.h"
#include "devices/analog/input/channel/AnalogInputChannelDevice.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/analog_input/AnalogInputChannelDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <string>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}

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
        return 0U;
    }

    size_t requestFrom(uint8_t, size_t size, bool) override {
        readBuffer.clear();
        readPos = 0;
        readBuffer.push_back(static_cast<uint8_t>(static_cast<uint16_t>(conversionRaw) >> 8));
        readBuffer.push_back(static_cast<uint8_t>(static_cast<uint16_t>(conversionRaw) & 0xFFU));
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

    int16_t conversionRaw{8192};
    uint8_t lastAddress{0};

private:
    std::vector<uint8_t> pendingWrite;
    std::vector<uint8_t> readBuffer;
    size_t readPos{0};
};

class FakeGpioOutputDriver final : public IGpioOutputDriver {
public:
    bool configureOutput(uint8_t pin, bool initialLevel) override {
        (void)pin;
        (void)initialLevel;
        return true;
    }
    bool write(uint8_t pin, bool level) override {
        (void)pin;
        (void)level;
        return true;
    }
    void release(uint8_t pin) override {
        (void)pin;
    }
};

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

    uint32_t milliVolts{1650};
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

Ads1115HubDeviceConfigV1 makeAds1115HubConfig() {
    Ads1115HubDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "ads1115");
    config.i2cAddress = kAds1115DefaultI2cAddress;
    config.gain = static_cast<uint8_t>(Ads1115Gain::Fsr2048);
    config.dataRateSps = static_cast<uint8_t>(Ads1115DataRate::Sps860);
    return config;
}

Cd74hc4067HubDeviceConfigV1 makeCd74hc4067HubConfig() {
    Cd74hc4067HubDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", "mux hub");
    return config;
}

AnalogInputChannelDeviceConfigV1 makeChannelConfig(uint8_t channel = 3) {
    AnalogInputChannelDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "analog input channel");
    config.channel = channel;
    config.poll.adcSamples = 2;
    config.poll.reportAlways = 0;
    config.poll.reportDeltaMilliVolts = 10;
    config.poll.pollMs = 100;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeChannelPayload(const AnalogInputChannelDeviceConfigV1& config);

void writeChannelConfigDoc(StaticJsonDocument<512>& doc, const AnalogInputChannelDeviceConfigV1& config, DeviceId hubId) {
    doc.clear();
    doc["typeName"] = "analog_input_channel";
    JsonObject configJson = doc.createNestedObject("config");
    config.writeJson(configJson);
    JsonArray deps = configJson.createNestedArray("deps");
    JsonObject dependency = deps.createNestedObject();
    dependency["role"] = "analog_input_hub";
    dependency["deviceId"] = hubId;
}

void bindLeafIdentity(AnalogInputChannelDevice& leaf, DeviceId leafId, DeviceId hubId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = leafId;
    record.header.typeId = AnalogInputChannelDevice::descriptor().typeId;
    record.header.configVersion = AnalogInputChannelDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodeChannelPayload(leaf.config()).size());
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::AnalogInputHub, hubId, false};
    record.status = DeviceStatus::Ready;
    leaf.bindDeviceIdentity(record, encodeChannelPayload(leaf.config()));
}

BoundedBlob<kMaxDeviceConfigBytes> encodeChannelPayload(const AnalogInputChannelDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic, config, buffer, analogInputChannelDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, analogInputChannelDeviceConfigSize(config)));
    return payload;
}

void bindDependency(AnalogInputChannelDevice& leaf, DeviceId leafId, DeviceId hubId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = leafId;
    record.deps[0] = {DeviceRole::AnalogInputHub, hubId, false};
    record.depCount = 1;
    leaf.bindDeviceIdentity(record, encodeChannelPayload(leaf.config()));
}

void bindHubDependency(Ads1115HubDevice& hub, DeviceId hubId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = hubId;
    record.deps[0] = {DeviceRole::I2CBus, busId, false};
    record.depCount = 1;
    hub.bindDeviceIdentity(record, BoundedBlob<kMaxDeviceConfigBytes>{});
}

void driveBusReady(I2cBusDevice& bus, uint32_t startNow = 1U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
}

void driveAds1115HubReady(Ads1115HubDevice& hub, uint32_t startNow) {
    hub.begin(startNow);
    hub.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(hub.status()));
}

void bringUpCd74hc4067Hub(Cd74hc4067HubDevice& hub, uint32_t startNow = 10) {
    hub.begin(startNow);
    hub.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(hub.status()));
}

template <typename Hub> void driveUntilReading(AnalogInputChannelDevice& leaf, Hub& hub, uint32_t startNow, uint32_t maxTicks) {
    leaf.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + maxTicks && !leaf.reading().valid; ++now) {
        hub.tick100ms(now);
        leaf.tick100ms(now);
    }
}

DeviceCreateRequest makeCd74hc4067HubCreateRequest(const char* name) {
    DeviceCreateRequest request{};
    request.typeId = Cd74hc4067HubDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.configVersion = Cd74hc4067HubDevice::descriptor().currentConfigVersion;
    Cd74hc4067HubDeviceConfigV1 config = makeCd74hc4067HubConfig();
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic, config, buffer, cd74hc4067HubDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(request.configBlob.assign(buffer, cd74hc4067HubDeviceConfigSize(config)));
    return request;
}

DeviceCreateRequest makeChannelCreateRequest(const char* name, DeviceId hubId, uint8_t channel) {
    DeviceCreateRequest request{};
    request.typeId = AnalogInputChannelDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::AnalogInputHub, hubId};
    request.configVersion = AnalogInputChannelDevice::descriptor().currentConfigVersion;
    AnalogInputChannelDeviceConfigV1 config = makeChannelConfig(channel);
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic, config, buffer, analogInputChannelDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(request.configBlob.assign(buffer, analogInputChannelDeviceConfigSize(config)));
    return request;
}

} // namespace

void test_analog_input_channel_config_rejects_invalid_fields() {
    AnalogInputChannelDeviceConfigV1 badChannel = makeChannelConfig();
    badChannel.channel = 20; // only 0..15 valid as a generic sanity bound
    TEST_ASSERT_FALSE(badChannel.validate().ok());

    AnalogInputChannelDeviceConfigV1 badSamples = makeChannelConfig();
    badSamples.poll.adcSamples = 0;
    TEST_ASSERT_FALSE(badSamples.validate().ok());

    AnalogInputChannelDeviceConfigV1 good = makeChannelConfig();
    TEST_ASSERT_TRUE_MESSAGE(good.validate().ok(), good.validate().message);

    StaticJsonDocument<512> doc;
    writeChannelConfigDoc(doc, good, 5001);
    assertMatchesJsonSchema("schemas/rest/v1/devices/analog_input_channel.config.schema.json", doc["config"].as<JsonVariantConst>());
}

void test_analog_input_channel_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kAnalogInputChannelTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::AnalogInput));
    TEST_ASSERT_EQUAL_UINT32(1, descriptor->dependencyRequirements.size());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceRole::AnalogInputHub), static_cast<int>(descriptor->dependencyRequirements[0].role));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("analog_input_channel"));
}

// Same leaf typeId/class against two unrelated hub chips -- proves the leaf is genuinely
// hub-agnostic (it never names Ads1115HubDevice or Cd74hc4067HubDevice, only the generic
// IAnalogInputHubRuntime role), so there is no reason for a second, chip-specific channel type.
void test_analog_input_channel_reads_through_ads1115_hub_and_averages_samples() {
    FakeAds1115I2cDriver i2c;
    i2c.conversionRaw = 16384; // half-scale -> 1024 mV at +-2.048V gain
    I2cBusDevice bus(makeBusConfig(), i2c);
    driveBusReady(bus);

    Ads1115HubDevice hub(makeAds1115HubConfig());
    bindHubDependency(hub, 7001, bus.deviceId());
    hub.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveAds1115HubReady(hub, 10);

    AnalogInputChannelDevice leaf(makeChannelConfig(1));
    bindDependency(leaf, 7002, hub.deviceId());
    bindLeafIdentity(leaf, 7002, hub.deviceId());
    leaf.setDependencyRuntime(DeviceRole::AnalogInputHub, &hub);

    driveUntilReading(leaf, hub, 20, 200U);

    TEST_ASSERT_TRUE(leaf.reading().valid);
    TEST_ASSERT_EQUAL_INT32(1024, leaf.reading().milliVolts);
    TEST_ASSERT_EQUAL_STRING("ok", leaf.outputStatus());

    StaticJsonDocument<1024> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    AnalogInputChannelDeviceApiAdapter::instance().writeDeviceJson(leaf, leaf.status(), output);
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-analog_input_channel.response.schema.json",
                            outputDoc.as<JsonVariantConst>());
}

void test_analog_input_channel_reads_through_cd74hc4067_hub_and_averages_samples() {
    FakeGpioOutputDriver gpio;
    FakeAdcInputDriver adc;
    adc.milliVolts = 1700;
    Cd74hc4067HubDevice hub(makeCd74hc4067HubConfig(), gpio, adc);
    bringUpCd74hc4067Hub(hub);

    AnalogInputChannelDevice leaf(makeChannelConfig(3));
    bindDependency(leaf, 9001, 9000);
    bindLeafIdentity(leaf, 9001, 9000);
    leaf.setDependencyRuntime(DeviceRole::AnalogInputHub, &hub);

    driveUntilReading(leaf, hub, 20, 40U);

    TEST_ASSERT_TRUE(leaf.reading().valid);
    TEST_ASSERT_EQUAL_INT32(1700, leaf.reading().milliVolts);
    TEST_ASSERT_EQUAL_STRING("ok", leaf.outputStatus());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(leaf.status()));
}

void test_analog_input_channel_reports_dependency_blocked_without_hub() {
    AnalogInputChannelDevice leaf(makeChannelConfig(1));
    bindDependency(leaf, 7003, 7000);

    leaf.begin(20);
    leaf.tick100ms(21);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(leaf.status()));
    TEST_ASSERT_FALSE(leaf.reading().valid);
}

void test_analog_input_channel_expander_channel_reports_configured_channel() {
    AnalogInputChannelDevice leaf(makeChannelConfig(7));
    uint8_t channel = 0;
    TEST_ASSERT_TRUE(leaf.expanderChannel(channel));
    TEST_ASSERT_EQUAL_UINT8(7, channel);
}

void test_analog_input_channel_disabling_mid_request_frees_the_hub_for_other_requesters() {
    FakeGpioOutputDriver gpio;
    FakeAdcInputDriver adc;
    Cd74hc4067HubDevice hub(makeCd74hc4067HubConfig(), gpio, adc);
    bringUpCd74hc4067Hub(hub);

    AnalogInputChannelDevice leaf(makeChannelConfig(2));
    bindDependency(leaf, 9101, 9100);
    bindLeafIdentity(leaf, 9101, 9100);
    leaf.setDependencyRuntime(DeviceRole::AnalogInputHub, &hub);

    // Idle -> Starting -> Sampling's first pollChannelReading call, which switches the mux lines
    // and leaves the hub owned by this leaf in the Pending state (settle time not yet elapsed).
    leaf.begin(30);
    leaf.tick100ms(31);
    leaf.tick100ms(32);
    TEST_ASSERT_FALSE(leaf.reading().valid);

    AnalogInputReading probe{};
    const char* probeStatus = nullptr;
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Busy),
                      static_cast<int>(hub.pollChannelReading(9, 5555, 33, probe, probeStatus)));

    leaf.requestDisable();
    leaf.tick100ms(34);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(leaf.status()));

    // The hub must be free again for a different requester now that the leaf released its claim.
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(AnalogInputHubPollResult::Busy),
                          static_cast<int>(hub.pollChannelReading(9, 5555, 35, probe, probeStatus)));
}

void test_analog_input_channel_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    writeChannelConfigDoc(createDoc, makeChannelConfig(4), 7000);
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-analog_input_channel.request.schema.json",
                            createDoc.as<JsonVariantConst>());

    StaticJsonDocument<512> updateDoc;
    updateDoc["deps"] = updateDoc.createNestedArray("deps");
    JsonObject config = updateDoc.createNestedObject("config");
    config["channel"] = 4;
    config["adcSamples"] = 2;
    config["pollMs"] = 200;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-analog_input_channel.request.schema.json",
                            updateDoc.as<JsonVariantConst>());
}

void test_analog_input_channel_registry_rejects_duplicate_channel_on_same_hub_but_allows_distinct_channels() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(6000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    const DeviceCreateResult hubResult = registry.create(makeCd74hc4067HubCreateRequest("mux"), 10);
    TEST_ASSERT_TRUE_MESSAGE(hubResult.ok(), hubResult.validation.message);

    const DeviceCreateResult first = registry.create(makeChannelCreateRequest("ch0", hubResult.deviceId, 0U), 20);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);

    const DeviceCreateRequest duplicateChannel = makeChannelCreateRequest("ch0-dup", hubResult.deviceId, 0U);
    const DeviceValidationResult duplicateValidation =
        AnalogInputChannelDeviceApiAdapter::instance().validateCreateRequest(duplicateChannel, registry);
    TEST_ASSERT_FALSE(duplicateValidation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(duplicateValidation.error));

    const DeviceCreateRequest distinctChannel = makeChannelCreateRequest("ch1", hubResult.deviceId, 1U);
    const DeviceValidationResult distinctValidation =
        AnalogInputChannelDeviceApiAdapter::instance().validateCreateRequest(distinctChannel, registry);
    TEST_ASSERT_TRUE_MESSAGE(distinctValidation.ok(), distinctValidation.message);
}
