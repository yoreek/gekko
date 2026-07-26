#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/lcd1602/Lcd1602Device.h"
#include "devices/display/lcd2004/Lcd2004Device.h"
#include "devices/display/lcd2004/Lcd2004DeviceConfig.h"
#include "devices/expander/Pcf8574ExpanderDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/expander/PortExpanderSwitchDeviceApiAdapter.h"
#include "integrations/rest/lcd2004/Lcd2004DeviceApiAdapter.h"
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
// test_pcf857x_expander.cpp and, for the row-address arithmetic, by RecordingPortExpanderRuntime
// below); this test cares about *when* Lcd2004Device writes, not this fake's exact I2C bytes.
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

// Directly implements IPortExpanderRuntime (bypassing the PCF8574+I2C chain) so the HD44780 4-bit
// byte stream can be decoded and asserted on -- specifically the DDRAM set-address command Lcd2004
// sends for each row, which is the one genuinely new piece of logic this type adds over lcd1602
// (Hd44780CharacterDisplayDeviceBase::rowAddress()'s row>=2 half-offset).
class RecordingPortExpanderRuntime final : public DeviceRuntimeBase, public IPortExpanderRuntime {
public:
    RecordingPortExpanderRuntime(uint8_t rsChannel, uint8_t eChannel, uint8_t d4Channel, uint8_t d5Channel, uint8_t d6Channel,
                                 uint8_t d7Channel)
        : DeviceRuntimeBase((PState)&RecordingPortExpanderRuntime::Idle), rsChannel_(rsChannel), eChannel_(eChannel), d4Channel_(d4Channel),
          d5Channel_(d5Channel), d6Channel_(d6Channel), d7Channel_(d7Channel) {
        // dependenciesReady() (DeviceRuntimeBase.cpp) requires the dependency's own status() to be
        // Ready -- this fake has no real lifecycle, so it just reports Ready immediately.
        setStatus(DeviceStatus::Ready);
    }

    uint8_t channelCount() const override {
        return 20U;
    }
    bool isChannelOn(uint8_t channel) const override {
        return channel < 32U && (state_ & (1UL << channel)) != 0UL;
    }
    const IPortExpanderRuntime* portExpanderRuntime() const override {
        return this;
    }

    bool requestChannelState(uint8_t channel, bool on, uint32_t) override {
        if (channel >= 32U) {
            return false;
        }
        const bool wasEOn = isChannelOn(eChannel_);
        if (on) {
            state_ |= (1UL << channel);
        } else {
            state_ &= ~(1UL << channel);
        }
        if (channel == eChannel_ && wasEOn && !on) {
            const bool rs = isChannelOn(rsChannel_);
            uint8_t nibble = 0U;
            if (isChannelOn(d4Channel_)) {
                nibble |= 0x1U;
            }
            if (isChannelOn(d5Channel_)) {
                nibble |= 0x2U;
            }
            if (isChannelOn(d6Channel_)) {
                nibble |= 0x4U;
            }
            if (isChannelOn(d7Channel_)) {
                nibble |= 0x8U;
            }
            latchNibble(rs, nibble);
        }
        return true;
    }

    struct DecodedByte {
        bool rs;
        uint8_t value;
    };
    std::vector<DecodedByte> bytes;

private:
    void latchNibble(bool rs, uint8_t nibble) {
        if (!havePendingHighNibble_) {
            pendingHighNibble_ = nibble;
            pendingRs_ = rs;
            havePendingHighNibble_ = true;
            return;
        }
        bytes.push_back({pendingRs_, static_cast<uint8_t>(static_cast<uint8_t>(pendingHighNibble_ << 4U) | nibble)});
        havePendingHighNibble_ = false;
    }

    State Idle() {}

    uint32_t state_{0};
    uint8_t rsChannel_;
    uint8_t eChannel_;
    uint8_t d4Channel_;
    uint8_t d5Channel_;
    uint8_t d6Channel_;
    uint8_t d7Channel_;
    bool havePendingHighNibble_{false};
    uint8_t pendingHighNibble_{0};
    bool pendingRs_{false};
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

Lcd2004DeviceConfigV1 makeLcdConfig(const char* name = "lcd") {
    Lcd2004DeviceConfigV1 config{};
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

BoundedBlob<kMaxDeviceConfigBytes> encodeLcdPayload(const Lcd2004DeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Lcd2004DeviceConfigV1::kMagic, config, buffer, lcd2004DeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, lcd2004DeviceConfigSize(config)));
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

void bindLcdDependency(Lcd2004Device& lcd, DeviceId lcdId, DeviceId expanderId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = lcdId;
    record.header.typeId = Lcd2004Device::descriptor().typeId;
    record.header.configVersion = Lcd2004Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::PortExpander, expanderId};
    record.status = DeviceStatus::Ready;
    lcd.bindDeviceIdentity(record, DeviceConfigBlob{});
}

void driveLcdUntilReady(Lcd2004Device& lcd, uint32_t startNow = 10U) {
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
    request.typeId = Lcd2004Device::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::PortExpander, expanderId};
    request.configVersion = Lcd2004Device::descriptor().currentConfigVersion;
    Lcd2004DeviceConfigV1 config = makeLcdConfig(name);
    config.channels.rsChannel = rsChannel;
    request.configBlob = encodeLcdPayload(config);
    return request;
}

} // namespace

void test_lcd2004_config_codec_json_and_validation() {
    const Lcd2004DeviceConfigV1 config = makeLcdConfig();
    TEST_ASSERT_TRUE(config.validate().ok());

    StaticJsonDocument<512> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_UINT8(0U, json["rsChannel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(2U, json["eChannel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(4U, json["d4Channel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(3U, json["backlightChannel"].as<uint8_t>());

    json["line1"] = "Row0";
    json["line2"] = "Row1";
    json["line3"] = "Row2";
    json["line4"] = "Row3";
    Lcd2004DeviceConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parsed.parseJson(json, error));
    TEST_ASSERT_EQUAL_STRING("Row0", parsed.line1);
    TEST_ASSERT_EQUAL_STRING("Row1", parsed.line2);
    TEST_ASSERT_EQUAL_STRING("Row2", parsed.line3);
    TEST_ASSERT_EQUAL_STRING("Row3", parsed.line4);
    TEST_ASSERT_TRUE(parsed.validate().ok());
}

void test_lcd2004_config_rejects_invalid_fields() {
    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    json["enabled"] = true;
    json["name"] = "lcd";
    json["line3"] = "012345678901234567890"; // 21 chars, exceeds the 20-char panel width
    Lcd2004DeviceConfigV1 tooLong{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(tooLong.parseJson(json, error));
    TEST_ASSERT_NOT_NULL(error);

    Lcd2004DeviceConfigV1 duplicateChannels = makeLcdConfig();
    duplicateChannels.channels.rsChannel = duplicateChannels.channels.d4Channel;
    TEST_ASSERT_FALSE(duplicateChannels.validate().ok());
}

void test_lcd2004_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();

    const DeviceTypeDescriptor* descriptor = typeRegistry.find(Lcd2004Device::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("lcd2004"));
    TEST_ASSERT_EQUAL_STRING("lcd2004", Lcd2004DeviceApiAdapter::instance().typeName());
    TEST_ASSERT_TRUE(Lcd2004Device::descriptor().typeId != Lcd1602Device::descriptor().typeId);
}

void test_lcd2004_reaches_ready_through_port_expander_and_renders_with_diffing() {
    FakeI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8574ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 6001, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveExpanderUntilReady(expander);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(expander.status()));

    Lcd2004Device lcd(makeLcdConfig());
    bindLcdDependency(lcd, 6002, expander.deviceId());
    lcd.setDependencyRuntime(DeviceRole::PortExpander, &expander);
    driveLcdUntilReady(lcd);
    TEST_ASSERT_EQUAL_MESSAGE(static_cast<int>(DeviceStatus::Ready), static_cast<int>(lcd.status()),
                              "lcd2004 should reach Ready once its port-expander dependency and HD44780 init sequence succeed");

    FakeWifiDriver wifi;
    const MetricValueResolver resolver(nullptr, wifi, 1000U);

    const uint32_t writesBeforeFirstRender = driver.writeCount;
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 1001U));
    const uint32_t writesAfterFirstRender = driver.writeCount;
    TEST_ASSERT_TRUE_MESSAGE(writesAfterFirstRender > writesBeforeFirstRender, "first render must write all four lines to hardware");

    // Re-rendering identical content must not touch hardware again.
    TEST_ASSERT_FALSE(lcd.renderText(resolver, 1002U));
    TEST_ASSERT_EQUAL_UINT32(writesAfterFirstRender, driver.writeCount);

    // Changing only line3 must write again.
    Lcd2004DeviceConfigV1 changedLine3 = lcd.config();
    std::snprintf(changedLine3.line3, sizeof(changedLine3.line3), "%s", "Changed");
    BoundedBlob<kMaxDeviceConfigBytes> updatedBlob = encodeLcdPayload(changedLine3);
    TEST_ASSERT_TRUE(lcd.applyConfig(updatedBlob, 1003U));
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 1004U));
    TEST_ASSERT_TRUE(driver.writeCount > writesAfterFirstRender);
}

void test_lcd2004_sends_correct_ddram_row_addresses() {
    // Standard PCF8574 LCM-IIC wiring: RS=0, E=2, D4-D7=4-7, backlight=3.
    RecordingPortExpanderRuntime expander(0U, 2U, 4U, 5U, 6U, 7U);

    Lcd2004DeviceConfigV1 config = makeLcdConfig();
    std::snprintf(config.line1, sizeof(config.line1), "%s", "A");
    std::snprintf(config.line2, sizeof(config.line2), "%s", "B");
    std::snprintf(config.line3, sizeof(config.line3), "%s", "C");
    std::snprintf(config.line4, sizeof(config.line4), "%s", "D");

    Lcd2004Device lcd(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 7001;
    record.header.typeId = Lcd2004Device::descriptor().typeId;
    record.header.configVersion = Lcd2004Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::PortExpander, 7000};
    record.status = DeviceStatus::Ready;
    lcd.bindDeviceIdentity(record, DeviceConfigBlob{});
    lcd.setDependencyRuntime(DeviceRole::PortExpander, &expander);

    lcd.begin(10U);
    for (uint32_t now = 11U; now < 5000U && lcd.status() != DeviceStatus::Ready; now += 10U) {
        lcd.tick100ms(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(lcd.status()));

    FakeWifiDriver wifi;
    const MetricValueResolver resolver(nullptr, wifi, 6000U);
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 6001U));

    // Each row write starts with a command byte (rs=false): 0x80 | rowAddress. The general HD44780
    // rule (not just the well-known 20x4 case): rows 0/1 start at 0x00/0x40; rows 2/3 continue in
    // the same 40-byte DDRAM half, offset by the panel's column count (20).
    std::vector<uint8_t> rowAddressCommands;
    for (const auto& byte : expander.bytes) {
        if (!byte.rs && (byte.value & 0x80U) != 0U) {
            rowAddressCommands.push_back(static_cast<uint8_t>(byte.value & 0x7FU));
        }
    }
    TEST_ASSERT_EQUAL_UINT(4U, rowAddressCommands.size());
    TEST_ASSERT_EQUAL_HEX8(0x00U, rowAddressCommands[0]);
    TEST_ASSERT_EQUAL_HEX8(0x40U, rowAddressCommands[1]);
    TEST_ASSERT_EQUAL_HEX8(0x14U, rowAddressCommands[2]);
    TEST_ASSERT_EQUAL_HEX8(0x54U, rowAddressCommands[3]);
}

void test_lcd2004_adapter_rejects_duplicate_channel_on_same_expander_but_allows_distinct_channel() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    SequentialDeviceIdSource ids(9500U);
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

    // The LCD's default eChannel is also 2 -- must collide with the switch above.
    const DeviceCreateRequest collidingLcd = makeLcdCreateRequest("lcd-bad", expanderResult.deviceId, 0U);
    const DeviceValidationResult collidingValidation = Lcd2004DeviceApiAdapter::instance().validateCreateRequest(collidingLcd, registry);
    TEST_ASSERT_FALSE(collidingValidation.ok());

    // Moving every LCD channel off of the busy channel 2 must succeed.
    DeviceCreateRequest okLcd = makeLcdCreateRequest("lcd-ok", expanderResult.deviceId, 0U);
    Lcd2004DeviceConfigV1 okConfig{};
    TEST_ASSERT_TRUE(decodeLcd2004DeviceConfig(okLcd.configBlob.data(), okLcd.configBlob.size(), okConfig));
    okConfig.channels.rsChannel = 0U;
    okConfig.channels.eChannel = 1U;
    okConfig.channels.d4Channel = 3U;
    okConfig.channels.d5Channel = 4U;
    okConfig.channels.d6Channel = 5U;
    okConfig.channels.d7Channel = 6U;
    okConfig.channels.backlightChannel = 7U;
    okLcd.configBlob = encodeLcdPayload(okConfig);
    const DeviceValidationResult okValidation = Lcd2004DeviceApiAdapter::instance().validateCreateRequest(okLcd, registry);
    TEST_ASSERT_TRUE_MESSAGE(okValidation.ok(), okValidation.message);
}
