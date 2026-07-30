#include "Hd44780TestSupport.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayLayoutProfile.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/lcd2004_pin/Lcd2004PinDevice.h"
#include "devices/display/lcd2004_pin/Lcd2004PinDeviceConfig.h"
#include "integrations/rest/lcd2004_pin/Lcd2004PinDeviceApiAdapter.h"

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

// Same nibble/byte decoder as test_lcd1602_pin.cpp's FakeHd44780PinLineDriver -- duplicated locally
// since each Unity test binary translation unit compiles independently.
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

Lcd2004PinDeviceConfigV1 makeLcdConfig(const char* name = "lcd-pin") {
    Lcd2004PinDeviceConfigV1 config{};
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

BoundedBlob<kMaxDeviceConfigBytes> encodeLcdPayload(const Lcd2004PinDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Lcd2004PinDeviceConfigV1::kMagic, config, buffer, lcd2004PinDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, lcd2004PinDeviceConfigSize(config)));
    return payload;
}

DisplayLayoutRecordV1 makeLcdLayout(const char* row0) {
    DisplayLayoutRecordV1 layout{};
    layout.activePageIndex = 0U;
    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");

    DisplayLayoutWidgetV1 widget{};
    std::snprintf(widget.id, sizeof(widget.id), "%s", "row0");
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Character);
    widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    widget.width = 20U;
    widget.height = 1U;
    std::snprintf(widget.text, sizeof(widget.text), "%s", row0);

    page.widgets.push_back(widget);
    layout.pages.push_back(page);
    return layout;
}

void bindLcdIdentity(Lcd2004PinDevice& lcd, DeviceId lcdId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = lcdId;
    record.header.typeId = Lcd2004PinDevice::descriptor().typeId;
    record.header.configVersion = Lcd2004PinDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.status = DeviceStatus::Ready;
    lcd.bindDeviceIdentity(record, encodeLcdPayload(lcd.config()));
}

void driveLcdUntilReady(Lcd2004PinDevice& lcd, uint32_t startNow = 10U) {
    lcd.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && lcd.status() != DeviceStatus::Ready; now += 10U) {
        lcd.tick100ms(now);
    }
}

} // namespace

void test_lcd2004_pin_config_codec_json_and_validation() {
    const Lcd2004PinDeviceConfigV1 config = makeLcdConfig();
    TEST_ASSERT_TRUE(config.validate().ok());

    StaticJsonDocument<512> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_UINT8(kRsPin, json["rsPin"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(kBacklightPin, json["backlightPin"].as<uint8_t>());

    Lcd2004PinDeviceConfigV1 duplicatePins = makeLcdConfig();
    duplicatePins.rsPin = duplicatePins.d4Pin;
    TEST_ASSERT_FALSE(duplicatePins.validate().ok());
}

void test_lcd2004_pin_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();

    const DeviceTypeDescriptor* descriptor = typeRegistry.find(Lcd2004PinDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("lcd2004_pin"));
    TEST_ASSERT_EQUAL_STRING("lcd2004_pin", Lcd2004PinDeviceApiAdapter::instance().typeName());

    Lcd2004PinDevice lcd(makeLcdConfig());
    const DisplayLayoutProfile profile = lcd.displayProfile();
    TEST_ASSERT_EQUAL_UINT8(20U, profile.logicalWidth);
    TEST_ASSERT_EQUAL_UINT8(4U, profile.logicalHeight);
}

// The one genuinely new piece of protocol logic lcd2004 adds over lcd1602: the DDRAM set-address
// command for rows 2/3 uses a half-offset by the display's own column count
// (Hd44780CharacterDisplayDeviceBase::rowAddress()), not just the usual row%2 base address.
void test_lcd2004_pin_sends_correct_ddram_row_addresses() {
    FakeHd44780PinLineDriver driver;
    Lcd2004PinDevice lcd(makeLcdConfig(), driver);
    bindLcdIdentity(lcd, 5302U);
    driveLcdUntilReady(lcd);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(lcd.status()));

    driver.resetDecoder();
    FakeWifiDriver wifi;
    const MetricValueResolver resolver(nullptr, wifi, 1000U);
    DisplayLayoutRecordV1 layout{};
    layout.activePageIndex = 0U;
    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");
    for (uint8_t row = 0U; row < 4U; ++row) {
        DisplayLayoutWidgetV1 widget{};
        std::snprintf(widget.id, sizeof(widget.id), "row%u", static_cast<unsigned>(row));
        widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Character);
        widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
        widget.y = row;
        widget.width = 20U;
        widget.height = 1U;
        std::snprintf(widget.text, sizeof(widget.text), "r%u", static_cast<unsigned>(row));
        page.widgets.push_back(widget);
    }
    layout.pages.push_back(page);
    lcd.setLayout(layout);
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 2001U));

    // Each row writes a 0x80|address command followed by 20 data bytes -- the command is always
    // the first byte of its 21-byte group.
    TEST_ASSERT_TRUE(driver.bytes.size() >= 4U * 21U);
    TEST_ASSERT_EQUAL_HEX8(0x80U, driver.bytes[0U * 21U]); // row 0: base 0x00
    TEST_ASSERT_EQUAL_HEX8(0xC0U, driver.bytes[1U * 21U]); // row 1: base 0x40
    TEST_ASSERT_EQUAL_HEX8(0x94U, driver.bytes[2U * 21U]); // row 2: base 0x00 + column offset 20
    TEST_ASSERT_EQUAL_HEX8(0xD4U, driver.bytes[3U * 21U]); // row 3: base 0x40 + column offset 20
    TEST_ASSERT_FALSE(driver.byteIsData[0U * 21U]);
}

void test_lcd2004_pin_reaches_ready_without_dependency_and_renders_correct_bytes() {
    FakeHd44780PinLineDriver driver;
    Lcd2004PinDevice lcd(makeLcdConfig(), driver);
    bindLcdIdentity(lcd, 5301U);
    driveLcdUntilReady(lcd);
    TEST_ASSERT_EQUAL_MESSAGE(static_cast<int>(DeviceStatus::Ready), static_cast<int>(lcd.status()),
                              "lcd2004_pin should reach Ready with no dependency at all once the HD44780 init sequence succeeds");
    TEST_ASSERT_EQUAL_UINT32(1U, driver.configureCount);

    driver.resetDecoder();
    FakeWifiDriver wifi;
    const MetricValueResolver resolver(nullptr, wifi, 1000U);
    lcd.setLayout(makeLcdLayout("Aquarium"));
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 1001U));

    TEST_ASSERT_TRUE(driver.bytes.size() >= 21U);
    TEST_ASSERT_FALSE(driver.byteIsData[0]);
    TEST_ASSERT_TRUE(driver.byteIsData[1]);
    TEST_ASSERT_EQUAL_HEX8('A', driver.bytes[1]);
    TEST_ASSERT_EQUAL_HEX8('q', driver.bytes[2]);
}
