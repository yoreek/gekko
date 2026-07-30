#include "Hd44780TestSupport.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayLayoutProfile.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/lcd1602/Lcd1602Device.h"
#include "devices/display/lcd1602/Lcd1602DeviceConfig.h"
#include "integrations/rest/lcd1602/Lcd1602DeviceApiAdapter.h"

#include <array>
#include <cstdio>
#include <string>
#include <unity.h>

using namespace ewfm;
using namespace ewfm::test_support;

namespace {

Lcd1602DeviceConfigV2 makeLcdConfig(const char* name = "lcd") {
    Lcd1602DeviceConfigV2 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeLcdPayload(const Lcd1602DeviceConfigV2& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Lcd1602DeviceConfigV2::kMagic, config, buffer, lcd1602DeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, lcd1602DeviceConfigSize(config)));
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
    topWidget.x = 0U;
    topWidget.y = 0U;
    topWidget.width = 16U;
    topWidget.height = 1U;
    std::snprintf(topWidget.text, sizeof(topWidget.text), "%s", top);

    DisplayLayoutWidgetV1 bottomWidget{};
    std::snprintf(bottomWidget.id, sizeof(bottomWidget.id), "%s", "row1");
    bottomWidget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Character);
    bottomWidget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    bottomWidget.x = 0U;
    bottomWidget.y = 1U;
    bottomWidget.width = 16U;
    bottomWidget.height = 1U;
    std::snprintf(bottomWidget.text, sizeof(bottomWidget.text), "%s", bottom);

    page.widgets.push_back(topWidget);
    page.widgets.push_back(bottomWidget);
    layout.pages.push_back(page);
    return layout;
}

void bindLcdDependency(Lcd1602Device& lcd, DeviceId lcdId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = lcdId;
    record.header.typeId = Lcd1602Device::descriptor().typeId;
    record.header.configVersion = Lcd1602Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::I2CBus, busId};
    record.status = DeviceStatus::Ready;
    lcd.bindDeviceIdentity(record, DeviceConfigBlob{});
}

void driveLcdUntilReady(Lcd1602Device& lcd, uint32_t startNow = 10U) {
    lcd.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && lcd.status() != DeviceStatus::Ready; now += 10U) {
        lcd.tick100ms(now);
    }
}

DeviceCreateRequest makeLcdCreateRequest(const char* name, DeviceId busId) {
    DeviceCreateRequest request{};
    request.typeId = Lcd1602Device::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1U;
    request.deps[0] = {DeviceRole::I2CBus, busId};
    request.configVersion = Lcd1602Device::descriptor().currentConfigVersion;
    request.configBlob = encodeLcdPayload(makeLcdConfig(name));
    return request;
}

} // namespace

void test_lcd1602_config_codec_json_and_validation() {
    const Lcd1602DeviceConfigV2 config = makeLcdConfig();
    TEST_ASSERT_TRUE(config.validate().ok());

    StaticJsonDocument<512> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_UINT8(0x27U, json["i2cAddress"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(0U, json["rsChannel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(2U, json["eChannel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(4U, json["d4Channel"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(3U, json["backlightChannel"].as<uint8_t>());
    TEST_ASSERT_FALSE(json.containsKey("layout"));

    json["rsChannel"] = 1U;
    Lcd1602DeviceConfigV2 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parsed.parseJson(json, error));
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.rsChannel);
}

void test_lcd1602_config_rejects_invalid_fields() {
    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    json["enabled"] = true;
    json["name"] = "lcd-1602-config-name-that-is-way-too-long";
    Lcd1602DeviceConfigV2 tooLong{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(tooLong.parseJson(json, error));
    TEST_ASSERT_NOT_NULL(error);

    Lcd1602DeviceConfigV2 duplicateChannels = makeLcdConfig();
    duplicateChannels.rsChannel = duplicateChannels.d4Channel; // collides with default d4Channel=4
    TEST_ASSERT_FALSE(duplicateChannels.validate().ok());

    Lcd1602DeviceConfigV2 outOfRange = makeLcdConfig();
    outOfRange.eChannel = 8U; // PCF8574 tops out at channel 7
    TEST_ASSERT_FALSE(outOfRange.validate().ok());

    Lcd1602DeviceConfigV2 badAddress = makeLcdConfig();
    badAddress.i2cAddress = 0x80U; // out of the 7-bit I2C address space
    TEST_ASSERT_FALSE(badAddress.validate().ok());
}

void test_lcd1602_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();

    const DeviceTypeDescriptor* descriptor = typeRegistry.find(Lcd1602Device::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("lcd1602"));
    TEST_ASSERT_EQUAL_STRING("lcd1602", Lcd1602DeviceApiAdapter::instance().typeName());

    bool requiresI2cBus = false;
    for (const auto& requirement : descriptor->dependencyRequirements) {
        if (requirement.role == DeviceRole::I2CBus) {
            requiresI2cBus = requirement.required;
        }
    }
    TEST_ASSERT_TRUE(requiresI2cBus);

    Lcd1602Device lcd(makeLcdConfig());
    const DisplayLayoutProfile profile = lcd.displayProfile();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayCoordinateUnit::CharacterCell), static_cast<uint8_t>(profile.coordinateUnit));
    TEST_ASSERT_EQUAL_UINT8(16U, profile.logicalWidth);
    TEST_ASSERT_EQUAL_UINT8(2U, profile.logicalHeight);
    TEST_ASSERT_EQUAL_UINT32(displayLayoutWidgetMask(DisplayLayoutWidgetType::Character), profile.supportedWidgetMask);
    TEST_ASSERT_EQUAL_UINT8(0x01U, profile.supportedRotationsMask);
    TEST_ASSERT_FALSE(profile.supportsBitmap);
    TEST_ASSERT_FALSE(profile.supportsColor);
}

void test_lcd1602_reaches_ready_through_i2c_bus_and_renders_with_diffing() {
    FakeI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Lcd1602Device lcd(makeLcdConfig());
    bindLcdDependency(lcd, 5002, bus.deviceId());
    lcd.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveLcdUntilReady(lcd);
    TEST_ASSERT_EQUAL_MESSAGE(static_cast<int>(DeviceStatus::Ready), static_cast<int>(lcd.status()),
                              "lcd1602 should reach Ready once its I2C bus dependency and HD44780 init sequence succeed");
    TEST_ASSERT_EQUAL_UINT8(0x27U, driver.lastAddress);

    FakeWifiDriver wifi;
    const MetricValueResolver resolver(nullptr, wifi, 1000U);
    lcd.setLayout(makeLcdLayout("Hello", "World"));

    const uint32_t writesBeforeFirstRender = driver.writeCount;
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 1001U));
    const uint32_t writesAfterFirstRender = driver.writeCount;
    TEST_ASSERT_TRUE_MESSAGE(writesAfterFirstRender > writesBeforeFirstRender, "first render must write both lines to hardware");

    // Re-rendering identical content must not touch hardware again.
    TEST_ASSERT_FALSE(lcd.renderText(resolver, 1002U));
    TEST_ASSERT_EQUAL_UINT32(writesAfterFirstRender, driver.writeCount);

    // Changing only row 0 in the layout must write again.
    lcd.setLayout(makeLcdLayout("Changed", "World"));
    TEST_ASSERT_TRUE(lcd.renderText(resolver, 1004U));
    TEST_ASSERT_TRUE(driver.writeCount > writesAfterFirstRender);
}

void test_lcd1602_adapter_requires_bus_dependency_and_validates_address() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    SequentialDeviceIdSource ids(9000U);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    const DeviceCreateResult busResult = registry.create(test_support::makeBusCreateRequest("bus"), 10);
    TEST_ASSERT_TRUE_MESSAGE(busResult.ok(), busResult.validation.message);

    const DeviceCreateRequest missingBus = makeLcdCreateRequest("lcd-no-bus", 0U);
    const DeviceValidationResult missingBusValidation = Lcd1602DeviceApiAdapter::instance().validateCreateRequest(missingBus, registry);
    TEST_ASSERT_FALSE(missingBusValidation.ok());

    const DeviceCreateRequest okLcd = makeLcdCreateRequest("lcd-ok", busResult.deviceId);
    const DeviceValidationResult okValidation = Lcd1602DeviceApiAdapter::instance().validateCreateRequest(okLcd, registry);
    TEST_ASSERT_TRUE_MESSAGE(okValidation.ok(), okValidation.message);
}
