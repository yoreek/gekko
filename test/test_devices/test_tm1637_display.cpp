#include "Hd44780TestSupport.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayLayoutProfile.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/DisplayTextEvaluator.h"
#include "devices/display/tm1637/Tm1637Device.h"
#include "devices/display/tm1637/Tm1637DeviceConfig.h"
#include "devices/display/tm1637/Tm1637SegmentCodec.h"
#include "integrations/rest/tm1637/Tm1637DeviceApiAdapter.h"

#include <array>
#include <cstdio>
#include <string>
#include <unity.h>
#include <vector>

using namespace ewfm;
using namespace ewfm::test_support;

namespace {

constexpr uint8_t kTestClkPin = 18U;
constexpr uint8_t kTestDioPin = 19U;

// Decodes the bit-banged wire instead of merely counting edges: a frame that skips the ACK clock or
// keeps driving DIO through it looks identical to a correct one when only writes are counted.
class FakeTm1637Bus final : public ITm1637LineDriver {
public:
    bool configure(uint8_t clkPin, uint8_t dioPin) override {
        clkPin_ = clkPin;
        dioPin_ = dioPin;
        configureCount++;
        return configureResult;
    }

    bool setClock(uint8_t clkPin, bool high) override {
        TEST_ASSERT_EQUAL_UINT8(clkPin_, clkPin);
        if (high && !clockHigh_) {
            sampleRisingEdge();
        }
        clockHigh_ = high;
        return true;
    }

    bool driveData(uint8_t dioPin, bool high) override {
        TEST_ASSERT_EQUAL_UINT8(dioPin_, dioPin);
        if (clockHigh_ && dataHigh_ && !high) {
            started_ = true;
            bitCount_ = 0U;
            value_ = 0U;
            ++startCount;
        } else if (clockHigh_ && !dataHigh_ && high) {
            started_ = false;
            ++stopCount;
        }
        dataReleased_ = false;
        dataHigh_ = high;
        return true;
    }

    bool releaseData(uint8_t dioPin) override {
        TEST_ASSERT_EQUAL_UINT8(dioPin_, dioPin);
        dataReleased_ = true;
        // Idle-high line, unless the chip is acknowledging.
        dataHigh_ = true;
        return true;
    }

    bool readData(uint8_t dioPin, bool& level) override {
        TEST_ASSERT_EQUAL_UINT8(dioPin_, dioPin);
        level = !chipAcknowledges;
        return true;
    }

    void waitMicros(uint32_t microseconds) override {
        totalWaitMicros += microseconds;
    }

    void release(uint8_t clkPin, uint8_t dioPin) override {
        TEST_ASSERT_EQUAL_UINT8(clkPin_, clkPin);
        TEST_ASSERT_EQUAL_UINT8(dioPin_, dioPin);
        ++releaseCount;
    }

    std::vector<uint8_t> bytes{};
    uint32_t startCount{0U};
    uint32_t stopCount{0U};
    uint32_t ackClocks{0U};
    uint32_t ackClocksWithDataDriven{0U};
    uint32_t configureCount{0U};
    uint32_t releaseCount{0U};
    uint32_t totalWaitMicros{0U};
    bool configureResult{true};
    bool chipAcknowledges{true};

private:
    void sampleRisingEdge() {
        if (!started_) {
            return;
        }
        if (bitCount_ < 8U) {
            if (dataHigh_) {
                value_ = static_cast<uint8_t>(value_ | (1U << bitCount_));
            }
            ++bitCount_;
            return;
        }
        ++ackClocks;
        if (!dataReleased_) {
            ++ackClocksWithDataDriven;
        }
        bytes.push_back(value_);
        value_ = 0U;
        bitCount_ = 0U;
    }

    uint8_t clkPin_{0xFFU};
    uint8_t dioPin_{0xFFU};
    bool clockHigh_{false};
    bool dataHigh_{true};
    bool dataReleased_{false};
    bool started_{false};
    uint8_t bitCount_{0U};
    uint8_t value_{0U};
};

Tm1637DeviceConfigV2 makeConfig(const char* name = "tm1637", uint8_t brightness = 7U, uint8_t rotation = 0U) {
    Tm1637DeviceConfigV2 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    config.panel = static_cast<uint8_t>(Tm1637PanelKind::FourDigitDecimal036);
    config.brightness = brightness;
    config.rotation = rotation;
    config.clkPin = kTestClkPin;
    config.dioPin = kTestDioPin;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodePayload(const Tm1637DeviceConfigV2& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Tm1637DeviceConfigV2::kMagic, config, buffer, tm1637DeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, tm1637DeviceConfigSize(config)));
    return payload;
}

DisplayLayoutRecordV1 makeDigitalLayout(const char* text) {
    DisplayLayoutRecordV1 layout{};
    layout.activePageIndex = 0U;
    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");

    DisplayLayoutWidgetV1 widget{};
    std::snprintf(widget.id, sizeof(widget.id), "%s", "digital");
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Digital);
    widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    widget.x = 0U;
    widget.y = 0U;
    widget.width = 4U;
    widget.height = 1U;
    widget.digitalAlign = static_cast<uint8_t>(DisplayDigitalAlign::Right);
    std::snprintf(widget.text, sizeof(widget.text), "%s", text);
    page.widgets.push_back(widget);
    layout.pages.push_back(page);
    return layout;
}

void bindTm1637Identity(Tm1637Device& device, DeviceId deviceId) {
    const Tm1637DeviceConfigV2& config = device.config();
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodePayload(config);

    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = Tm1637Device::descriptor().typeId;
    record.header.configVersion = Tm1637Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(payload.size());
    record.depCount = 0U;
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, payload);
}

void driveTm1637Until(Tm1637Device& device, DeviceStatus expected, uint32_t startNow = 10U) {
    device.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && device.status() != expected; ++now) {
        device.tick100ms(now);
    }
}

} // namespace

void test_tm1637_config_codec_json_and_validation() {
    const Tm1637DeviceConfigV2 config = makeConfig("digits", 5U, 180U);
    TEST_ASSERT_TRUE(config.validate().ok());

    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_STRING("digits", json["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("four_digit_decimal_036", json["panel"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(5U, json["brightness"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(180U, json["rotation"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(kTestClkPin, json["clkPin"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(kTestDioPin, json["dioPin"].as<uint8_t>());

    Tm1637DeviceConfigV2 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parsed.parseJson(json, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_UINT8(config.brightness, parsed.brightness);
    TEST_ASSERT_EQUAL_UINT8(config.rotation, parsed.rotation);
    TEST_ASSERT_EQUAL_UINT8(config.clkPin, parsed.clkPin);
    TEST_ASSERT_EQUAL_UINT8(config.dioPin, parsed.dioPin);
}

void test_tm1637_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();

    const DeviceTypeDescriptor* descriptor = typeRegistry.find(Tm1637Device::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("Tm1637Device", descriptor->name);
    TEST_ASSERT_EQUAL_UINT32(2U, descriptor->currentConfigVersion);
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("tm1637"));
    TEST_ASSERT_EQUAL_STRING("tm1637", Tm1637DeviceApiAdapter::instance().typeName());

    FakeTm1637Bus bus;
    Tm1637Device device(makeConfig(), bus);
    const DisplayLayoutProfile profile = device.displayProfile();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayCoordinateUnit::DigitCell), static_cast<uint8_t>(profile.coordinateUnit));
    TEST_ASSERT_EQUAL_UINT8(4U, profile.logicalWidth);
    TEST_ASSERT_EQUAL_UINT8(1U, profile.logicalHeight);
    TEST_ASSERT_EQUAL_UINT32(displayLayoutWidgetMask(DisplayLayoutWidgetType::Digital), profile.supportedWidgetMask);
    TEST_ASSERT_EQUAL_UINT8(0x05U, profile.supportedRotationsMask);
    TEST_ASSERT_FALSE(profile.supportsBitmap);
    TEST_ASSERT_FALSE(profile.supportsColor);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayAuxSegmentMode::PerDigitDecimalPoint),
                            static_cast<uint8_t>(profile.auxSegmentMode));
}

void test_tm1637_segment_codec_applies_decimal_point_and_rotation() {
    DisplayDigitalFrame frame{};
    frame.cellCount = 1U;
    frame.cells[0].glyph = '1';
    frame.cells[0].decimalPoint = 1U;

    std::array<uint8_t, Tm1637SegmentCodec::kDigitCount> bytes0{};
    std::array<uint8_t, Tm1637SegmentCodec::kDigitCount> bytes180{};
    TEST_ASSERT_TRUE(Tm1637SegmentCodec::encode(frame, 0U, bytes0.data(), bytes0.size()));
    TEST_ASSERT_TRUE(Tm1637SegmentCodec::encode(frame, 180U, bytes180.data(), bytes180.size()));

    TEST_ASSERT_EQUAL_HEX8(0x86U, bytes0[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, bytes180[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, bytes0[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, bytes180[1]);
    TEST_ASSERT_EQUAL_HEX8(0xB0U, bytes180[3]);
}

void test_tm1637_device_renders_digital_layout_and_diffing() {
    FakeTm1637Bus bus;
    Tm1637Device device(makeConfig("tm1637", 4U, 0U), bus);
    bindTm1637Identity(device, 7101U);
    driveTm1637Until(device, DeviceStatus::Ready);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT32(1U, bus.configureCount);

    device.setLayout(makeDigitalLayout("12.34"));
    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi, 1000U);

    TEST_ASSERT_TRUE(device.renderDisplay(resolver, 1000U));

    // Data command, address command plus four digits, then display-on with brightness 4.
    TEST_ASSERT_EQUAL_UINT32(7U, static_cast<uint32_t>(bus.bytes.size()));
    TEST_ASSERT_EQUAL_HEX8(0x40U, bus.bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0xC0U, bus.bytes[1]);
    TEST_ASSERT_EQUAL_HEX8(0x06U, bus.bytes[2]); // '1'
    TEST_ASSERT_EQUAL_HEX8(0xDBU, bus.bytes[3]); // '2' with decimal point
    TEST_ASSERT_EQUAL_HEX8(0x4FU, bus.bytes[4]); // '3'
    TEST_ASSERT_EQUAL_HEX8(0x66U, bus.bytes[5]); // '4'
    TEST_ASSERT_EQUAL_HEX8(0x8CU, bus.bytes[6]);

    // Every byte must carry its own ACK clock, and DIO must be released while it happens.
    TEST_ASSERT_EQUAL_UINT32(7U, bus.ackClocks);
    TEST_ASSERT_EQUAL_UINT32(0U, bus.ackClocksWithDataDriven);
    TEST_ASSERT_EQUAL_UINT32(3U, bus.startCount);
    TEST_ASSERT_EQUAL_UINT32(3U, bus.stopCount);
    TEST_ASSERT_TRUE(bus.totalWaitMicros > 0U);

    const size_t bytesWritten = bus.bytes.size();
    TEST_ASSERT_FALSE(device.renderDisplay(resolver, 1001U));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(bytesWritten), static_cast<uint32_t>(bus.bytes.size()));
}

void test_tm1637_unacknowledged_frame_retries_next_tick() {
    FakeTm1637Bus bus;
    bus.chipAcknowledges = false;
    Tm1637Device device(makeConfig(), bus);
    bindTm1637Identity(device, 7104U);
    driveTm1637Until(device, DeviceStatus::Ready);

    device.setLayout(makeDigitalLayout("42"));
    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi, 1000U);

    TEST_ASSERT_FALSE(device.renderDisplay(resolver, 1000U));
    const size_t firstAttempt = bus.bytes.size();
    TEST_ASSERT_TRUE(firstAttempt > 0U);

    // The cache must not latch a frame the chip never confirmed.
    TEST_ASSERT_FALSE(device.renderDisplay(resolver, 1100U));
    TEST_ASSERT_TRUE(bus.bytes.size() > firstAttempt);
}

void test_tm1637_migrated_config_without_pins_faults() {
    EWFM_LEGACY_CONFIG_USE_BEGIN
    Tm1637DeviceConfigV1 legacy{};
    legacy.enabled = 1U;
    std::snprintf(legacy.name, sizeof(legacy.name), "%s", "legacy");
    legacy.panel = static_cast<uint8_t>(Tm1637PanelKind::FourDigitDecimal036);
    legacy.brightness = 3U;
    legacy.rotation = 180U;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = tm1637DeviceConfigSize(legacy);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Tm1637DeviceConfigV1::kMagic, legacy, buffer, size));
    EWFM_LEGACY_CONFIG_USE_END

    Tm1637DeviceConfigV2 migrated{};
    TEST_ASSERT_TRUE(decodeTm1637DeviceConfig(buffer, size, migrated));
    TEST_ASSERT_EQUAL_STRING("legacy", migrated.name);
    TEST_ASSERT_EQUAL_UINT8(3U, migrated.brightness);
    TEST_ASSERT_EQUAL_UINT8(180U, migrated.rotation);
    TEST_ASSERT_EQUAL_UINT8(kTm1637UnsetPin, migrated.clkPin);
    TEST_ASSERT_EQUAL_UINT8(kTm1637UnsetPin, migrated.dioPin);
    TEST_ASSERT_FALSE(migrated.pinsConfigured());
    TEST_ASSERT_TRUE(migrated.validate().ok());

    FakeTm1637Bus bus;
    Tm1637Device device(migrated, bus);
    bindTm1637Identity(device, 7105U);
    driveTm1637Until(device, DeviceStatus::Faulted);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT32(0U, bus.configureCount);

    // REST must never accept the unset pair back.
    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    migrated.writeJson(json);
    Tm1637DeviceConfigV2 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(parsed.parseJson(json, error));
    TEST_ASSERT_NOT_NULL(error);
}

void test_tm1637_config_rejects_invalid_fields() {
    Tm1637DeviceConfigV2 config = makeConfig();
    config.brightness = 8U;
    TEST_ASSERT_FALSE(config.validate().ok());

    config = makeConfig();
    config.rotation = 90U;
    TEST_ASSERT_FALSE(config.validate().ok());

    config = makeConfig();
    config.dioPin = config.clkPin;
    TEST_ASSERT_FALSE(config.validate().ok());

    config = makeConfig();
    config.clkPin = 6U; // flash pin
    TEST_ASSERT_FALSE(config.validate().ok());
}
