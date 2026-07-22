#include "JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayTextPlaceholderAst.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/htu21/Htu21Protocol.h"
#include "devices/sensors/htu21/Htu21SensorDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/htu21/Htu21SensorDeviceApiAdapter.h"
#include "integrations/rest/rtc_ds3231/Ds3231RtcDeviceApiAdapter.h"
#include "metrics/MetricTypes.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <string>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}

// Same generator polynomial the device uses; producing the CRC the checker expects proves the
// checker against ReefDuino's reference implementation by construction.
uint8_t computeHtu21Crc(uint16_t value) {
    uint32_t remainder = static_cast<uint32_t>(value) << 8;
    uint32_t divisor = 0x988000UL;
    for (int bit = 0; bit < 16; ++bit) {
        if ((remainder & (1UL << (23 - bit))) != 0UL) {
            remainder ^= divisor;
        }
        divisor >>= 1;
    }
    return static_cast<uint8_t>(remainder);
}

// Simulates the HTU21's no-hold command protocol over the II2cBusDriver primitives: a 1-byte
// command write selects the measurement (0xF3 temperature / 0xF5 humidity), and the following
// A three-byte request returns MSB, LSB, CRC of the configured raw value.
class FakeHtu21I2cDriver final : public II2cBusDriver {
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
        pendingWrite.clear();
    }

    uint8_t endTransmission(bool) override {
        if (!present) {
            return 2U;
        }
        if (!pendingWrite.empty()) {
            lastCommand = pendingWrite[0];
        }
        return 0U;
    }

    size_t requestFrom(uint8_t, size_t size, bool) override {
        readBuffer.clear();
        readPos = 0;
        if (!present || truncateReads) {
            return 0U;
        }
        const bool humidity = lastCommand == kHtu21CmdMeasureHumidityNoHold;
        const uint16_t raw = humidity ? humidityRaw : temperatureRaw;
        uint8_t crc = computeHtu21Crc(raw);
        if ((humidity && corruptHumidityCrc) || (!humidity && corruptTemperatureCrc)) {
            crc ^= 0xFFU;
        }
        readBuffer.push_back(static_cast<uint8_t>(raw >> 8));
        readBuffer.push_back(static_cast<uint8_t>(raw & 0xFFU));
        readBuffer.push_back(crc);
        return size <= readBuffer.size() ? size : readBuffer.size();
    }

    size_t write(uint8_t data) override {
        pendingWrite.push_back(data);
        return 1U;
    }

    size_t write(const uint8_t* data, size_t quantity) override {
        pendingWrite.insert(pendingWrite.end(), data, data + quantity);
        return quantity;
    }

    int available() override {
        return static_cast<int>(readBuffer.size() - readPos);
    }

    int read() override {
        if (readPos >= readBuffer.size()) {
            return -1;
        }
        return readBuffer[readPos++];
    }

    void flush() override {}

    bool present{true};
    bool truncateReads{false};
    bool corruptTemperatureCrc{false};
    bool corruptHumidityCrc{false};
    uint16_t temperatureRaw{0x6800U}; // 24536 milli-degC
    uint16_t humidityRaw{0x7000U};    // 48687 milli-%RH
    uint8_t lastAddress{0};
    uint8_t lastCommand{0};

private:
    std::vector<uint8_t> pendingWrite;
    std::vector<uint8_t> readBuffer;
    size_t readPos{0};
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

Htu21SensorConfigV3 makeHtu21Config(uint8_t i2cAddress = kHtu21DefaultI2cAddress) {
    Htu21SensorConfigV3 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "climate");
    config.i2cAddress = i2cAddress;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeBusPayload(const I2cBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, config, buffer, i2cBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, i2cBusDeviceConfigSize(config)));
    return payload;
}

template <typename Config> BoundedBlob<kMaxDeviceConfigBytes> encodeHtu21Payload(const Config& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Config::kMagic, config, buffer, htu21SensorConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, htu21SensorConfigSize(config)));
    return payload;
}

void driveBusReady(I2cBusDevice& bus, uint32_t startNow = 1U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
}

void bindBusIdentity(I2cBusDevice& bus, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = busId;
    record.header.typeId = I2cBusDevice::descriptor().typeId;
    record.header.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodeBusPayload(bus.config()).size());
    record.status = DeviceStatus::Ready;
    bus.bindDeviceIdentity(record, encodeBusPayload(bus.config()));
}

void bindHtu21Dependency(Htu21SensorDevice& sensor, DeviceId sensorId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = sensorId;
    record.header.typeId = kHtu21SensorTypeId;
    record.header.configVersion = kHtu21SensorConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::I2CBus, busId};
    record.status = DeviceStatus::Ready;
    sensor.bindDeviceIdentity(record, encodeHtu21Payload(sensor.config()));
}

bool htu21HasValidReading(const Htu21SensorDevice& sensor) {
    TemperatureReading temperature{};
    HumidityReading humidity{};
    (void)sensor.latestTemperatureReading(temperature);
    (void)sensor.latestHumidityReading(humidity);
    return temperature.valid && humidity.valid;
}

uint32_t driveHtu21UntilReading(Htu21SensorDevice& sensor, uint32_t startNow = 10U) {
    sensor.begin(startNow);
    uint32_t now = startNow + 1U;
    for (; now < startNow + 10000U && !htu21HasValidReading(sensor); now += 100U) {
        sensor.tick100ms(now);
    }
    return now;
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

DeviceCreateRequest makeHtu21CreateRequest(const char* name, DeviceId busId) {
    DeviceCreateRequest request{};
    request.typeId = kHtu21SensorTypeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::I2CBus, busId};
    request.configVersion = kHtu21SensorConfigVersion;
    request.configBlob = encodeHtu21Payload(makeHtu21Config());
    return request;
}

} // namespace

void test_htu21_protocol_crc_and_conversions() {
    // CRC checker accepts the generator's output and rejects a corrupted byte.
    TEST_ASSERT_TRUE(htu21CrcValid(0x6800U, computeHtu21Crc(0x6800U)));
    TEST_ASSERT_TRUE(htu21CrcValid(0x7C80U, computeHtu21Crc(0x7C80U)));
    TEST_ASSERT_FALSE(htu21CrcValid(0x6800U, static_cast<uint8_t>(computeHtu21Crc(0x6800U) ^ 0x01U)));

    // Datasheet formulas: T = raw*175.72/65536 - 46.85, RH = raw*125/65536 - 6.
    TEST_ASSERT_EQUAL_INT32(24536, htu21RawToMilliCelsius(0x6800U));
    TEST_ASSERT_EQUAL_INT32(48687, htu21RawToMilliPercent(0x7000U));
    // Status bits (two LSBs) are masked off before conversion.
    TEST_ASSERT_EQUAL_INT32(htu21RawToMilliCelsius(0x6800U), htu21RawToMilliCelsius(0x6803U));
    TEST_ASSERT_EQUAL_INT32(htu21RawToMilliPercent(0x7000U), htu21RawToMilliPercent(0x7003U));
    // Humidity clamps to the physical 0..100 % range.
    TEST_ASSERT_EQUAL_INT32(0, htu21RawToMilliPercent(0x0000U));
    TEST_ASSERT_EQUAL_INT32(100000, htu21RawToMilliPercent(0xFFFCU));
}

void test_htu21_config_codec_json_and_validation() {
    Htu21SensorConfigV3 config = makeHtu21Config(0x41U);
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    config.pollMs = 30000U;
    config.reportDeltaCentiCelsius = 25U;
    config.reportDeltaCentiPercent = 50U;
    config.temperatureFilter.smoothingWeight = 0.5F;
    config.humidityFilter.calibrationOffset = -1500.0F;
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeHtu21Payload(config);

    Htu21SensorConfigV3 decoded{};
    TEST_ASSERT_TRUE(decodeHtu21SensorConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(0x41U, decoded.i2cAddress);
    TEST_ASSERT_EQUAL_UINT32(30000U, decoded.pollMs);
    TEST_ASSERT_EQUAL_UINT16(25U, decoded.reportDeltaCentiCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, decoded.reportDeltaCentiPercent);
    TEST_ASSERT_EQUAL_FLOAT(0.5F, decoded.temperatureFilter.smoothingWeight);
    TEST_ASSERT_EQUAL_FLOAT(-1500.0F, decoded.humidityFilter.calibrationOffset);
    TEST_ASSERT_EQUAL_STRING("climate", decoded.name);

    StaticJsonDocument<768> doc;
    JsonObject json = doc.to<JsonObject>();
    writeHtu21SensorConfigJson(config, json);
    TEST_ASSERT_EQUAL_STRING("fahrenheit", json["unit"].as<const char*>());
    TEST_ASSERT_EQUAL_FLOAT(0.25F, json["reportDeltaCelsius"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(0.5F, json["reportDeltaHumidity"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(0.5F, json["temperatureFilter"]["smoothingWeight"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(-1500.0F, json["humidityFilter"]["calibrationOffset"].as<float>());
    TEST_ASSERT_EQUAL_UINT8(0x41U, json["i2cAddress"].as<uint8_t>());
    assertMatchesJsonSchema("schemas/rest/v1/devices/htu21.config.schema.json", doc.as<JsonVariantConst>());

    Htu21SensorConfigV3 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parseHtu21SensorConfigJson(json, parsed, error));
    TEST_ASSERT_EQUAL_UINT32(30000U, parsed.pollMs);
    TEST_ASSERT_EQUAL_UINT16(25U, parsed.reportDeltaCentiCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, parsed.reportDeltaCentiPercent);
    TEST_ASSERT_EQUAL_FLOAT(0.5F, parsed.temperatureFilter.smoothingWeight);
    TEST_ASSERT_EQUAL_FLOAT(-1500.0F, parsed.humidityFilter.calibrationOffset);
    TEST_ASSERT_EQUAL_UINT8(0x41U, parsed.i2cAddress);
    TEST_ASSERT_TRUE(parsed.validate().ok());

    Htu21SensorConfigV3 badPoll = makeHtu21Config();
    badPoll.pollMs = 100U;
    TEST_ASSERT_FALSE(badPoll.validate().ok());

    Htu21SensorConfigV3 badDelta = makeHtu21Config();
    badDelta.reportDeltaCentiPercent = 0U;
    TEST_ASSERT_FALSE(badDelta.validate().ok());

    Htu21SensorConfigV3 badFilter = makeHtu21Config();
    badFilter.humidityFilter.smoothingWeight = 0.0F;
    TEST_ASSERT_FALSE(badFilter.validate().ok());

    Htu21SensorConfigV3 badAddress = makeHtu21Config(0x80U);
    TEST_ASSERT_FALSE(badAddress.validate().ok());

    EWFM_LEGACY_CONFIG_USE_BEGIN
    Htu21SensorConfigV1 legacy{};
    legacy.enabled = config.enabled;
    std::snprintf(legacy.name, sizeof(legacy.name), "%s", config.name);
    legacy.outputUnit = config.outputUnit;
    legacy.reportAlways = config.reportAlways;
    legacy.reportDeltaCentiCelsius = config.reportDeltaCentiCelsius;
    legacy.reportDeltaCentiPercent = config.reportDeltaCentiPercent;
    legacy.pollMs = config.pollMs;
    legacy.temperatureFilter = config.temperatureFilter;
    legacy.humidityFilter = config.humidityFilter;
    uint8_t legacyBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Htu21SensorConfigV1::kMagic, legacy, legacyBuffer, htu21SensorConfigSize(legacy)));
    EWFM_LEGACY_CONFIG_USE_END
    Htu21SensorConfigV3 migrated{};
    TEST_ASSERT_TRUE(decodeHtu21SensorConfig(legacyBuffer, htu21SensorConfigSize(legacy), migrated));
    TEST_ASSERT_EQUAL_UINT8(kHtu21DefaultI2cAddress, migrated.i2cAddress);
}

void test_htu21_registry_migrates_v1_blob_on_begin() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DeviceRegistryEntry busRecord{};
    busRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    busRecord.header.deviceId = 7101U;
    busRecord.header.typeId = I2cBusDevice::descriptor().typeId;
    busRecord.header.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    busRecord.header.configRevision = 1U;
    busRecord.status = DeviceStatus::Ready;

    DeviceRegistryEntry sensorRecord{};
    sensorRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    sensorRecord.header.deviceId = 7102U;
    sensorRecord.header.typeId = kHtu21SensorTypeId;
    sensorRecord.header.configVersion = 1U;
    sensorRecord.header.configRevision = 1U;
    sensorRecord.depCount = 1U;
    sensorRecord.deps[0] = {DeviceRole::I2CBus, busRecord.header.deviceId};
    sensorRecord.status = DeviceStatus::Ready;

    EWFM_LEGACY_CONFIG_USE_BEGIN
    Htu21SensorConfigV1 legacy{};
    legacy.enabled = 1U;
    std::snprintf(legacy.name, sizeof(legacy.name), "%s", "legacy-climate");
    uint8_t legacyBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Htu21SensorConfigV1::kMagic, legacy, legacyBuffer, htu21SensorConfigSize(legacy)));
    EWFM_LEGACY_CONFIG_USE_END
    DeviceConfigBlob legacyBlob{};
    TEST_ASSERT_TRUE(legacyBlob.assign(legacyBuffer, htu21SensorConfigSize(legacy)));

    DeviceRegistrySnapshot snapshot{};
    snapshot.records = {busRecord, sensorRecord};
    snapshot.indexEntries = {{busRecord.header.deviceId, busRecord.header.typeId},
                             {sensorRecord.header.deviceId, sensorRecord.header.typeId}};
    DeviceConfigBlobMap configBlobs{};
    configBlobs[busRecord.header.deviceId] = encodeBusPayload(makeBusConfig());
    configBlobs[sensorRecord.header.deviceId] = legacyBlob;
    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());

    SequentialDeviceIdSource ids(7200U);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE_MESSAGE(registry.begin(0U).ok(), "registry begin failed");

    const Htu21SensorDevice* runtime = static_cast<const Htu21SensorDevice*>(registry.runtime(sensorRecord.header.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_UINT32(kHtu21SensorConfigVersion, runtime->configVersion());
    TEST_ASSERT_EQUAL_UINT8(kHtu21DefaultI2cAddress, runtime->config().i2cAddress);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
}

void test_htu21_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kHtu21SensorTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::TemperatureSensor));
    TEST_ASSERT_EQUAL(1, static_cast<int>(descriptor->dependencyRequirements.size()));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceRole::I2CBus), static_cast<int>(descriptor->dependencyRequirements[0].role));
    TEST_ASSERT_TRUE(descriptor->dependencyRequirements[0].required);

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(kHtu21SensorTypeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("htu21"));
}

void test_htu21_runtime_measures_temperature_and_humidity() {
    FakeHtu21I2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    bindBusIdentity(bus, 6001U);
    driveBusReady(bus);

    Htu21SensorDevice sensor(makeHtu21Config(0x41U));
    bindHtu21Dependency(sensor, 5001, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveHtu21UntilReading(sensor);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
    TEST_ASSERT_EQUAL_UINT8(0x41U, driver.lastAddress);

    TemperatureReading temperature{};
    TEST_ASSERT_TRUE(sensor.latestTemperatureReading(temperature));
    TEST_ASSERT_TRUE(temperature.valid);
    TEST_ASSERT_EQUAL_INT32(24536, temperature.milliCelsius);
    TEST_ASSERT_EQUAL_STRING("ok", sensor.latestTemperatureStatus());

    HumidityReading humidity{};
    TEST_ASSERT_TRUE(sensor.latestHumidityReading(humidity));
    TEST_ASSERT_TRUE(humidity.valid);
    TEST_ASSERT_EQUAL_INT32(48687, humidity.milliPercent);
    TEST_ASSERT_EQUAL_STRING("ok", sensor.latestHumidityStatus());

    // Both role hooks are exposed by the same runtime: TemperatureSensor by role (thermostat
    // binding), humidity as a role-less metric channel.
    TEST_ASSERT_NOT_NULL(sensor.temperatureReadingRuntime());
    TEST_ASSERT_NOT_NULL(sensor.humidityReadingRuntime());

    const DisplayTextPlaceholderSegment temperatureRef{MetricNamespace::Device, 5001, kDeviceMetricTemperature};
    const DisplayTextPlaceholderSegment humidityRef{MetricNamespace::Device, 5001, kDeviceMetricHumidity};
    TEST_ASSERT_TRUE(displayTextPlaceholderSupportsRuntime(sensor, temperatureRef));
    TEST_ASSERT_TRUE(displayTextPlaceholderSupportsRuntime(sensor, humidityRef));

    StaticJsonDocument<1536> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    Htu21SensorDeviceApiAdapter::instance().writeDeviceJson(sensor, sensor.status(), output);
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-htu21.response.schema.json", outputDoc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_STRING("htu21", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(0x41U, output["config"]["i2cAddress"].as<uint8_t>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["temperature"]["valid"].as<bool>());
    TEST_ASSERT_EQUAL_FLOAT(24.536F, output["runtime"]["output"]["temperature"]["value"].as<float>());
    TEST_ASSERT_EQUAL_STRING("C", output["runtime"]["output"]["temperature"]["unitSymbol"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["humidity"]["valid"].as<bool>());
    TEST_ASSERT_EQUAL_FLOAT(48.687F, output["runtime"]["output"]["humidity"]["value"].as<float>());
    TEST_ASSERT_EQUAL_STRING("%", output["runtime"]["output"]["humidity"]["unitSymbol"].as<const char*>());
}

void test_htu21_runtime_applies_channel_filters() {
    FakeHtu21I2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Htu21SensorConfigV3 config = makeHtu21Config();
    config.temperatureFilter.calibrationOffset = 1000.0F; // +1 degC in milli units
    config.humidityFilter.calibrationFactor = 0.5F;
    Htu21SensorDevice sensor(config);
    bindHtu21Dependency(sensor, 5002, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveHtu21UntilReading(sensor);

    TemperatureReading temperature{};
    TEST_ASSERT_TRUE(sensor.latestTemperatureReading(temperature));
    TEST_ASSERT_EQUAL_INT32(25536, temperature.milliCelsius);

    HumidityReading humidity{};
    TEST_ASSERT_TRUE(sensor.latestHumidityReading(humidity));
    TEST_ASSERT_EQUAL_INT32(24344, humidity.milliPercent); // lround(48687 * 0.5)
}

void test_htu21_runtime_crc_failure_invalidates_and_recovers() {
    FakeHtu21I2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Htu21SensorDevice sensor(makeHtu21Config());
    bindHtu21Dependency(sensor, 5003, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveHtu21UntilReading(sensor);
    TEST_ASSERT_TRUE(htu21HasValidReading(sensor));

    driver.corruptHumidityCrc = true;
    uint32_t now = 20000U;
    for (; now < 60000U && htu21HasValidReading(sensor); now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_FALSE(htu21HasValidReading(sensor));
    TEST_ASSERT_EQUAL_STRING("crc_error", sensor.latestHumidityStatus());
    TEST_ASSERT_EQUAL_STRING("crc_error", sensor.latestTemperatureStatus());

    driver.corruptHumidityCrc = false;
    for (; now < 120000U && !htu21HasValidReading(sensor); now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_TRUE(htu21HasValidReading(sensor));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
}

void test_htu21_runtime_retries_when_chip_absent_then_faults_and_recovers() {
    FakeHtu21I2cDriver driver;
    driver.present = false;

    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Htu21SensorDevice sensor(makeHtu21Config());
    bindHtu21Dependency(sensor, 5004, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    sensor.begin(10U);

    uint32_t now = 10U;
    const uint32_t faultDeadline = now + 40000U;
    for (; now < faultDeadline && sensor.status() != DeviceStatus::Faulted; now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(sensor.status()));
    TEST_ASSERT_EQUAL_STRING("not_found", sensor.latestTemperatureStatus());
    TEST_ASSERT_EQUAL_STRING("not_found", sensor.latestHumidityStatus());

    driver.present = true;
    const uint32_t recoveryDeadline = now + 40000U;
    for (; now < recoveryDeadline && !htu21HasValidReading(sensor); now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_TRUE(htu21HasValidReading(sensor));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
}

void test_htu21_runtime_reinitializes_on_dependency_generation_change() {
    FakeHtu21I2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Htu21SensorDevice sensor(makeHtu21Config());
    bindHtu21Dependency(sensor, 5005, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    const uint32_t afterFirstReading = driveHtu21UntilReading(sensor);
    TEST_ASSERT_TRUE(htu21HasValidReading(sensor));

    // A bus reconfigure bumps its generation; the sensor must notice and restart its own cycle
    // instead of continuing against a stale bus session.
    bus.requestReconfigure();
    uint32_t now = afterFirstReading;
    for (const uint32_t deadline = now + 2000U; now < deadline && bus.status() != DeviceStatus::Ready; now += 100U) {
        bus.tick100ms(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));

    bool sawReconfiguring = false;
    for (const uint32_t deadline = now + 10000U; now < deadline; now += 100U) {
        sensor.tick100ms(now);
        if (sensor.status() == DeviceStatus::Reconfiguring) {
            sawReconfiguring = true;
        }
        if (sawReconfiguring && sensor.status() == DeviceStatus::Ready && htu21HasValidReading(sensor)) {
            break;
        }
    }
    TEST_ASSERT_TRUE(sawReconfiguring);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
    TEST_ASSERT_TRUE(htu21HasValidReading(sensor));
}

void test_htu21_adapter_partial_update_preserves_unit_and_deltas() {
    Htu21SensorConfigV3 config = makeHtu21Config(0x41U);
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    config.reportDeltaCentiCelsius = 25U;
    config.reportDeltaCentiPercent = 50U;
    config.temperatureFilter.smoothingWeight = 0.5F;
    Htu21SensorDevice sensor(config);

    StaticJsonDocument<256> inputDoc;
    JsonObject input = inputDoc.to<JsonObject>();
    JsonObject configInput = input.createNestedObject("config");
    configInput["name"] = "climate";
    configInput["pollMs"] = 60000U;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-htu21.request.schema.json", inputDoc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Htu21SensorDeviceApiAdapter::instance().parseUpdateConfigRequest(input, sensor, request, error));

    Htu21SensorConfigV3 updated{};
    TEST_ASSERT_TRUE(decodeHtu21SensorConfig(request.configBlob.data(), request.configBlob.size(), updated));
    TEST_ASSERT_EQUAL_UINT32(60000U, updated.pollMs);
    TEST_ASSERT_EQUAL_UINT8(temperatureUnitToByte(TemperatureUnit::Fahrenheit), updated.outputUnit);
    TEST_ASSERT_EQUAL_UINT16(25U, updated.reportDeltaCentiCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, updated.reportDeltaCentiPercent);
    TEST_ASSERT_EQUAL_FLOAT(0.5F, updated.temperatureFilter.smoothingWeight);
    TEST_ASSERT_EQUAL_UINT8(0x41U, updated.i2cAddress);

    Htu21SensorConfigV3 addressUpdate = config;
    addressUpdate.i2cAddress = 0x42U;
    const BoundedBlob<kMaxDeviceConfigBytes> addressPayload = encodeHtu21Payload(addressUpdate);
    const DeviceConfigUpdatePlan addressPlan = sensor.planConfigUpdate(addressPayload);
    TEST_ASSERT_TRUE(addressPlan.endOldConfig);
    TEST_ASSERT_TRUE(addressPlan.resetStateMachine);
}

void test_htu21_adapter_rejects_duplicate_0x40_dependent_on_same_bus() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(5000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult busAResult = registry.create(makeBusCreateRequest("i2c-a"), 10);
    TEST_ASSERT_TRUE_MESSAGE(busAResult.ok(), busAResult.validation.message);
    DeviceCreateResult busBResult = registry.create(makeBusCreateRequest("i2c-b"), 11);
    TEST_ASSERT_TRUE_MESSAGE(busBResult.ok(), busBResult.validation.message);

    StaticJsonDocument<768> createDoc;
    createDoc["typeName"] = "htu21";
    JsonObject createConfig = createDoc.createNestedObject("config");
    createConfig["name"] = "climate-a";
    createConfig["enabled"] = true;
    JsonArray deps = createConfig.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "i2c_bus";
    dep["deviceId"] = busAResult.deviceId;
    createConfig["i2cAddress"] = 0x40U;
    createConfig["unit"] = "celsius";
    createConfig["pollMs"] = 5000U;
    createConfig["reportAlways"] = false;
    createConfig["reportDeltaCelsius"] = 0.1F;
    createConfig["reportDeltaHumidity"] = 0.1F;
    JsonObject temperatureFilter = createConfig.createNestedObject("temperatureFilter");
    temperatureFilter["smoothingWeight"] = 1.0F;
    temperatureFilter["calibrationFactor"] = 1.0F;
    temperatureFilter["calibrationOffset"] = 0.0F;
    JsonObject humidityFilter = createConfig.createNestedObject("humidityFilter");
    humidityFilter["smoothingWeight"] = 1.0F;
    humidityFilter["calibrationFactor"] = 1.0F;
    humidityFilter["calibrationOffset"] = 0.0F;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-htu21.request.schema.json", createDoc.as<JsonVariantConst>());

    DeviceCreateRequest createRequest{};
    const char* createError = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Htu21SensorDeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), createRequest, createError),
        createError);

    const DeviceCreateResult first = registry.create(createRequest, 20);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);

    // The default address is 0x40, so a second default-configured HTU21 on the same bus collides.
    const DeviceCreateRequest duplicate = makeHtu21CreateRequest("climate-b", busAResult.deviceId);
    const DeviceValidationResult duplicateValidation = Htu21SensorDeviceApiAdapter::instance().validateCreateRequest(duplicate, registry);
    TEST_ASSERT_FALSE(duplicateValidation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(duplicateValidation.error));

    // A second HTU21 on a different bus is fine.
    const DeviceCreateRequest otherBus = makeHtu21CreateRequest("climate-c", busBResult.deviceId);
    const DeviceValidationResult otherBusValidation = Htu21SensorDeviceApiAdapter::instance().validateCreateRequest(otherBus, registry);
    TEST_ASSERT_TRUE_MESSAGE(otherBusValidation.ok(), otherBusValidation.message);

    // Cross-type collision: any other device configured at 0x40 on the HTU21's bus is rejected
    // through the same bus-wide duplicate-address walk.
    Ds3231RtcDeviceConfigV2 rtcConfig{};
    rtcConfig.enabled = 1U;
    std::snprintf(rtcConfig.name, sizeof(rtcConfig.name), "%s", "rtc");
    rtcConfig.i2cAddress = kHtu21DefaultI2cAddress;
    DeviceCreateRequest rtcRequest{};
    rtcRequest.typeId = kDs3231RtcTypeId;
    TEST_ASSERT_TRUE(rtcRequest.assignName("rtc"));
    rtcRequest.setEnabled(true);
    rtcRequest.depCount = 1;
    rtcRequest.deps[0] = {DeviceRole::I2CBus, busAResult.deviceId};
    rtcRequest.configVersion = kDs3231RtcConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ds3231RtcDeviceConfigV2::kMagic, rtcConfig, buffer, ds3231RtcDeviceConfigSize(rtcConfig)));
    TEST_ASSERT_TRUE(rtcRequest.configBlob.assign(buffer, ds3231RtcDeviceConfigSize(rtcConfig)));
    const DeviceValidationResult rtcValidation = Ds3231RtcDeviceApiAdapter::instance().validateCreateRequest(rtcRequest, registry);
    TEST_ASSERT_FALSE(rtcValidation.ok());
}
