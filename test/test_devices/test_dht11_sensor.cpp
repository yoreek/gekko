#include "JsonSchemaSmokeValidator.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/dht11/Dht11Protocol.h"
#include "devices/sensors/dht11/Dht11SensorConfig.h"
#include "devices/sensors/dht11/Dht11SensorDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/dht11/Dht11SensorDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <array>
#include <cstdio>
#include <string>
#include <unity.h>

using namespace ewfm;

namespace {

void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}

class ScriptedDht11LineDriver final : public IDht11LineDriver {
public:
    bool driveLow(uint8_t pin) override {
        driveLowPin = pin;
        ++driveLowCalls;
        return driveLowResult;
    }

    bool release(uint8_t pin, bool internalPullup) override {
        releasePin = pin;
        releaseInternalPullup = internalPullup;
        ++releaseCalls;
        captureActive = true;
        captureMicros = 0U;
        return releaseResult;
    }

    bool read(uint8_t pin, bool& level) override {
        readPin = pin;
        ++readCalls;
        if (!readResult) {
            return false;
        }
        level = levelAt(captureMicros);
        return true;
    }

    void waitMicros(uint32_t microseconds) override {
        currentMicros += microseconds;
        if (captureActive) {
            captureMicros += microseconds;
        }
    }

    void setFrame(const std::array<uint8_t, kDht11FrameBytes>& frame) {
        frame_ = frame;
    }

    void setCorruptChecksum(bool enabled) {
        corruptChecksum = enabled;
    }

    bool driveLowResult{true};
    bool releaseResult{true};
    bool readResult{true};
    uint8_t driveLowPin{0xFF};
    uint8_t releasePin{0xFF};
    bool releaseInternalPullup{false};
    uint8_t readPin{0xFF};
    int driveLowCalls{0};
    int releaseCalls{0};
    int readCalls{0};
    uint32_t currentMicros{0U};

private:
    bool levelAt(uint32_t elapsedMicros) const {
        if (elapsedMicros < 80U) {
            return false;
        }
        elapsedMicros -= 80U;
        if (elapsedMicros < 80U) {
            return true;
        }
        elapsedMicros -= 80U;

        std::array<uint8_t, kDht11FrameBytes> effectiveFrame = frame_;
        if (corruptChecksum) {
            effectiveFrame[4] ^= 0x01U;
        }

        for (uint8_t byteIndex = 0U; byteIndex < kDht11FrameBytes; ++byteIndex) {
            for (uint8_t bitIndex = 0U; bitIndex < 8U; ++bitIndex) {
                if (elapsedMicros < 50U) {
                    return false;
                }
                elapsedMicros -= 50U;
                const bool bitValue = (effectiveFrame[byteIndex] & (1U << (7U - bitIndex))) != 0U;
                const uint32_t highMicros = bitValue ? 70U : 26U;
                if (elapsedMicros < highMicros) {
                    return true;
                }
                elapsedMicros -= highMicros;
            }
        }
        return false;
    }

    std::array<uint8_t, kDht11FrameBytes> frame_{};
    bool captureActive{false};
    uint32_t captureMicros{0U};
    bool corruptChecksum{false};
};

Dht11SensorConfigV3 makeConfig() {
    Dht11SensorConfigV3 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "climate");
    config.gpioPin = 17U;
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Celsius);
    config.reportAlways = 0U;
    config.reportDeltaCentiCelsius = 10U;
    config.reportDeltaCentiPercent = 10U;
    config.pollMs = 5000U;
    config.temperatureFilter.smoothingWeight = 0.25F;
    config.humidityFilter.smoothingWeight = 0.5F;
    return config;
}

std::array<uint8_t, kDht11FrameBytes> makeFrame(uint8_t humidityInteger, uint8_t humidityDecimal, uint8_t temperatureInteger,
                                                uint8_t temperatureDecimal) {
    const uint8_t checksum = static_cast<uint8_t>(humidityInteger + humidityDecimal + temperatureInteger + temperatureDecimal);
    return {humidityInteger, humidityDecimal, temperatureInteger, temperatureDecimal, checksum};
}

BoundedBlob<kMaxDeviceConfigBytes> encodeConfigBlob(const Dht11SensorConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dht11SensorConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Dht11SensorConfigV3::kMagic, config, buffer, size));
    TEST_ASSERT_TRUE(payload.assign(buffer, size));
    return payload;
}

void bindIdentity(Dht11SensorDevice& device, DeviceId deviceId, const Dht11SensorConfigV3& config) {
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = Dht11SensorDevice::descriptor().typeId;
    record.header.configVersion = Dht11SensorDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, encodeConfigBlob(config));
}

bool hasValidReadings(const Dht11SensorDevice& device) {
    TemperatureReading temperature{};
    HumidityReading humidity{};
    (void)device.latestTemperatureReading(temperature);
    (void)device.latestHumidityReading(humidity);
    return temperature.valid && humidity.valid;
}

void tickUntilReady(Dht11SensorDevice& device, uint32_t startNow) {
    device.begin(startNow);
    device.tick100ms(startNow);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
}

uint32_t tickUntilReading(Dht11SensorDevice& device, uint32_t startNow) {
    device.begin(startNow);
    for (uint32_t now = startNow; now < startNow + 10000U; now += 100U) {
        device.tick100ms(now);
        if (hasValidReadings(device)) {
            return now;
        }
    }
    return startNow + 10000U;
}

} // namespace

void test_dht11_protocol_decodes_frame_and_checks_checksum() {
    const std::array<uint8_t, kDht11FrameBytes> frame = makeFrame(55U, 0U, 24U, 0U);
    TEST_ASSERT_TRUE(dht11ChecksumValid(frame.data()));

    int32_t milliCelsius = 0;
    int32_t milliPercent = 0;
    const char* error = nullptr;
    TEST_ASSERT_TRUE(dht11DecodeFrame(frame.data(), milliCelsius, milliPercent, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_INT32(24000, milliCelsius);
    TEST_ASSERT_EQUAL_INT32(55000, milliPercent);

    std::array<uint8_t, kDht11FrameBytes> badFrame = frame;
    badFrame[4] ^= 0x01U;
    TEST_ASSERT_FALSE(dht11ChecksumValid(badFrame.data()));
    TEST_ASSERT_FALSE(dht11DecodeFrame(badFrame.data(), milliCelsius, milliPercent, error));
    TEST_ASSERT_EQUAL_STRING("checksum_error", error);
}

void test_dht11_config_codec_json_and_validation() {
    Dht11SensorConfigV3 config = makeConfig();
    config.gpioPin = 27U;
    config.internalPullup = 1U;
    config.captureMode = static_cast<uint8_t>(Dht11CaptureMode::Native);
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    config.reportAlways = 1U;
    config.reportDeltaCentiCelsius = 25U;
    config.reportDeltaCentiPercent = 40U;
    config.pollMs = 15000U;
    config.temperatureFilter.smoothingWeight = 0.5F;
    config.humidityFilter.calibrationOffset = -750.0F;

    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeConfigBlob(config);
    Dht11SensorConfigV3 decoded{};
    TEST_ASSERT_TRUE(decodeDht11SensorConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(27U, decoded.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(1U, decoded.internalPullup);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Dht11CaptureMode::Native), decoded.captureMode);
    TEST_ASSERT_EQUAL_UINT8(temperatureUnitToByte(TemperatureUnit::Fahrenheit), decoded.outputUnit);
    TEST_ASSERT_EQUAL_UINT8(1U, decoded.reportAlways);
    TEST_ASSERT_EQUAL_UINT16(25U, decoded.reportDeltaCentiCelsius);
    TEST_ASSERT_EQUAL_UINT16(40U, decoded.reportDeltaCentiPercent);
    TEST_ASSERT_EQUAL_UINT32(15000U, decoded.pollMs);
    TEST_ASSERT_EQUAL_FLOAT(0.5F, decoded.temperatureFilter.smoothingWeight);
    TEST_ASSERT_EQUAL_FLOAT(-750.0F, decoded.humidityFilter.calibrationOffset);
    TEST_ASSERT_EQUAL_STRING("climate", decoded.name);

    StaticJsonDocument<512> doc;
    decoded.writeJson(doc.to<JsonObject>());
    Dht11SensorConfigV3 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parseDht11SensorConfigJson(doc.as<JsonObjectConst>(), parsed, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_UINT8(27U, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.internalPullup);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Dht11CaptureMode::Native), parsed.captureMode);
    TEST_ASSERT_EQUAL_UINT32(15000U, parsed.pollMs);

    Dht11SensorConfigV3 invalid = makeConfig();
    invalid.gpioPin = 6U;
    TEST_ASSERT_FALSE(invalid.validate().ok());
}

void test_dht11_config_migrates_v2_to_v3_native_mode() {
    EWFM_LEGACY_CONFIG_USE_BEGIN
    Dht11SensorConfigV2 legacy{};
    legacy.enabled = 1U;
    std::snprintf(legacy.name, sizeof(legacy.name), "%s", "climate");
    legacy.gpioPin = 25U;
    legacy.internalPullup = 1U;
    legacy.pollMs = 7000U;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dht11SensorConfigSize(legacy);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Dht11SensorConfigV2::kMagic, legacy, buffer, size));
    EWFM_LEGACY_CONFIG_USE_END

    Dht11SensorConfigV3 migrated{};
    TEST_ASSERT_TRUE(decodeDht11SensorConfig(buffer, size, migrated));
    TEST_ASSERT_EQUAL_UINT8(25U, migrated.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(1U, migrated.internalPullup);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Dht11CaptureMode::Native), migrated.captureMode);
    TEST_ASSERT_EQUAL_UINT32(7000U, migrated.pollMs);
}

void test_dht11_type_and_api_adapter_are_registered() {
    const DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = types.find(kDht11SensorTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("Dht11SensorDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(ITemperatureReadingRuntime::kProvidedRole));
    TEST_ASSERT_FALSE(descriptor->supportsCommands);
    TEST_ASSERT_FALSE(descriptor->supportsRetainedState);

    const DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kDht11SensorTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("dht11", adapter->typeName());
}

void test_dht11_device_publishes_temperature_and_humidity_from_single_wire_frame() {
    ScriptedDht11LineDriver driver;
    driver.setFrame(makeFrame(55U, 0U, 24U, 0U));

    Dht11SensorDevice device(makeConfig(), driver);
    bindIdentity(device, 101U, device.config());

    tickUntilReady(device, 0U);
    const uint32_t readingAt = tickUntilReading(device, 1000U);
    TEST_ASSERT_TRUE(readingAt < 11000U);

    TemperatureReading temperature{};
    HumidityReading humidity{};
    TEST_ASSERT_TRUE(device.latestTemperatureReading(temperature));
    TEST_ASSERT_TRUE(device.latestHumidityReading(humidity));
    TEST_ASSERT_TRUE(temperature.valid);
    TEST_ASSERT_TRUE(humidity.valid);
    TEST_ASSERT_EQUAL_INT32(24000, temperature.milliCelsius);
    TEST_ASSERT_EQUAL_INT32(55000, humidity.milliPercent);
    TEST_ASSERT_EQUAL_UINT8(17U, driver.driveLowPin);
    TEST_ASSERT_EQUAL_UINT8(17U, driver.releasePin);
    TEST_ASSERT_TRUE(driver.driveLowCalls > 0);
    TEST_ASSERT_TRUE(driver.releaseCalls > 0);
    TEST_ASSERT_TRUE(driver.readCalls > 0);
}

void test_dht11_device_faults_after_repeated_invalid_frames() {
    ScriptedDht11LineDriver driver;
    driver.setFrame(makeFrame(55U, 0U, 24U, 0U));
    driver.setCorruptChecksum(true);

    Dht11SensorDevice device(makeConfig(), driver);
    bindIdentity(device, 102U, device.config());

    device.begin(0U);
    for (uint32_t now = 0U; now <= 40000U && device.status() != DeviceStatus::Faulted; now += 1000U) {
        device.tick100ms(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(device.status()));
}

void test_dht11_api_adapter_rejects_dependencies_and_writes_runtime_json() {
    ScriptedDht11LineDriver driver;
    driver.setFrame(makeFrame(55U, 0U, 24U, 0U));
    Dht11SensorDevice device(makeConfig(), driver);
    bindIdentity(device, 103U, device.config());
    tickUntilReading(device, 1000U);

    StaticJsonDocument<512> updateDoc;
    updateDoc["config"]["name"] = "climate";
    updateDoc["config"]["enabled"] = true;
    updateDoc["config"]["gpioPin"] = 17U;
    JsonArray deps = updateDoc["deps"].to<JsonArray>();
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "i2c_bus";
    dep["deviceId"] = 12U;

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(
        Dht11SensorDeviceApiAdapter::instance().parseUpdateConfigRequest(updateDoc.as<JsonObjectConst>(), device, request, error));
    TEST_ASSERT_NOT_NULL(error);

    StaticJsonDocument<1536> outputDoc;
    Dht11SensorDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), outputDoc.to<JsonObject>());
    const JsonObjectConst root = outputDoc.as<JsonObjectConst>();
    TEST_ASSERT_TRUE(root["record"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(root["config"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(root["runtime"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(root["runtime"]["output"]["temperature"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(root["runtime"]["output"]["humidity"].is<JsonObjectConst>());
}

void test_dht11_schema_contracts_cover_config_and_response_shapes() {
    Dht11SensorConfigV3 config = makeConfig();
    StaticJsonDocument<512> configDoc;
    config.writeJson(configDoc.to<JsonObject>());
    assertMatchesJsonSchema("schemas/rest/v1/devices/dht11.config.schema.json", configDoc.as<JsonVariantConst>());

    ScriptedDht11LineDriver driver;
    driver.setFrame(makeFrame(55U, 0U, 24U, 0U));
    Dht11SensorDevice device(config, driver);
    bindIdentity(device, 104U, device.config());
    tickUntilReading(device, 1000U);

    StaticJsonDocument<1536> responseDoc;
    Dht11SensorDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), responseDoc.to<JsonObject>());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-dht11.response.schema.json", responseDoc.as<JsonVariantConst>());
}
