#include "JsonSchemaSmokeValidator.h"
#include "devices/core/DeviceTypes.h"
#include "devices/pixel/effects/PixelEffectAlertDevice.h"
#include "devices/pixel/effects/PixelEffectAlertDeviceConfig.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/pixel_strip/PixelEffectAlertDeviceApiAdapter.h"

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

class FakeCondition final : public DeviceRuntimeBase, public IStatusRuntime {
public:
    FakeCondition() : DeviceRuntimeBase((PState)&FakeCondition::Idle) {
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
    const IStatusRuntime* statusRuntime() const override {
        return this;
    }
    bool isActive() const override {
        return active_;
    }

    bool active_{false};

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }
};

PixelEffectAlertDeviceConfigV1 makeConfig() {
    PixelEffectAlertDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "pixel-effect-alert");
    config.color = PixelColor{255, 0, 0};
    config.blinkIntervalMs = 500U;
    return config;
}

DeviceConfigBlob encodeConfig(const PixelEffectAlertDeviceConfigV1& config) {
    DeviceConfigBlob blob{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = pixelEffectAlertDeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(PixelEffectAlertDeviceConfigV1::kMagic, config, buffer, size));
    TEST_ASSERT_TRUE(blob.assign(buffer, size));
    return blob;
}

void bindIdentity(PixelEffectAlertDevice& device, DeviceId deviceId, DeviceId stripId, DeviceId conditionId) {
    const DeviceConfigBlob blob = encodeConfig(device.config());
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = PixelEffectAlertDevice::descriptor().typeId;
    record.header.configVersion = PixelEffectAlertDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());
    record.deps[0] = DeviceDependencyLink{DeviceRole::PixelStrip, stripId, false};
    uint8_t index = 1U;
    if (conditionId != 0U) {
        record.deps[index++] = DeviceDependencyLink{DeviceRole::Condition, conditionId, false};
    }
    record.depCount = index;
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, blob);
}

} // namespace

void test_pixel_effect_alert_stays_black_with_no_condition_attached() {
    PixelEffectAlertDevice device(makeConfig());
    FakePixelStrip strip;
    bindIdentity(device, 300U, 301U, 0U);
    device.setDependencyRuntimeAt(0, &strip);
    device.begin(10U);
    device.tick100ms(11U); // Idle -> Starting -> Ready (status set, Ready body not run yet)
    device.tick100ms(12U); // Ready body runs for the first time: paints black (unsatisfied)

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_FALSE(device.conditionsSatisfied());
    TEST_ASSERT_EQUAL_UINT32(1U, strip.fillCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, strip.lastFillColor.r); // black, never the alert color

    // Repeated ticks with the unsatisfied condition must not repaint.
    device.tick100ms(600U);
    TEST_ASSERT_EQUAL_UINT32(1U, strip.fillCalls);
}

void test_pixel_effect_alert_toggles_at_blink_interval_while_condition_active() {
    PixelEffectAlertDevice device(makeConfig());
    FakePixelStrip strip;
    FakeCondition condition;
    bindIdentity(device, 310U, 311U, 312U);
    device.setDependencyRuntimeAt(0, &strip);
    device.setDependencyRuntimeAt(1, &condition);
    device.begin(10U);
    device.tick100ms(11U); // Idle -> Starting -> Ready (status set, Ready body not run yet)
    device.tick100ms(50U); // Ready body runs for the first time: paints black (unsatisfied)
    TEST_ASSERT_FALSE(device.conditionsSatisfied());
    TEST_ASSERT_EQUAL_UINT32(1U, strip.fillCalls);

    // The unsatisfied->satisfied transition must repaint immediately on its own on-phase, not wait
    // out whatever was left of a blink timer that was counting down for the black state.
    condition.active_ = true;
    device.tick100ms(100U);
    TEST_ASSERT_TRUE(device.conditionsSatisfied());
    TEST_ASSERT_EQUAL_UINT32(2U, strip.fillCalls);
    TEST_ASSERT_EQUAL_UINT8(255U, strip.lastFillColor.r);

    // Well inside the 500ms window (measured from the transition at t=100): no repaint.
    device.tick100ms(200U);
    TEST_ASSERT_EQUAL_UINT32(2U, strip.fillCalls);

    // Crossing the boundary flips the phase to off (black).
    device.tick100ms(650U);
    TEST_ASSERT_EQUAL_UINT32(3U, strip.fillCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, strip.lastFillColor.r);

    // Crossing the next boundary flips back on.
    device.tick100ms(1200U);
    TEST_ASSERT_EQUAL_UINT32(4U, strip.fillCalls);
    TEST_ASSERT_EQUAL_UINT8(255U, strip.lastFillColor.r);

    condition.active_ = false;
    device.tick100ms(1300U);
    TEST_ASSERT_EQUAL_UINT32(5U, strip.fillCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, strip.lastFillColor.r);
}

void test_pixel_effect_alert_inverted_condition() {
    PixelEffectAlertDevice device(makeConfig());
    FakePixelStrip strip;
    FakeCondition condition;
    DeviceConfigBlob blob = encodeConfig(device.config());
    DeviceRegistryEntry record{};
    record.header.deviceId = 320U;
    record.header.typeId = PixelEffectAlertDevice::descriptor().typeId;
    record.header.configVersion = PixelEffectAlertDevice::descriptor().currentConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());
    record.deps[0] = DeviceDependencyLink{DeviceRole::PixelStrip, 321U, false};
    record.deps[1] = DeviceDependencyLink{DeviceRole::Condition, 322U, true}; // inverted
    record.depCount = 2U;
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, blob);
    device.setDependencyRuntimeAt(0, &strip);
    device.setDependencyRuntimeAt(1, &condition);
    device.begin(10U);
    device.tick100ms(11U);

    // condition inactive + inverted -> satisfied
    TEST_ASSERT_TRUE(device.conditionsSatisfied());

    condition.active_ = true;
    device.tick100ms(12U);
    TEST_ASSERT_FALSE(device.conditionsSatisfied());
}

void test_pixel_effect_alert_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = types.find(kPixelEffectAlertDeviceTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->exclusiveDependencyRoles.contains(DeviceRole::PixelStrip));

    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kPixelEffectAlertDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("pixel_effect_alert", adapter->typeName());
}

void test_pixel_effect_alert_api_adapter_schema_smoke() {
    StaticJsonDocument<1024> createDoc;
    createDoc["typeName"] = "pixel_effect_alert";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeConfig().writeJson(createConfig);
    JsonArray deps = createConfig.createNestedArray("deps");
    JsonObject stripDep = deps.createNestedObject();
    stripDep["role"] = "pixel_strip";
    stripDep["deviceId"] = 1;
    JsonObject conditionDep = deps.createNestedObject();
    conditionDep["role"] = "condition";
    conditionDep["deviceId"] = 2;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-pixel_effect_alert.request.schema.json",
                            createDoc.as<JsonVariantConst>());
}
