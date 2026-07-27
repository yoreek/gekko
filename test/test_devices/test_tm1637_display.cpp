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

using namespace ewfm;
using namespace ewfm::test_support;

namespace {

class FakeSwitchOutput final : public DeviceRuntimeBase, public ISwitchOutputRuntime {
public:
    FakeSwitchOutput() : DeviceRuntimeBase((PState)&FakeSwitchOutput::Idle) {
        status_ = DeviceStatus::Ready;
    }

    void begin(uint32_t) override {
        status_ = DeviceStatus::Ready;
    }
    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }
    StateType currentOutputState() const override {
        return state_;
    }
    bool requestOutputState(StateType state, uint32_t) override {
        ++writeCount;
        state_ = state;
        return true;
    }
    const ISwitchOutputRuntime* switchOutputRuntime() const override {
        return this;
    }

    uint32_t writeCount{0U};

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }

    StateType state_{false};
};

Tm1637DeviceConfigV1 makeConfig(const char* name = "tm1637", uint8_t brightness = 7U, uint8_t rotation = 0U) {
    Tm1637DeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    config.panel = static_cast<uint8_t>(Tm1637PanelKind::FourDigitDecimal036);
    config.brightness = brightness;
    config.rotation = rotation;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodePayload(const Tm1637DeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Tm1637DeviceConfigV1::kMagic, config, buffer, tm1637DeviceConfigSize(config)));
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

void bindTm1637Identity(Tm1637Device& device, DeviceId deviceId, DeviceId clkId, DeviceId dioId) {
    const Tm1637DeviceConfigV1& config = device.config();
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodePayload(config);

    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = Tm1637Device::descriptor().typeId;
    record.header.configVersion = Tm1637Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(payload.size());
    record.depCount = 2U;
    record.deps[0] = {DeviceRole::Switch, clkId};
    record.deps[1] = {DeviceRole::Switch, dioId};
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, payload);
}

void driveTm1637UntilReady(Tm1637Device& device, uint32_t startNow = 10U) {
    device.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && device.status() != DeviceStatus::Ready; ++now) {
        device.tick100ms(now);
    }
}

} // namespace

void test_tm1637_config_codec_json_and_validation() {
    const Tm1637DeviceConfigV1 config = makeConfig("digits", 5U, 180U);
    TEST_ASSERT_TRUE(config.validate().ok());

    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_STRING("digits", json["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("four_digit_decimal_036", json["panel"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(5U, json["brightness"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(180U, json["rotation"].as<uint8_t>());

    Tm1637DeviceConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parsed.parseJson(json, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_UINT8(config.brightness, parsed.brightness);
    TEST_ASSERT_EQUAL_UINT8(config.rotation, parsed.rotation);
}

void test_tm1637_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();

    const DeviceTypeDescriptor* descriptor = typeRegistry.find(Tm1637Device::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("Tm1637Device", descriptor->name);
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("tm1637"));
    TEST_ASSERT_EQUAL_STRING("tm1637", Tm1637DeviceApiAdapter::instance().typeName());

    Tm1637Device device(makeConfig());
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
    FakeSwitchOutput clk;
    FakeSwitchOutput dio;

    Tm1637Device device(makeConfig("tm1637", 4U, 180U));
    bindTm1637Identity(device, 7101U, 7102U, 7103U);
    DeviceRuntimeBase& runtime = device;
    runtime.setDependencyRuntimeAt(0U, &clk);
    runtime.setDependencyRuntimeAt(1U, &dio);
    driveTm1637UntilReady(device);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));

    device.setLayout(makeDigitalLayout("12.34"));
    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi, 1000U);

    TEST_ASSERT_TRUE(device.renderDisplay(resolver, 1000U));
    TEST_ASSERT_TRUE(clk.writeCount > 0U);
    TEST_ASSERT_TRUE(dio.writeCount > 0U);

    const uint32_t clkWrites = clk.writeCount;
    const uint32_t dioWrites = dio.writeCount;
    TEST_ASSERT_FALSE(device.renderDisplay(resolver, 1001U));
    TEST_ASSERT_EQUAL_UINT32(clkWrites, clk.writeCount);
    TEST_ASSERT_EQUAL_UINT32(dioWrites, dio.writeCount);
}

void test_tm1637_config_rejects_invalid_fields() {
    Tm1637DeviceConfigV1 config = makeConfig();
    config.brightness = 8U;
    TEST_ASSERT_FALSE(config.validate().ok());

    config = makeConfig();
    config.rotation = 90U;
    TEST_ASSERT_FALSE(config.validate().ok());
}
