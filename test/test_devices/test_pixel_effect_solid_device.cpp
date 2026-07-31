#include "JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceTypes.h"
#include "devices/pixel/effects/PixelEffectSolidDevice.h"
#include "devices/pixel/effects/PixelEffectSolidDeviceConfig.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/pixel_strip/PixelEffectSolidDeviceApiAdapter.h"

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

class FakePixelStrip final : public DeviceRuntimeBase, public IPixelStripRuntime {
public:
    FakePixelStrip() : DeviceRuntimeBase((PState)&FakePixelStrip::Idle) {
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
    const IPixelStripRuntime* pixelStripRuntime() const override {
        return this;
    }

    uint16_t pixelCount() const override {
        return kCount;
    }
    bool setPixel(uint16_t index, PixelColor color) override {
        if (index >= kCount) {
            return false;
        }
        buffer_[index] = color;
        return true;
    }
    bool fill(PixelColor color) override {
        for (uint16_t index = 0; index < kCount; ++index) {
            buffer_[index] = color;
        }
        ++fillCalls;
        lastFillColor = color;
        return true;
    }
    bool show(uint32_t now) override {
        (void)now;
        ++showCalls;
        return true;
    }
    PixelColor currentPixel(uint16_t index) const override {
        return index < kCount ? buffer_[index] : PixelColor{};
    }

    static constexpr uint16_t kCount = 8U;
    PixelColor buffer_[kCount]{};
    uint32_t fillCalls{0U};
    uint32_t showCalls{0U};
    PixelColor lastFillColor{};

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }
};

PixelEffectSolidDeviceConfigV1 makeConfig() {
    PixelEffectSolidDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "pixel-effect-solid");
    config.startupColor = PixelColor{200, 100, 50};
    return config;
}

DeviceConfigBlob encodeConfig(const PixelEffectSolidDeviceConfigV1& config) {
    DeviceConfigBlob blob{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = pixelEffectSolidDeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(PixelEffectSolidDeviceConfigV1::kMagic, config, buffer, size));
    TEST_ASSERT_TRUE(blob.assign(buffer, size));
    return blob;
}

void bindIdentity(PixelEffectSolidDevice& device, DeviceId deviceId, DeviceId stripId) {
    const DeviceConfigBlob blob = encodeConfig(device.config());
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = PixelEffectSolidDevice::descriptor().typeId;
    record.header.configVersion = PixelEffectSolidDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());
    record.deps[0] = DeviceDependencyLink{DeviceRole::PixelStrip, stripId, false};
    record.depCount = 1U;
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, blob);
}

} // namespace

void test_pixel_effect_solid_fills_target_strip_on_start() {
    PixelEffectSolidDevice device(makeConfig());
    FakePixelStrip strip;
    bindIdentity(device, 200U, 201U);
    device.setDependencyRuntimeAt(0, &strip);
    device.begin(10U);
    device.tick100ms(11U); // Idle -> Starting -> Ready (status set, Ready body not run yet)
    device.tick100ms(12U); // Ready body runs for the first time: applies the color

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT32(1U, strip.fillCalls);
    TEST_ASSERT_EQUAL_UINT32(1U, strip.showCalls);
    TEST_ASSERT_EQUAL_UINT8(200U, strip.lastFillColor.r);
    TEST_ASSERT_EQUAL_UINT8(100U, strip.lastFillColor.g);
    TEST_ASSERT_EQUAL_UINT8(50U, strip.lastFillColor.b);

    // A tick with nothing changed must not repaint the strip.
    device.tick100ms(13U);
    TEST_ASSERT_EQUAL_UINT32(1U, strip.fillCalls);
    TEST_ASSERT_EQUAL_UINT32(1U, strip.showCalls);
}

void test_pixel_effect_solid_set_output_changes_live_color_not_config() {
    PixelEffectSolidDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = true;
    PixelEffectSolidDevice device(config);
    FakePixelStrip strip;
    bindIdentity(device, 210U, 211U);
    device.setDependencyRuntimeAt(0, &strip);
    device.begin(10U);
    device.tick100ms(11U);
    device.tick100ms(12U);
    TEST_ASSERT_EQUAL_UINT32(1U, strip.fillCalls);
    TEST_ASSERT_FALSE(device.retainedStateDirty());

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "{\"r\":5,\"g\":6,\"b\":7}"}));
    TEST_ASSERT_EQUAL_UINT32(2U, strip.fillCalls);
    TEST_ASSERT_EQUAL_UINT8(5U, strip.lastFillColor.r);
    TEST_ASSERT_TRUE(device.retainedStateDirty());
    // The command never touches the persisted config -- only the live/retained value.
    TEST_ASSERT_EQUAL_UINT8(config.startupColor.r, device.config().startupColor.r);

    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "{\"r\":300,\"g\":0,\"b\":0}"}));
}

void test_pixel_effect_solid_set_output_on_off_gates_without_changing_color() {
    PixelEffectSolidDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = true;
    PixelEffectSolidDevice device(config);
    FakePixelStrip strip;
    bindIdentity(device, 215U, 216U);
    device.setDependencyRuntimeAt(0, &strip);
    device.begin(10U);
    device.tick100ms(11U);
    device.tick100ms(12U);
    TEST_ASSERT_TRUE(device.liveOn());                    // on by default
    TEST_ASSERT_EQUAL_UINT8(200U, strip.lastFillColor.r); // startupColor applied while on

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), R"({"on":false})"}));
    TEST_ASSERT_FALSE(device.liveOn());
    // Off fills black at the strip regardless of the configured color...
    TEST_ASSERT_EQUAL_UINT8(0U, strip.lastFillColor.r);
    TEST_ASSERT_EQUAL_UINT8(0U, strip.lastFillColor.g);
    TEST_ASSERT_EQUAL_UINT8(0U, strip.lastFillColor.b);
    // ...but liveColor() itself is untouched -- only the gate applied at output time
    // (PixelEffectSolidDevice::applyColorIfNeeded()) changed.
    TEST_ASSERT_EQUAL_UINT8(200U, device.liveColor().r);
    TEST_ASSERT_TRUE(device.retainedStateDirty());

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), R"({"on":true})"}));
    TEST_ASSERT_TRUE(device.liveOn());
    TEST_ASSERT_EQUAL_UINT8(200U, strip.lastFillColor.r); // startupColor re-applied
}

void test_pixel_effect_solid_rejects_invalid_on_payload() {
    PixelEffectSolidDevice device(makeConfig());
    FakePixelStrip strip;
    bindIdentity(device, 217U, 218U);
    device.setDependencyRuntimeAt(0, &strip);
    device.begin(10U);
    device.tick100ms(11U);
    device.tick100ms(12U);

    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), R"({"on":"yes"})"}));
    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "{}"}));
    TEST_ASSERT_TRUE(device.liveOn()); // unchanged by the rejected commands
}

void test_pixel_effect_solid_restores_retained_on_state_on_reboot() {
    PixelEffectSolidDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = true;
    const DeviceConfigBlob blob = encodeConfig(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 219U;
    record.header.typeId = PixelEffectSolidDevice::descriptor().typeId;
    record.header.configVersion = PixelEffectSolidDevice::descriptor().currentConfigVersion;
    record.deps[0] = DeviceDependencyLink{DeviceRole::PixelStrip, 220U, false};
    record.depCount = 1U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    PixelEffectSolidDevice device(record, blob);
    FakePixelStrip strip;
    device.setDependencyRuntimeAt(0, &strip);
    device.begin(10U);
    device.tick100ms(11U);
    device.tick100ms(12U);
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), R"({"on":false})"}));
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    PixelEffectSolidDevice restored(record, blob);
    FakePixelStrip restoredStrip;
    TEST_ASSERT_TRUE(restored.loadRetainedState(retainedStore).ok());
    restored.setDependencyRuntimeAt(0, &restoredStrip);
    restored.begin(30U);
    restored.tick100ms(31U);
    restored.tick100ms(32U);
    TEST_ASSERT_FALSE(restored.liveOn());
}

void test_pixel_effect_solid_restores_retained_color_on_reboot() {
    PixelEffectSolidDeviceConfigV1 config = makeConfig();
    config.restorePreviousState = true;
    const DeviceConfigBlob blob = encodeConfig(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 212U;
    record.header.typeId = PixelEffectSolidDevice::descriptor().typeId;
    record.header.configVersion = PixelEffectSolidDevice::descriptor().currentConfigVersion;
    record.deps[0] = DeviceDependencyLink{DeviceRole::PixelStrip, 213U, false};
    record.depCount = 1U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    PixelEffectSolidDevice device(record, blob);
    FakePixelStrip strip;
    device.setDependencyRuntimeAt(0, &strip);
    device.begin(10U);
    device.tick100ms(11U);
    device.tick100ms(12U);
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, device.deviceId(), "{\"r\":9,\"g\":8,\"b\":7}"}));
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    PixelEffectSolidDevice restored(record, blob);
    FakePixelStrip restoredStrip;
    TEST_ASSERT_TRUE(restored.loadRetainedState(retainedStore).ok());
    restored.setDependencyRuntimeAt(0, &restoredStrip);
    restored.begin(30U);
    restored.tick100ms(31U);
    restored.tick100ms(32U);
    TEST_ASSERT_EQUAL_UINT8(9U, restored.liveColor().r);
    TEST_ASSERT_EQUAL_UINT8(8U, restored.liveColor().g);
    TEST_ASSERT_EQUAL_UINT8(7U, restored.liveColor().b);
}

void test_pixel_effect_solid_is_dependency_blocked_without_target() {
    PixelEffectSolidDevice device(makeConfig());
    bindIdentity(device, 220U, 221U);
    device.begin(10U);
    device.tick100ms(11U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(device.status()));
}

void test_pixel_effect_solid_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = types.find(kPixelEffectSolidDeviceTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->exclusiveDependencyRoles.contains(DeviceRole::PixelStrip));

    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kPixelEffectSolidDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("pixel_effect_solid", adapter->typeName());
}

void test_pixel_effect_solid_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "pixel_effect_solid";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeConfig().writeJson(createConfig);
    JsonArray deps = createConfig.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "pixel_strip";
    dep["deviceId"] = 1;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-pixel_effect_solid.request.schema.json",
                            createDoc.as<JsonVariantConst>());
}
