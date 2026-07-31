#include "JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceTypes.h"
#include "devices/pixel/PixelStripDevice.h"
#include "devices/pixel/PixelStripDeviceConfig.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/pixel_strip/PixelStripDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <string>
#include <unity.h>

using namespace ewfm;

namespace {

void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}

PixelStripDeviceConfigV1 makeConfig() {
    PixelStripDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "pixel-strip");
    config.pin = 5U;
    config.pixelCount = 12U;
    config.restorePreviousState = false;
    config.startupBrightness = 128U;
    return config;
}

DeviceConfigBlob encodeConfig(const PixelStripDeviceConfigV1& config) {
    DeviceConfigBlob blob{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = pixelStripDeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(PixelStripDeviceConfigV1::kMagic, config, buffer, size));
    TEST_ASSERT_TRUE(blob.assign(buffer, size));
    return blob;
}

void bindPixelStripIdentity(PixelStripDevice& device, DeviceId deviceId) {
    const DeviceConfigBlob blob = encodeConfig(device.config());
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = PixelStripDevice::descriptor().typeId;
    record.header.configVersion = PixelStripDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, blob);
}

} // namespace

void test_pixel_strip_brightness_percent_helpers_round_trip() {
    TEST_ASSERT_EQUAL_UINT8(0U, percentToPixelBrightness(0U));
    TEST_ASSERT_EQUAL_UINT8(kPixelBrightnessMax, percentToPixelBrightness(100U));
    TEST_ASSERT_EQUAL_UINT8(100U, pixelBrightnessToPercent(kPixelBrightnessMax));
    TEST_ASSERT_EQUAL_UINT8(0U, pixelBrightnessToPercent(0U));
    TEST_ASSERT_EQUAL_UINT8(50U, pixelBrightnessToPercent(percentToPixelBrightness(50U)));
}

void test_pixel_strip_config_round_trip_and_validation() {
    const PixelStripDeviceConfigV1 config = makeConfig();
    const DeviceConfigBlob blob = encodeConfig(config);

    PixelStripDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodePixelStripDeviceConfig(blob.data(), blob.size(), decoded));
    TEST_ASSERT_TRUE(decoded.validate().ok());
    TEST_ASSERT_EQUAL_UINT8(config.pin, decoded.pin);
    TEST_ASSERT_EQUAL_UINT16(config.pixelCount, decoded.pixelCount);
    TEST_ASSERT_EQUAL_UINT8(config.startupBrightness, decoded.startupBrightness);

    StaticJsonDocument<256> json;
    decoded.writeJson(json.to<JsonObject>());
    assertMatchesJsonSchema("schemas/rest/v1/devices/pixel_strip.config.schema.json", json.as<JsonVariantConst>());
}

void test_pixel_strip_config_rejects_pixel_count_out_of_bounds() {
    PixelStripDeviceConfigV1 config = makeConfig();
    config.pixelCount = 0U;
    TEST_ASSERT_FALSE(config.validate().ok());

    config.pixelCount = static_cast<uint16_t>(kMaxPixelStripLength + 1U);
    TEST_ASSERT_FALSE(config.validate().ok());
}

void test_pixel_strip_buffer_set_fill_and_read_back_are_bounded() {
    PixelStripDevice device(makeConfig());
    bindPixelStripIdentity(device, 100U);
    device.begin(10U);
    device.tick100ms(11U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT16(12U, device.pixelCount());

    TEST_ASSERT_TRUE(device.setPixel(0, PixelColor{10, 20, 30}));
    TEST_ASSERT_FALSE(device.setPixel(12, PixelColor{1, 2, 3})); // out of the configured pixelCount
    const PixelColor read = device.currentPixel(0);
    TEST_ASSERT_EQUAL_UINT8(10U, read.r);
    TEST_ASSERT_EQUAL_UINT8(20U, read.g);
    TEST_ASSERT_EQUAL_UINT8(30U, read.b);

    TEST_ASSERT_TRUE(device.fill(PixelColor{1, 2, 3}));
    for (uint16_t index = 0; index < device.pixelCount(); ++index) {
        const PixelColor color = device.currentPixel(index);
        TEST_ASSERT_EQUAL_UINT8(1U, color.r);
        TEST_ASSERT_EQUAL_UINT8(2U, color.g);
        TEST_ASSERT_EQUAL_UINT8(3U, color.b);
    }
}

void test_pixel_strip_show_fails_before_hardware_is_ready() {
    PixelStripDevice device(makeConfig());
    TEST_ASSERT_FALSE(device.show(10U));
}

void test_pixel_strip_set_output_changes_live_brightness_not_config() {
    PixelStripDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = true;
    PixelStripDevice device(config);
    bindPixelStripIdentity(device, 102U);
    device.begin(10U);
    device.tick100ms(11U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT8(percentToPixelBrightness(50U), device.liveBrightness()); // startupBrightness default
    TEST_ASSERT_FALSE(device.retainedStateDirty());

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "70"}));
    TEST_ASSERT_EQUAL_UINT8(percentToPixelBrightness(70U), device.liveBrightness());
    TEST_ASSERT_TRUE(device.retainedStateDirty());
    // The command never touches the persisted config -- only the live/retained value.
    TEST_ASSERT_EQUAL_UINT8(config.startupBrightness, device.config().startupBrightness);

    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "150"}));
}

void test_pixel_strip_set_output_on_off_gates_without_changing_brightness() {
    PixelStripDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = true;
    PixelStripDevice device(config);
    bindPixelStripIdentity(device, 105U);
    device.begin(10U);
    device.tick100ms(11U);
    TEST_ASSERT_TRUE(device.liveOn()); // on by default

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "70"}));
    const uint8_t brightnessBeforeOff = device.liveBrightness();
    TEST_ASSERT_EQUAL_UINT8(percentToPixelBrightness(70U), brightnessBeforeOff);

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), R"({"on":false})"}));
    TEST_ASSERT_FALSE(device.liveOn());
    // Turning off never touches the live brightness value itself -- only the gate applied at
    // output time (PixelStripDevice::applyLiveBrightness()).
    TEST_ASSERT_EQUAL_UINT8(brightnessBeforeOff, device.liveBrightness());
    TEST_ASSERT_TRUE(device.retainedStateDirty());

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), R"({"on":true})"}));
    TEST_ASSERT_TRUE(device.liveOn());
    TEST_ASSERT_EQUAL_UINT8(brightnessBeforeOff, device.liveBrightness());
}

void test_pixel_strip_rejects_invalid_on_payload() {
    PixelStripDevice device(makeConfig());
    bindPixelStripIdentity(device, 106U);
    device.begin(10U);
    device.tick100ms(11U);

    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), R"({"on":"yes"})"}));
    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "{}"}));
    TEST_ASSERT_TRUE(device.liveOn()); // unchanged by the rejected commands
}

void test_pixel_strip_restores_retained_on_state_on_reboot() {
    PixelStripDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = true;
    const DeviceConfigBlob blob = encodeConfig(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 107U;
    record.header.typeId = kPixelStripDeviceTypeId;
    record.header.configVersion = kPixelStripDeviceConfigVersion;

    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    PixelStripDevice device(record, blob);
    device.begin(10U);
    device.tick100ms(11U);
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), R"({"on":false})"}));
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    PixelStripDevice restored(record, blob);
    TEST_ASSERT_TRUE(restored.loadRetainedState(retainedStore).ok());
    restored.begin(30U);
    restored.tick100ms(31U);
    TEST_ASSERT_FALSE(restored.liveOn());
}

void test_pixel_strip_restores_retained_brightness_on_reboot() {
    PixelStripDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = true;
    const DeviceConfigBlob blob = encodeConfig(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 103U;
    record.header.typeId = kPixelStripDeviceTypeId;
    record.header.configVersion = kPixelStripDeviceConfigVersion;

    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    PixelStripDevice device(record, blob);
    device.begin(10U);
    device.tick100ms(11U);
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "20"}));
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    PixelStripDevice restored(record, blob);
    TEST_ASSERT_TRUE(restored.loadRetainedState(retainedStore).ok());
    restored.begin(30U);
    restored.tick100ms(31U);
    TEST_ASSERT_EQUAL_UINT8(percentToPixelBrightness(20U), restored.liveBrightness());
}

void test_pixel_strip_ignores_retained_brightness_when_restore_disabled() {
    PixelStripDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = false;
    PixelStripDevice device(config);
    bindPixelStripIdentity(device, 104U);
    device.begin(10U);
    device.tick100ms(11U);
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "20"}));

    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    // restorePreviousState is false -- saveRetainedState() must be a no-op, matching
    // AbstractOutputDevice's shouldRestorePreviousState() guard.
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());
    PixelStripRetainedStateV1 record{};
    TEST_ASSERT_FALSE(retainedStore.load(device.deviceId(), record).ok());
}

void test_pixel_strip_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = types.find(kPixelStripDeviceTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("PixelStripDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::PixelStrip));

    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kPixelStripDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("pixel_strip", adapter->typeName());
    TEST_ASSERT_EQUAL_PTR(adapter, adapters.findByName("pixel_strip"));
}

void test_pixel_strip_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "pixel_strip";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeConfig().writeJson(createConfig);
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-pixel_strip.request.schema.json", createDoc.as<JsonVariantConst>());

    StaticJsonDocument<256> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["pixelCount"] = 20;
    updateConfig["startupBrightness"] = 70;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-pixel_strip.request.schema.json", updateDoc.as<JsonVariantConst>());

    PixelStripDevice device(makeConfig());
    bindPixelStripIdentity(device, 101U);
    device.begin(10U);
    device.tick100ms(11U);
    StaticJsonDocument<1024> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    PixelStripDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-pixel_strip.response.schema.json", outputDoc.as<JsonVariantConst>());
}
