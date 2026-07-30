#include "Hd44780TestSupport.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayLayoutProfile.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/lcd1602_pin/Lcd1602PinDevice.h"
#include "devices/display/lcd1602_pin/Lcd1602PinDeviceConfig.h"
#include "integrations/rest/lcd1602_pin/Lcd1602PinDeviceApiAdapter.h"

#include <array>
#include <cstdio>
#include <string>
#include <unity.h>
#include <vector>

using namespace ewfm;
using namespace ewfm::test_support;

namespace {

constexpr uint8_t kRsPin = 12U;
constexpr uint8_t kEPin = 13U;
constexpr uint8_t kD4Pin = 14U;
constexpr uint8_t kD5Pin = 15U;
constexpr uint8_t kD6Pin = 16U;
constexpr uint8_t kD7Pin = 17U;
constexpr uint8_t kBacklightPin = 18U;

// Decodes the actual nibble/byte sequence latched on the E line's falling edge, the same way
// FakeTm1637Bus decodes the bit-banged TM1637 wire: proves the HD44780 protocol wiring is correct
// rather than merely counting driver calls.
class FakeHd44780PinLineDriver final : public IHd44780PinLineDriver {
public:
    bool configure(const uint8_t* pins, const uint8_t count) override {
        const uint8_t copyCount = count > 7U ? 7U : count;
        for (uint8_t index = 0U; index < copyCount; ++index) {
            pins_[index] = pins[index];
        }
        ++configureCount;
        return configureResult;
    }

    bool setLine(const uint8_t lineIndex, const bool level) override {
        if (lineIndex < 7U) {
            levels_[lineIndex] = level;
        }
        if (lineIndex == kHd44780LineE && lastE_ && !level) {
            latchNibble(levels_[kHd44780LineRs],
                        static_cast<uint8_t>((levels_[kHd44780LineD4] ? 0x1U : 0U) | (levels_[kHd44780LineD5] ? 0x2U : 0U) |
                                             (levels_[kHd44780LineD6] ? 0x4U : 0U) | (levels_[kHd44780LineD7] ? 0x8U : 0U)));
        }
        if (lineIndex == kHd44780LineE) {
            lastE_ = level;
        }
        ++setLineCount;
        return true;
    }

    void release(const uint8_t*, uint8_t) override {
        ++releaseCount;
    }

    // Discards any pending half-latched nibble and the decoded byte log, so tests can decode only
    // the writes that happen after this point (e.g. skip the HD44780 init sequence's unpaired
    // 4-bit-mode nibbles).
    void resetDecoder() {
        pendingHighNibble_ = false;
        bytes.clear();
        byteIsData.clear();
    }

    std::vector<uint8_t> bytes;
    std::vector<bool> byteIsData;
    uint32_t configureCount{0U};
    uint32_t setLineCount{0U};
    uint32_t releaseCount{0U};
    bool configureResult{true};

private:
    void latchNibble(const bool rs, const uint8_t nibble) {
        if (!pendingHighNibble_) {
            pendingHigh_ = nibble;
            pendingRs_ = rs;
            pendingHighNibble_ = true;
            return;
        }
        bytes.push_back(static_cast<uint8_t>((pendingHigh_ << 4U) | nibble));
        byteIsData.push_back(pendingRs_);
        pendingHighNibble_ = false;
    }

    uint8_t pins_[7]{};
    bool levels_[7]{};
    bool lastE_{false};
    bool pendingHighNibble_{false};
    uint8_t pendingHigh_{0U};
    bool pendingRs_{false};
};

Lcd1602PinDeviceConfigV1 makeLcdConfig(const char* name = "lcd-pin") {
    Lcd1602PinDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    config.rsPin = kRsPin;
    config.ePin = kEPin;
    config.d4Pin = kD4Pin;
    config.d5Pin = kD5Pin;
    config.d6Pin = kD6Pin;
    config.d7Pin = kD7Pin;
    config.backlightPin = kBacklightPin;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeLcdPayload(const Lcd1602PinDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Lcd1602PinDeviceConfigV1::kMagic, config, buffer, lcd1602PinDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, lcd1602PinDeviceConfigSize(config)));
    return payload;
}

DisplayLayoutRecordV1 makeLcdLayout(const char* top, const char* bottom) {
    DisplayLayoutRecordV1 layout{};
    layout.activePageIndex = 0U;
    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");

    DisplayLayoutWidgetV1 topWidget{};
    std::snprintf(topWidget.id, sizeof(topWidget.id), "%s", "row0");
    topWidget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Character);
    topWidget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    topWidget.width = 16U;
    topWidget.height = 1U;
    std::snprintf(topWidget.text, sizeof(topWidget.text), "%s", top);

    DisplayLayoutWidgetV1 bottomWidget{};
    std::snprintf(bottomWidget.id, sizeof(bottomWidget.id), "%s", "row1");
    bottomWidget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Character);
    bottomWidget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    bottomWidget.y = 1U;
    bottomWidget.width = 16U;
    bottomWidget.height = 1U;
    std::snprintf(bottomWidget.text, sizeof(bottomWidget.text), "%s", bottom);

    page.widgets.push_back(topWidget);
    page.widgets.push_back(bottomWidget);
    layout.pages.push_back(page);
    return layout;
}

void bindLcdIdentity(Lcd1602PinDevice& lcd, DeviceId lcdId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = lcdId;
    record.header.typeId = Lcd1602PinDevice::descriptor().typeId;
    record.header.configVersion = Lcd1602PinDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.status = DeviceStatus::Ready;
    lcd.bindDeviceIdentity(record, encodeLcdPayload(lcd.config()));
}

void driveLcdUntilReady(Lcd1602PinDevice& lcd, uint32_t startNow = 10U) {
    lcd.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && lcd.status() != DeviceStatus::Ready; now += 10U) {
        lcd.tick100ms(now);
    }
}

} // namespace

void test_lcd1602_pin_config_codec_json_and_validation() {
    const Lcd1602PinDeviceConfigV1 config = makeLcdConfig();
    TEST_ASSERT_TRUE(config.validate().ok());

    StaticJsonDocument<512> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_UINT8(kRsPin, json["rsPin"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(kD4Pin, json["d4Pin"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(kBacklightPin, json["backlightPin"].as<uint8_t>());

    Lcd1602PinDeviceConfigV1 duplicatePins = makeLcdConfig();
    duplicatePins.rsPin = duplicatePins.d4Pin;
    TEST_ASSERT_FALSE(duplicatePins.validate().ok());

    Lcd1602PinDeviceConfigV1 invalidPin = makeLcdConfig();
    invalidPin.ePin = 6U; // flash pin, never a valid GPIO
    TEST_ASSERT_FALSE(invalidPin.validate().ok());
}

void test_lcd1602_pin_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();

    const DeviceTypeDescriptor* descriptor = typeRegistry.find(Lcd1602PinDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("lcd1602_pin"));
    TEST_ASSERT_EQUAL_STRING("lcd1602_pin", Lcd1602PinDeviceApiAdapter::instance().typeName());

    for (const auto& requirement : descriptor->dependencyRequirements) {
        TEST_ASSERT_FALSE_MESSAGE(requirement.role == DeviceRole::I2CBus, "pin variant must not require an I2C bus dependency");
    }

    Lcd1602PinDevice lcd(makeLcdConfig());
    const DisplayLayoutProfile profile = lcd.displayProfile();
    TEST_ASSERT_EQUAL_UINT8(16U, profile.logicalWidth);
    TEST_ASSERT_EQUAL_UINT8(2U, profile.logicalHeight);
}

void test_lcd1602_pin_reaches_ready_without_dependency_and_renders_correct_bytes() {
    FakeHd44780PinLineDriver driver;
    Lcd1602PinDevice lcd(makeLcdConfig(), driver);
    bindLcdIdentity(lcd, 5201U);
    driveLcdUntilReady(lcd);
    TEST_ASSERT_EQUAL_MESSAGE(static_cast<int>(DeviceStatus::Ready), static_cast<int>(lcd.status()),
                              "lcd1602_pin should reach Ready with no dependency at all once the HD44780 init sequence succeeds");
    TEST_ASSERT_EQUAL_UINT32(1U, driver.configureCount);

    driver.resetDecoder();
    FakeWifiDriver wifi;
    const MetricValueResolver resolver(nullptr, wifi, 1000U);
    lcd.setLayout(makeLcdLayout("Hi", "Bye"));
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 1001U));

    // Row 0: DDRAM address command (rs=0) followed by 16 data bytes ('H','i', then 14 spaces).
    TEST_ASSERT_TRUE(driver.bytes.size() >= 17U);
    TEST_ASSERT_FALSE(driver.byteIsData[0]);
    TEST_ASSERT_TRUE(driver.byteIsData[1]);
    TEST_ASSERT_EQUAL_HEX8('H', driver.bytes[1]);
    TEST_ASSERT_EQUAL_HEX8('i', driver.bytes[2]);
    TEST_ASSERT_EQUAL_HEX8(' ', driver.bytes[3]);

    // Re-rendering identical content must not touch hardware again.
    const size_t bytesAfterFirstRender = driver.bytes.size();
    TEST_ASSERT_FALSE(lcd.renderText(resolver, 1002U));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(bytesAfterFirstRender), static_cast<uint32_t>(driver.bytes.size()));
}

void test_lcd1602_pin_migrated_config_without_pins_faults() {
    Lcd1602PinDeviceConfigV1 unset{};
    unset.enabled = 1U;
    std::snprintf(unset.name, sizeof(unset.name), "%s", "unset");
    TEST_ASSERT_FALSE(unset.validate().ok());
}
