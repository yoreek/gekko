#include "JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayTextPlaceholderAst.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/aht10/Aht10Protocol.h"
#include "devices/sensors/aht10/Aht10SensorDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/aht10/Aht10SensorDeviceApiAdapter.h"
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

// Simulates the AHT10 frame protocol over the II2cBusDriver primitives: a 3-byte command write
// selects init or measurement, and the following 6-byte request returns status + 20-bit humidity
// + 20-bit temperature packed exactly like the sensor.
class FakeAht10I2cDriver final : public II2cBusDriver {
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
        const uint8_t status = busy ? kAht10StatusBusy : static_cast<uint8_t>(calibrated ? kAht10StatusCalibrated : 0U);
        readBuffer.push_back(status);
        readBuffer.push_back(static_cast<uint8_t>((humidityRaw >> 12) & 0xFFU));
        readBuffer.push_back(static_cast<uint8_t>((humidityRaw >> 4) & 0xFFU));
        readBuffer.push_back(static_cast<uint8_t>(((humidityRaw & 0x0FU) << 4) | ((temperatureRaw >> 16) & 0x0FU)));
        readBuffer.push_back(static_cast<uint8_t>((temperatureRaw >> 8) & 0xFFU));
        readBuffer.push_back(static_cast<uint8_t>(temperatureRaw & 0xFFU));
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
    bool busy{false};
    bool calibrated{true};
    uint32_t temperatureRaw{0x80000U}; // 50000 milli-degC
    uint32_t humidityRaw{0x80000U};    // 50000 milli-%RH
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

Aht10SensorConfigV1 makeAht10Config(uint8_t i2cAddress = kAht10DefaultI2cAddress) {
    Aht10SensorConfigV1 config{};
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

template <typename Config> BoundedBlob<kMaxDeviceConfigBytes> encodeAht10Payload(const Config& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Config::kMagic, config, buffer, aht10SensorConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, aht10SensorConfigSize(config)));
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

void bindAht10Dependency(Aht10SensorDevice& sensor, DeviceId sensorId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = sensorId;
    record.header.typeId = kAht10SensorTypeId;
    record.header.configVersion = kAht10SensorConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::I2CBus, busId};
    record.status = DeviceStatus::Ready;
    sensor.bindDeviceIdentity(record, encodeAht10Payload(sensor.config()));
}

bool aht10HasValidReading(const Aht10SensorDevice& sensor) {
    TemperatureReading temperature{};
    HumidityReading humidity{};
    (void)sensor.latestTemperatureReading(temperature);
    (void)sensor.latestHumidityReading(humidity);
    return temperature.valid && humidity.valid;
}

uint32_t driveAht10UntilReading(Aht10SensorDevice& sensor, uint32_t startNow = 10U) {
    sensor.begin(startNow);
    uint32_t now = startNow + 1U;
    for (; now < startNow + 10000U && !aht10HasValidReading(sensor); now += 100U) {
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

DeviceCreateRequest makeAht10CreateRequest(const char* name, DeviceId busId) {
    DeviceCreateRequest request{};
    request.typeId = kAht10SensorTypeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::I2CBus, busId};
    request.configVersion = kAht10SensorConfigVersion;
    request.configBlob = encodeAht10Payload(makeAht10Config());
    return request;
}

} // namespace

void test_aht10_protocol_crc_and_conversions() {
    const uint32_t raw = 0x80000U;
    const uint8_t frame[] = {kAht10StatusCalibrated, 0x80U, 0x00U, 0x08U, 0x00U, 0x00U};
    uint32_t humidityRaw = 0U;
    uint32_t temperatureRaw = 0U;
    const char* error = nullptr;
    TEST_ASSERT_TRUE(aht10DecodeMeasurement(frame, humidityRaw, temperatureRaw, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_UINT32(raw, humidityRaw);
    TEST_ASSERT_EQUAL_UINT32(raw, temperatureRaw);
    TEST_ASSERT_TRUE(aht10MeasurementReady(kAht10StatusCalibrated));
    TEST_ASSERT_TRUE(aht10MeasurementCalibrated(kAht10StatusCalibrated));
    TEST_ASSERT_FALSE(aht10MeasurementReady(kAht10StatusBusy));
    TEST_ASSERT_FALSE(aht10MeasurementCalibrated(0U));
    TEST_ASSERT_EQUAL_INT32(50000, aht10RawToMilliCelsius(raw));
    TEST_ASSERT_EQUAL_INT32(50000, aht10RawToMilliPercent(raw));
    TEST_ASSERT_EQUAL_INT32(0, aht10RawToMilliPercent(0U));
    TEST_ASSERT_EQUAL_INT32(99999, aht10RawToMilliPercent(0xFFFFFU));
}

void test_aht10_config_codec_json_and_validation() {
    Aht10SensorConfigV1 config = makeAht10Config(0x41U);
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    config.pollMs = 30000U;
    config.reportDeltaCentiCelsius = 25U;
    config.reportDeltaCentiPercent = 50U;
    config.temperatureFilter.smoothingWeight = 0.5F;
    config.humidityFilter.calibrationOffset = -1500.0F;
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeAht10Payload(config);

    Aht10SensorConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeAht10SensorConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(0x41U, decoded.i2cAddress);
    TEST_ASSERT_EQUAL_UINT32(30000U, decoded.pollMs);
    TEST_ASSERT_EQUAL_UINT16(25U, decoded.reportDeltaCentiCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, decoded.reportDeltaCentiPercent);
    TEST_ASSERT_EQUAL_FLOAT(0.5F, decoded.temperatureFilter.smoothingWeight);
    TEST_ASSERT_EQUAL_FLOAT(-1500.0F, decoded.humidityFilter.calibrationOffset);
    TEST_ASSERT_EQUAL_STRING("climate", decoded.name);

    StaticJsonDocument<768> doc;
    JsonObject json = doc.to<JsonObject>();
    writeAht10SensorConfigJson(config, json);
    TEST_ASSERT_EQUAL_STRING("fahrenheit", json["unit"].as<const char*>());
    TEST_ASSERT_EQUAL_FLOAT(0.25F, json["reportDeltaCelsius"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(0.5F, json["reportDeltaHumidity"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(0.5F, json["temperatureFilter"]["smoothingWeight"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(-1500.0F, json["humidityFilter"]["calibrationOffset"].as<float>());
    TEST_ASSERT_EQUAL_UINT8(0x41U, json["i2cAddress"].as<uint8_t>());
    assertMatchesJsonSchema("schemas/rest/v1/devices/aht10.config.schema.json", doc.as<JsonVariantConst>());

    Aht10SensorConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parseAht10SensorConfigJson(json, parsed, error));
    TEST_ASSERT_EQUAL_UINT32(30000U, parsed.pollMs);
    TEST_ASSERT_EQUAL_UINT16(25U, parsed.reportDeltaCentiCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, parsed.reportDeltaCentiPercent);
    TEST_ASSERT_EQUAL_FLOAT(0.5F, parsed.temperatureFilter.smoothingWeight);
    TEST_ASSERT_EQUAL_FLOAT(-1500.0F, parsed.humidityFilter.calibrationOffset);
    TEST_ASSERT_EQUAL_UINT8(0x41U, parsed.i2cAddress);
    TEST_ASSERT_TRUE(parsed.validate().ok());

    Aht10SensorConfigV1 badPoll = makeAht10Config();
    badPoll.pollMs = 100U;
    TEST_ASSERT_FALSE(badPoll.validate().ok());

    Aht10SensorConfigV1 badDelta = makeAht10Config();
    badDelta.reportDeltaCentiPercent = 0U;
    TEST_ASSERT_FALSE(badDelta.validate().ok());

    Aht10SensorConfigV1 badFilter = makeAht10Config();
    badFilter.humidityFilter.smoothingWeight = 0.0F;
    TEST_ASSERT_FALSE(badFilter.validate().ok());

    Aht10SensorConfigV1 badAddress = makeAht10Config(0x80U);
    TEST_ASSERT_FALSE(badAddress.validate().ok());

    EWFM_LEGACY_CONFIG_USE_BEGIN
    Aht10SensorConfigV1 legacy{};
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
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Aht10SensorConfigV1::kMagic, legacy, legacyBuffer, aht10SensorConfigSize(legacy)));
    EWFM_LEGACY_CONFIG_USE_END
    Aht10SensorConfigV1 migrated{};
    TEST_ASSERT_TRUE(decodeAht10SensorConfig(legacyBuffer, aht10SensorConfigSize(legacy), migrated));
    TEST_ASSERT_EQUAL_UINT8(kAht10DefaultI2cAddress, migrated.i2cAddress);
}

void test_aht10_registry_loads_v1_blob_on_begin() {
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
    sensorRecord.header.typeId = kAht10SensorTypeId;
    sensorRecord.header.configVersion = 1U;
    sensorRecord.header.configRevision = 1U;
    sensorRecord.depCount = 1U;
    sensorRecord.deps[0] = {DeviceRole::I2CBus, busRecord.header.deviceId};
    sensorRecord.status = DeviceStatus::Ready;

    EWFM_LEGACY_CONFIG_USE_BEGIN
    Aht10SensorConfigV1 legacy{};
    legacy.enabled = 1U;
    std::snprintf(legacy.name, sizeof(legacy.name), "%s", "legacy-climate");
    uint8_t legacyBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Aht10SensorConfigV1::kMagic, legacy, legacyBuffer, aht10SensorConfigSize(legacy)));
    EWFM_LEGACY_CONFIG_USE_END
    DeviceConfigBlob legacyBlob{};
    TEST_ASSERT_TRUE(legacyBlob.assign(legacyBuffer, aht10SensorConfigSize(legacy)));

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

    const Aht10SensorDevice* runtime = static_cast<const Aht10SensorDevice*>(registry.runtime(sensorRecord.header.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_UINT32(kAht10SensorConfigVersion, runtime->configVersion());
    TEST_ASSERT_EQUAL_UINT8(kAht10DefaultI2cAddress, runtime->config().i2cAddress);
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());
}

void test_aht10_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kAht10SensorTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::TemperatureSensor));
    TEST_ASSERT_EQUAL(1, static_cast<int>(descriptor->dependencyRequirements.size()));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceRole::I2CBus), static_cast<int>(descriptor->dependencyRequirements[0].role));
    TEST_ASSERT_TRUE(descriptor->dependencyRequirements[0].required);

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(kAht10SensorTypeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("aht10"));
}

void test_aht10_runtime_measures_temperature_and_humidity() {
    FakeAht10I2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    bindBusIdentity(bus, 6001U);
    driveBusReady(bus);

    Aht10SensorDevice sensor(makeAht10Config(0x41U));
    bindAht10Dependency(sensor, 5001, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveAht10UntilReading(sensor);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
    TEST_ASSERT_EQUAL_UINT8(0x41U, driver.lastAddress);

    TemperatureReading temperature{};
    TEST_ASSERT_TRUE(sensor.latestTemperatureReading(temperature));
    TEST_ASSERT_TRUE(temperature.valid);
    TEST_ASSERT_EQUAL_INT32(50000, temperature.milliCelsius);
    TEST_ASSERT_EQUAL_STRING("ok", sensor.latestTemperatureStatus());

    HumidityReading humidity{};
    TEST_ASSERT_TRUE(sensor.latestHumidityReading(humidity));
    TEST_ASSERT_TRUE(humidity.valid);
    TEST_ASSERT_EQUAL_INT32(50000, humidity.milliPercent);
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
    Aht10SensorDeviceApiAdapter::instance().writeDeviceJson(sensor, sensor.status(), output);
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-aht10.response.schema.json", outputDoc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_STRING("aht10", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(0x41U, output["config"]["i2cAddress"].as<uint8_t>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["temperature"]["valid"].as<bool>());
    TEST_ASSERT_EQUAL_FLOAT(50.0F, output["runtime"]["output"]["temperature"]["value"].as<float>());
    TEST_ASSERT_EQUAL_STRING("C", output["runtime"]["output"]["temperature"]["unitSymbol"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["humidity"]["valid"].as<bool>());
    TEST_ASSERT_EQUAL_FLOAT(50.0F, output["runtime"]["output"]["humidity"]["value"].as<float>());
    TEST_ASSERT_EQUAL_STRING("%", output["runtime"]["output"]["humidity"]["unitSymbol"].as<const char*>());
}

void test_aht10_runtime_applies_channel_filters() {
    FakeAht10I2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Aht10SensorConfigV1 config = makeAht10Config();
    config.temperatureFilter.calibrationOffset = 1000.0F; // +1 degC in milli units
    config.humidityFilter.calibrationFactor = 0.5F;
    Aht10SensorDevice sensor(config);
    bindAht10Dependency(sensor, 5002, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveAht10UntilReading(sensor);

    TemperatureReading temperature{};
    TEST_ASSERT_TRUE(sensor.latestTemperatureReading(temperature));
    TEST_ASSERT_EQUAL_INT32(51000, temperature.milliCelsius);

    HumidityReading humidity{};
    TEST_ASSERT_TRUE(sensor.latestHumidityReading(humidity));
    TEST_ASSERT_EQUAL_INT32(25000, humidity.milliPercent); // lround(50000 * 0.5)
}

void test_aht10_runtime_uncalibrated_invalidates_and_recovers() {
    FakeAht10I2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Aht10SensorDevice sensor(makeAht10Config());
    bindAht10Dependency(sensor, 5003, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveAht10UntilReading(sensor);
    TEST_ASSERT_TRUE(aht10HasValidReading(sensor));

    driver.calibrated = false;
    uint32_t now = 20000U;
    for (; now < 60000U && aht10HasValidReading(sensor); now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_FALSE(aht10HasValidReading(sensor));
    TEST_ASSERT_EQUAL_STRING("uncalibrated", sensor.latestHumidityStatus());
    TEST_ASSERT_EQUAL_STRING("uncalibrated", sensor.latestTemperatureStatus());

    driver.calibrated = true;
    for (; now < 120000U && !aht10HasValidReading(sensor); now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_TRUE(aht10HasValidReading(sensor));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
}

void test_aht10_runtime_retries_when_chip_absent_then_faults_and_recovers() {
    FakeAht10I2cDriver driver;
    driver.present = false;

    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Aht10SensorDevice sensor(makeAht10Config());
    bindAht10Dependency(sensor, 5004, bus.deviceId());
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
    for (; now < recoveryDeadline && !aht10HasValidReading(sensor); now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_TRUE(aht10HasValidReading(sensor));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
}

void test_aht10_runtime_reinitializes_on_dependency_generation_change() {
    FakeAht10I2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Aht10SensorDevice sensor(makeAht10Config());
    bindAht10Dependency(sensor, 5005, bus.deviceId());
    sensor.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    const uint32_t afterFirstReading = driveAht10UntilReading(sensor);
    TEST_ASSERT_TRUE(aht10HasValidReading(sensor));

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
        if (sawReconfiguring && sensor.status() == DeviceStatus::Ready && aht10HasValidReading(sensor)) {
            break;
        }
    }
    TEST_ASSERT_TRUE(sawReconfiguring);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
    TEST_ASSERT_TRUE(aht10HasValidReading(sensor));
}

void test_aht10_adapter_partial_update_preserves_unit_and_deltas() {
    Aht10SensorConfigV1 config = makeAht10Config(0x41U);
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    config.reportDeltaCentiCelsius = 25U;
    config.reportDeltaCentiPercent = 50U;
    config.temperatureFilter.smoothingWeight = 0.5F;
    Aht10SensorDevice sensor(config);

    StaticJsonDocument<256> inputDoc;
    JsonObject input = inputDoc.to<JsonObject>();
    JsonObject configInput = input.createNestedObject("config");
    configInput["name"] = "climate";
    configInput["pollMs"] = 60000U;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-aht10.request.schema.json", inputDoc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Aht10SensorDeviceApiAdapter::instance().parseUpdateConfigRequest(input, sensor, request, error));

    Aht10SensorConfigV1 updated{};
    TEST_ASSERT_TRUE(decodeAht10SensorConfig(request.configBlob.data(), request.configBlob.size(), updated));
    TEST_ASSERT_EQUAL_UINT32(60000U, updated.pollMs);
    TEST_ASSERT_EQUAL_UINT8(temperatureUnitToByte(TemperatureUnit::Fahrenheit), updated.outputUnit);
    TEST_ASSERT_EQUAL_UINT16(25U, updated.reportDeltaCentiCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, updated.reportDeltaCentiPercent);
    TEST_ASSERT_EQUAL_FLOAT(0.5F, updated.temperatureFilter.smoothingWeight);
    TEST_ASSERT_EQUAL_UINT8(0x41U, updated.i2cAddress);

    Aht10SensorConfigV1 addressUpdate = config;
    addressUpdate.i2cAddress = 0x42U;
    const BoundedBlob<kMaxDeviceConfigBytes> addressPayload = encodeAht10Payload(addressUpdate);
    const DeviceConfigUpdatePlan addressPlan = sensor.planConfigUpdate(addressPayload);
    TEST_ASSERT_TRUE(addressPlan.endOldConfig);
    TEST_ASSERT_TRUE(addressPlan.resetStateMachine);
}

void test_aht10_adapter_rejects_duplicate_0x38_dependent_on_same_bus() {
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
    createDoc["typeName"] = "aht10";
    JsonObject createConfig = createDoc.createNestedObject("config");
    createConfig["name"] = "climate-a";
    createConfig["enabled"] = true;
    JsonArray deps = createConfig.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "i2c_bus";
    dep["deviceId"] = busAResult.deviceId;
    createConfig["i2cAddress"] = 0x38U;
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
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-aht10.request.schema.json", createDoc.as<JsonVariantConst>());

    DeviceCreateRequest createRequest{};
    const char* createError = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Aht10SensorDeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), createRequest, createError),
        createError);

    const DeviceCreateResult first = registry.create(createRequest, 20);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);

    // The default address is 0x38, so a second default-configured AHT10 on the same bus collides.
    const DeviceCreateRequest duplicate = makeAht10CreateRequest("climate-b", busAResult.deviceId);
    const DeviceValidationResult duplicateValidation = Aht10SensorDeviceApiAdapter::instance().validateCreateRequest(duplicate, registry);
    TEST_ASSERT_FALSE(duplicateValidation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(duplicateValidation.error));

    // A second AHT10 on a different bus is fine.
    const DeviceCreateRequest otherBus = makeAht10CreateRequest("climate-c", busBResult.deviceId);
    const DeviceValidationResult otherBusValidation = Aht10SensorDeviceApiAdapter::instance().validateCreateRequest(otherBus, registry);
    TEST_ASSERT_TRUE_MESSAGE(otherBusValidation.ok(), otherBusValidation.message);

    // Cross-type collision: any other device configured at 0x38 on the AHT10's bus is rejected
    // through the same bus-wide duplicate-address walk.
    Ds3231RtcDeviceConfigV2 rtcConfig{};
    rtcConfig.enabled = 1U;
    std::snprintf(rtcConfig.name, sizeof(rtcConfig.name), "%s", "rtc");
    rtcConfig.i2cAddress = kAht10DefaultI2cAddress;
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
