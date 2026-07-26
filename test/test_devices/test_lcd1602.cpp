#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/lcd1602/Lcd1602Device.h"
#include "devices/display/lcd1602/Lcd1602DeviceConfig.h"
#include "devices/expander/Pcf8574ExpanderDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/expander/PortExpanderSwitchDeviceApiAdapter.h"
#include "integrations/rest/lcd1602/Lcd1602DeviceApiAdapter.h"
#include "metrics/MetricValueResolver.h"
#include "wifi/WifiDriver.h"

#include <cstdio>
#include <string>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

// Minimal fake of the I2C register-write primitives a PCF857x expander drives -- write-only, no
// register-select step. Only `writeCount` is needed here (the byte content is exercised by
// test_pcf857x_expander.cpp); this test cares about *when* Lcd1602Device writes, not the exact
// HD44780 byte stream.
class FakeI2cDriver final : public II2cBusDriver {
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
        return 100000U;
    }
    void beginTransmission(uint8_t address) override {
        lastAddress = address;
    }
    uint8_t endTransmission(bool) override {
        ++writeCount;
        return 0U;
    }
    size_t requestFrom(uint8_t, size_t, bool) override {
        return 0U;
    }
    size_t write(uint8_t) override {
        return 1U;
    }
    size_t write(const uint8_t*, size_t quantity) override {
        return quantity;
    }
    int available() override {
        return 0;
    }
    int read() override {
        return -1;
    }
    void flush() override {}

    uint8_t lastAddress{0};
    uint32_t writeCount{0};
};

class FakeWifiDriver final : public IWifiDriver {
public:
    bool begin() override {
        return true;
    }
    bool beginStation(const WiFiCredentials&) override {
        return true;
    }
    void disconnect() override {}
    void clearStationCredentials() override {}
    bool startSetupAp(const std::string&, const std::string&) override {
        return true;
    }
    void stopSetupAp() override {}
    WifiDriverStatus status() const override {
        return WifiDriverStatus::Connected;
    }
    bool networkStackReady() const override {
        return true;
    }
    bool stationReady() const override {
        return true;
    }
    bool setupApReady() const override {
        return false;
    }
    std::string stationIp() const override {
        return "192.168.1.50";
    }
    std::string setupApIp() const override {
        return "";
    }
    bool startScan() override {
        return true;
    }
    bool scanComplete(std::vector<WifiNetwork>&, size_t) override {
        return false;
    }
    std::string macSuffix() const override {
        return "abcd";
    }
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

Pcf857xExpanderConfigV2 makeExpanderConfig(uint8_t i2cAddress = 0x20U) {
    Pcf857xExpanderConfigV2 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "expander");
    config.i2cAddress = i2cAddress;
    return config;
}

Lcd1602DeviceConfigV1 makeLcdConfig(const char* name = "lcd") {
    Lcd1602DeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeBusPayload(const I2cBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, config, buffer, i2cBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, i2cBusDeviceConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeExpanderPayload(const Pcf857xExpanderConfigV2& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, config, buffer, pcf857xExpanderConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, pcf857xExpanderConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeSwitchPayload(const PortExpanderSwitchDeviceConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, config, buffer, portExpanderSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, portExpanderSwitchDeviceConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeLcdPayload(const Lcd1602DeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Lcd1602DeviceConfigV1::kMagic, config, buffer, lcd1602DeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, lcd1602DeviceConfigSize(config)));
    return payload;
}

void driveBusReady(I2cBusDevice& bus, uint32_t startNow = 1U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
}

void bindExpanderDependency(Pcf8574ExpanderDevice& expander, DeviceId expanderId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = expanderId;
    record.header.typeId = Pcf8574ExpanderDevice::descriptor().typeId;
    record.header.configVersion = Pcf8574ExpanderDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::I2CBus, busId};
    record.status = DeviceStatus::Ready;
    expander.bindDeviceIdentity(record, encodeExpanderPayload(expander.config()));
}

void driveExpanderUntilReady(IDeviceRuntime& expander, uint32_t startNow = 10U) {
    expander.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && expander.status() != DeviceStatus::Ready; now += 1U) {
        expander.tick1s(now);
    }
}

void bindLcdDependency(Lcd1602Device& lcd, DeviceId lcdId, DeviceId expanderId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = lcdId;
    record.header.typeId = Lcd1602Device::descriptor().typeId;
    record.header.configVersion = Lcd1602Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::PortExpander, expanderId};
    record.status = DeviceStatus::Ready;
    lcd.bindDeviceIdentity(record, DeviceConfigBlob{});
}

void driveLcdUntilReady(Lcd1602Device& lcd, uint32_t startNow = 10U) {
    lcd.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && lcd.status() != DeviceStatus::Ready; now += 10U) {
        lcd.tick100ms(now);
    }
}

DeviceCreateRequest makeBusCreateRequest(const char* name) {
    DeviceCreateRequest request{};
    request.typeId = I2cBusDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeBusPayload(makeBusConfig());
    return request;
}

DeviceCreateRequest makeExpanderCreateRequest(const char* name, DeviceId busId) {
    DeviceCreateRequest request{};
    request.typeId = Pcf8574ExpanderDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::I2CBus, busId};
    request.configVersion = Pcf8574ExpanderDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeExpanderPayload(makeExpanderConfig());
    return request;
}

DeviceCreateRequest makeSwitchCreateRequest(const char* name, DeviceId expanderId, uint8_t channel) {
    DeviceCreateRequest request{};
    request.typeId = PortExpanderSwitchDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::PortExpander, expanderId};
    request.configVersion = PortExpanderSwitchDevice::descriptor().currentConfigVersion;
    PortExpanderSwitchDeviceConfigV3 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    config.channel = channel;
    request.configBlob = encodeSwitchPayload(config);
    return request;
}

DeviceCreateRequest makeLcdCreateRequest(const char* name, DeviceId expanderId, uint8_t rsChannel) {
    DeviceCreateRequest request{};
    request.typeId = Lcd1602Device::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::PortExpander, expanderId};
    request.configVersion = Lcd1602Device::descriptor().currentConfigVersion;
    Lcd1602DeviceConfigV1 config = makeLcdConfig(name);
    config.rsChannel = rsChannel;
    request.configBlob = encodeLcdPayload(config);
    return request;
}

} // namespace

void test_lcd1602_config_codec_json_and_validation() {
    const Lcd1602DeviceConfigV1 config = makeLcdConfig();
    TEST_ASSERT_TRUE(config.validate().ok());

    StaticJsonDocument<512> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_UINT8(0U, json["rsChannel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(2U, json["eChannel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(4U, json["d4Channel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(3U, json["backlightChannel"].as<uint8_t>());

    json["line1"] = "Hello";
    json["line2"] = "World";
    Lcd1602DeviceConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parsed.parseJson(json, error));
    TEST_ASSERT_EQUAL_STRING("Hello", parsed.line1);
    TEST_ASSERT_EQUAL_STRING("World", parsed.line2);
    TEST_ASSERT_TRUE(parsed.validate().ok());
}

void test_lcd1602_config_rejects_invalid_fields() {
    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    json["enabled"] = true;
    json["name"] = "lcd";
    json["line1"] = "01234567890123456"; // 17 chars, exceeds the 16-char panel width
    Lcd1602DeviceConfigV1 tooLong{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(tooLong.parseJson(json, error));
    TEST_ASSERT_NOT_NULL(error);

    Lcd1602DeviceConfigV1 duplicateChannels = makeLcdConfig();
    duplicateChannels.rsChannel = duplicateChannels.d4Channel; // collides with default d4Channel=4
    TEST_ASSERT_FALSE(duplicateChannels.validate().ok());

    Lcd1602DeviceConfigV1 outOfRange = makeLcdConfig();
    outOfRange.eChannel = 20U; // PCF8575 tops out at channel 15
    TEST_ASSERT_FALSE(outOfRange.validate().ok());
}

void test_lcd1602_config_channels_skip_unset_backlight() {
    Lcd1602DeviceConfigV1 config = makeLcdConfig();
    uint8_t channels[7]{};
    TEST_ASSERT_EQUAL_UINT8(7U, lcd1602ConfigChannels(config, channels, 7U));

    config.backlightChannel = kLcd1602ChannelUnset;
    TEST_ASSERT_EQUAL_UINT8(6U, lcd1602ConfigChannels(config, channels, 7U));
}

void test_lcd1602_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();

    const DeviceTypeDescriptor* descriptor = typeRegistry.find(Lcd1602Device::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_FALSE(descriptor->providedRoles.contains(DeviceRole::PortExpander));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("lcd1602"));
    TEST_ASSERT_EQUAL_STRING("lcd1602", Lcd1602DeviceApiAdapter::instance().typeName());
}

void test_lcd1602_reaches_ready_through_port_expander_and_renders_with_diffing() {
    FakeI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8574ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 5001, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveExpanderUntilReady(expander);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(expander.status()));

    Lcd1602Device lcd(makeLcdConfig());
    bindLcdDependency(lcd, 5002, expander.deviceId());
    lcd.setDependencyRuntime(DeviceRole::PortExpander, &expander);
    driveLcdUntilReady(lcd);
    TEST_ASSERT_EQUAL_MESSAGE(static_cast<int>(DeviceStatus::Ready), static_cast<int>(lcd.status()),
                              "lcd1602 should reach Ready once its port-expander dependency and HD44780 init sequence succeed");

    FakeWifiDriver wifi;
    const MetricValueResolver resolver(nullptr, wifi, 1000U);

    const uint32_t writesBeforeFirstRender = driver.writeCount;
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 1001U));
    const uint32_t writesAfterFirstRender = driver.writeCount;
    TEST_ASSERT_TRUE_MESSAGE(writesAfterFirstRender > writesBeforeFirstRender, "first render must write both lines to hardware");

    // Re-rendering identical content must not touch hardware again.
    TEST_ASSERT_FALSE(lcd.renderText(resolver, 1002U));
    TEST_ASSERT_EQUAL_UINT32(writesAfterFirstRender, driver.writeCount);

    // Changing only line1 must write again (and line2's cached copy must not force a rewrite).
    Lcd1602DeviceConfigV1 changedLine1 = lcd.config();
    std::snprintf(changedLine1.line1, sizeof(changedLine1.line1), "%s", "Changed");
    BoundedBlob<kMaxDeviceConfigBytes> updatedBlob = encodeLcdPayload(changedLine1);
    TEST_ASSERT_TRUE(lcd.applyConfig(updatedBlob, 1003U));
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 1004U));
    TEST_ASSERT_TRUE(driver.writeCount > writesAfterFirstRender);
}

void test_lcd1602_adapter_rejects_duplicate_channel_on_same_expander_but_allows_distinct_channel() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    SequentialDeviceIdSource ids(9000U);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    const DeviceCreateResult busResult = registry.create(makeBusCreateRequest("bus"), 10);
    TEST_ASSERT_TRUE_MESSAGE(busResult.ok(), busResult.validation.message);

    const DeviceCreateResult expanderResult = registry.create(makeExpanderCreateRequest("expander", busResult.deviceId), 11);
    TEST_ASSERT_TRUE_MESSAGE(expanderResult.ok(), expanderResult.validation.message);

    // Occupy channel 2 with an ordinary switch first.
    const DeviceCreateResult switchResult = registry.create(makeSwitchCreateRequest("sw", expanderResult.deviceId, 2U), 12);
    TEST_ASSERT_TRUE_MESSAGE(switchResult.ok(), switchResult.validation.message);

    // The LCD's default eChannel is also 2 -- must collide with the switch above even though the
    // switch and the LCD disagree on every other channel.
    const DeviceCreateRequest collidingLcd = makeLcdCreateRequest("lcd-bad", expanderResult.deviceId, 0U);
    const DeviceValidationResult collidingValidation = Lcd1602DeviceApiAdapter::instance().validateCreateRequest(collidingLcd, registry);
    TEST_ASSERT_FALSE(collidingValidation.ok());

    // Moving every LCD channel off of the busy channel 2 must succeed. Pcf8574 (the expander used
    // here) only has 8 channels (0-7), so stay within that range.
    DeviceCreateRequest okLcd = makeLcdCreateRequest("lcd-ok", expanderResult.deviceId, 0U);
    Lcd1602DeviceConfigV1 okConfig{};
    TEST_ASSERT_TRUE(decodeLcd1602DeviceConfig(okLcd.configBlob.data(), okLcd.configBlob.size(), okConfig));
    okConfig.rsChannel = 0U;
    okConfig.eChannel = 1U;
    okConfig.d4Channel = 3U;
    okConfig.d5Channel = 4U;
    okConfig.d6Channel = 5U;
    okConfig.d7Channel = 6U;
    okConfig.backlightChannel = 7U;
    okLcd.configBlob = encodeLcdPayload(okConfig);
    const DeviceValidationResult okValidation = Lcd1602DeviceApiAdapter::instance().validateCreateRequest(okLcd, registry);
    TEST_ASSERT_TRUE_MESSAGE(okValidation.ok(), okValidation.message);
}
