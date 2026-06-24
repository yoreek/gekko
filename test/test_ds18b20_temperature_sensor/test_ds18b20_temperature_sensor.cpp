#include "config/MemoryConfigStorage.h"
#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/ds18b20/Ds18b20TemperatureSensorDeviceApiAdapter.h"
#include "portal/ws/PortalWebSocketMessages.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

class FakeOneWireBusDriver final : public IOneWireBusDriver {
public:
    bool begin(uint8_t pin, bool internalPullup) override {
        lastPin = pin;
        lastInternalPullup = internalPullup;
        began = true;
        return beginOk;
    }

    void depower() override {
        depowered = true;
    }

    bool reset() override {
        ++resetCount;
        return resetOk;
    }

    void resetSearch() override {
        searchIndex = 0;
    }

    bool search(OneWireRomAddress& address) override {
        if (searchIndex >= candidates.size()) {
            return false;
        }
        address = candidates[searchIndex++];
        return true;
    }

    void select(const OneWireRomAddress& address) override {
        selected = address;
        selectedSeen = true;
    }

    void skip() override {
        skipSeen = true;
    }

    void write(uint8_t value, bool power = false) override {
        (void)power;
        writes.push_back(value);
        if (value == kDs18b20CommandReadScratchpad) {
            readIndex = 0;
            return;
        }
        if (value == kDs18b20CommandConvertT) {
            ++conversionCount;
            return;
        }
        if (value == kDs18b20CommandWriteScratchpad) {
            writeScratchpadBytesRemaining = 3;
            return;
        }
        if (writeScratchpadBytesRemaining == 3) {
            scratchpad[2] = value;
            --writeScratchpadBytesRemaining;
            return;
        }
        if (writeScratchpadBytesRemaining == 2) {
            scratchpad[3] = value;
            --writeScratchpadBytesRemaining;
            return;
        }
        if (writeScratchpadBytesRemaining == 1) {
            scratchpad[4] = value;
            --writeScratchpadBytesRemaining;
            updateScratchpadCrc();
        }
    }

    uint8_t read() override {
        if (readIndex >= kDs18b20ScratchpadSize) {
            return 0;
        }
        return scratchpad[readIndex++];
    }

    uint8_t readBit() override {
        return 1;
    }

    uint8_t crc8(const uint8_t* data, size_t len) const override {
        return oneWireCrc8(data, len);
    }

    void setTemperatureRaw(int16_t raw, uint8_t resolution = 12) {
        scratchpad[0] = static_cast<uint8_t>(raw & 0xFF);
        scratchpad[1] = static_cast<uint8_t>((raw >> 8) & 0xFF);
        scratchpad[2] = 0x4B;
        scratchpad[3] = 0x46;
        scratchpad[4] = ds18b20ResolutionConfigByte(resolution);
        scratchpad[5] = 0xFF;
        scratchpad[6] = 0x0C;
        scratchpad[7] = 0x10;
        updateScratchpadCrc();
    }

    void corruptScratchpadCrc() {
        scratchpad[8] ^= 0xFFU;
    }

    void updateScratchpadCrc() {
        scratchpad[8] = oneWireCrc8(scratchpad, kDs18b20ScratchpadSize - 1U);
    }

    bool beginOk{true};
    bool resetOk{true};
    bool began{false};
    bool depowered{false};
    bool lastInternalPullup{false};
    bool selectedSeen{false};
    bool skipSeen{false};
    uint8_t lastPin{0};
    uint32_t resetCount{0};
    uint32_t conversionCount{0};
    size_t searchIndex{0};
    size_t readIndex{0};
    uint8_t writeScratchpadBytesRemaining{0};
    OneWireRomAddress selected{};
    uint8_t scratchpad[kDs18b20ScratchpadSize]{};
    std::vector<uint8_t> writes{};
    std::vector<OneWireRomAddress> candidates{};
};

BoundedBlob<kMaxDeviceConfigBytes> encodeDs18b20Payload(const Ds18b20TemperatureSensorConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeDs18b20TemperatureSensorConfig(config, buffer, ds18b20TemperatureSensorConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, ds18b20TemperatureSensorConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeOneWirePayload(const OneWireBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeOneWireBusDeviceConfig(config, buffer, oneWireBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, oneWireBusDeviceConfigSize(config)));
    return payload;
}

OneWireRomAddress makeRom(uint8_t serial0 = 0xFF) {
    OneWireRomAddress address{};
    address.bytes[0] = kDs18b20FamilyCode;
    address.bytes[1] = serial0;
    address.bytes[2] = 0x64;
    address.bytes[3] = 0x1D;
    address.bytes[4] = 0x62;
    address.bytes[5] = 0x16;
    address.bytes[6] = 0x03;
    address.bytes[7] = oneWireCrc8(address.bytes, 7);
    return address;
}

OneWireBusDeviceConfigV1 makeBusConfig() {
    OneWireBusDeviceConfigV1 config{};
    config.base.enabled = 1;
    std::snprintf(config.base.name, sizeof(config.base.name), "%s", "onewire");
    config.gpioPin = 4;
    config.internalPullup = 0;
    return config;
}

Ds18b20TemperatureSensorConfigV1 makeSensorConfig(uint8_t serial0 = 0xFF) {
    Ds18b20TemperatureSensorConfigV1 config{};
    config.base.enabled = 1;
    std::snprintf(config.base.name, sizeof(config.base.name), "%s", "temperature");
    config.address = makeRom(serial0);
    config.resolution = 12;
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Celsius);
    config.reportAlways = 0;
    config.reportDeltaCentiCelsius = 1;
    config.pollMs = 1000;
    return config;
}

void driveDependencyReady(OneWireBusDevice& bus) {
    bus.begin(1);
    bus.tick100ms(2);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
}

void driveSensorUntilReading(Ds18b20TemperatureSensorDevice& sensor, uint32_t startNow = 10) {
    sensor.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 2200U && !sensor.reading().valid; now += 50U) {
        sensor.tick100ms(now);
    }
}

void bindSensorDependency(Ds18b20TemperatureSensorDevice& sensor, DeviceId sensorId, DeviceId dependencyId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = sensorId;
    record.header.typeId = kDs18b20TemperatureSensorTypeId;
    record.header.configVersion = kDs18b20TemperatureSensorConfigVersion;
    record.header.configRevision = 1;
    record.header.payloadLength = static_cast<uint32_t>(encodeDs18b20Payload(sensor.config()).size());
    record.depCount = 1;
    record.deps[0] = {DeviceDependencyRole::OneWireBus, dependencyId};
    record.status = DeviceStatus::Ready;
    sensor.bindDeviceIdentity(record, encodeDs18b20Payload(sensor.config()));
}

void tickSensorUntilMeasuredAtChanges(Ds18b20TemperatureSensorDevice& sensor, uint32_t previousMeasuredAt, uint32_t startNow) {
    for (uint32_t now = startNow; now < startNow + 2200U && sensor.reading().measuredAtMs == previousMeasuredAt; now += 50U) {
        sensor.tick100ms(now);
    }
}

DeviceRegistryEntry makeSensorRecord(DeviceId id, DeviceId dependencyId, const Ds18b20TemperatureSensorConfigV1& config) {
    const BoundedBlob<kMaxDeviceConfigBytes> configBlob = encodeDs18b20Payload(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = id;
    record.header.typeId = kDs18b20TemperatureSensorTypeId;
    record.header.configVersion = kDs18b20TemperatureSensorConfigVersion;
    record.header.configRevision = 1;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.depCount = 1;
    record.deps[0] = {DeviceDependencyRole::OneWireBus, dependencyId};
    record.status = DeviceStatus::Ready;
    return record;
}

DeviceCreateRequest makeBusCreateRequest(const char* name) {
    DeviceCreateRequest request{};
    request.typeId = OneWireBusDevice::descriptor().typeId;
    request.name = name;
    request.enabled = true;
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeOneWirePayload(makeBusConfig());
    return request;
}

DeviceCreateRequest makeSensorCreateRequest(const char* name, DeviceId dependencyId, const Ds18b20TemperatureSensorConfigV1& config) {
    DeviceCreateRequest request{};
    request.typeId = kDs18b20TemperatureSensorTypeId;
    request.name = name;
    request.enabled = true;
    request.depCount = 1;
    request.deps[0] = {DeviceDependencyRole::OneWireBus, dependencyId};
    request.configVersion = kDs18b20TemperatureSensorConfigVersion;
    request.configBlob = encodeDs18b20Payload(config);
    return request;
}

} // namespace

void test_temperature_helpers_convert_units_and_dirty_threshold() {
    TemperatureReading previous{};
    TemperatureReading current{};
    current.valid = true;
    current.milliCelsius = 23625;
    current.measuredAtMs = 42;

    TEST_ASSERT_TRUE(temperatureReadingChanged(previous, current, 1));
    previous = current;
    current.milliCelsius = 23629;
    TEST_ASSERT_FALSE(temperatureReadingChanged(previous, current, 1));
    current.milliCelsius = 23635;
    TEST_ASSERT_TRUE(temperatureReadingChanged(previous, current, 1));
    TEST_ASSERT_EQUAL_INT32(74525, convertMilliCelsiusToUnit(23625, TemperatureUnit::Fahrenheit));
}

void test_ds18b20_config_codec_json_and_validation() {
    Ds18b20TemperatureSensorConfigV1 config = makeSensorConfig();
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    config.reportAlways = 1;
    config.reportDeltaCentiCelsius = 25;
    const BoundedBlob<kMaxDeviceConfigBytes> blob = encodeDs18b20Payload(config);

    Ds18b20TemperatureSensorConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeDs18b20TemperatureSensorConfig(reinterpret_cast<const uint8_t*>(blob.data()), blob.size(), decoded));
    TEST_ASSERT_EQUAL_MEMORY(config.address.bytes, decoded.address.bytes, sizeof(config.address.bytes));
    TEST_ASSERT_EQUAL_UINT8(12, decoded.resolution);
    TEST_ASSERT_EQUAL_UINT8(temperatureUnitToByte(TemperatureUnit::Fahrenheit), decoded.outputUnit);

    StaticJsonDocument<384> doc;
    JsonObject json = doc.to<JsonObject>();
    writeDs18b20TemperatureSensorConfigJson(config, json);
    TEST_ASSERT_EQUAL_STRING("fahrenheit", json["unit"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("28FF641D621603AD", json["address"].as<const char*>());

    Ds18b20TemperatureSensorConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parseDs18b20TemperatureSensorConfigJson(json, parsed, error));
    TEST_ASSERT_EQUAL_UINT16(25, parsed.reportDeltaCentiCelsius);

    json["resolution"] = 8;
    TEST_ASSERT_FALSE(parseDs18b20TemperatureSensorConfigJson(json, parsed, error));
    TEST_ASSERT_NOT_NULL(error);
}

void test_ds18b20_protocol_helpers_parse_scratchpad() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 12);

    int32_t milliCelsius = 0;
    TEST_ASSERT_TRUE(ds18b20ScratchpadCrcValid(driver.scratchpad));
    TEST_ASSERT_TRUE(ds18b20ParseScratchpadTemperature(driver.scratchpad, milliCelsius));
    TEST_ASSERT_EQUAL_INT32(23500, milliCelsius);
    TEST_ASSERT_EQUAL_UINT16(94, ds18b20ConversionTimeMs(9));
    TEST_ASSERT_EQUAL_UINT16(750, ds18b20ConversionTimeMs(12));

    driver.corruptScratchpadCrc();
    TEST_ASSERT_FALSE(ds18b20ScratchpadCrcValid(driver.scratchpad));
}

void test_ds18b20_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kDs18b20TemperatureSensorTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("Ds18b20TemperatureSensorDevice", descriptor->name);
    TEST_ASSERT_FALSE(descriptor->supportsRetainedState);
    TEST_ASSERT_TRUE(descriptor->ticks100ms);
    TEST_ASSERT_EQUAL_UINT32(1, descriptor->dependencyRequirements.size());
    TEST_ASSERT_EQUAL_UINT32(OneWireBusDevice::descriptor().typeId, descriptor->dependencyRequirements[0].compatibleTypeIds[0]);

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(kDs18b20TemperatureSensorTypeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("ds18b20_temperature_sensor"));
}

void test_ds18b20_api_adapter_parses_create_update_and_rejects_invalid_input() {
    StaticJsonDocument<512> doc;
    doc["typeId"] = kDs18b20TemperatureSensorTypeId;
    doc["name"] = "temperature";
    doc["enabled"] = true;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "onewire_bus";
    dep["deviceId"] = 44;
    JsonObject config = doc.createNestedObject("config");
    config["address"] = "28FF641D621603AD";
    config["resolution"] = 11;
    config["unit"] = "fahrenheit";
    config["pollMs"] = 2000;
    config["reportDeltaCentiCelsius"] = 25;
    config["reportAlways"] = true;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Ds18b20TemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(kDs18b20TemperatureSensorTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT8(1, request.depCount);
    TEST_ASSERT_EQUAL_UINT32(44, request.deps[0].deviceId);
    Ds18b20TemperatureSensorConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeDs18b20TemperatureSensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                          request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(11, parsed.resolution);
    TEST_ASSERT_EQUAL_UINT8(temperatureUnitToByte(TemperatureUnit::Fahrenheit), parsed.outputUnit);
    TEST_ASSERT_EQUAL_UINT16(25, parsed.reportDeltaCentiCelsius);
    TEST_ASSERT_TRUE(parsed.reportAlways != 0U);

    DeviceRegistryEntry record = makeSensorRecord(50, 44, parsed);
    Ds18b20TemperatureSensorDevice runtime(record, encodeDs18b20Payload(parsed));
    StaticJsonDocument<512> updateDoc;
    updateDoc["command"] = "update_config";
    JsonArray updateDeps = updateDoc.createNestedArray("deps");
    JsonObject updateDep = updateDeps.createNestedObject();
    updateDep["role"] = "onewire_bus";
    updateDep["deviceId"] = 45;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["address"] = "28FF641D621603AD";
    updateConfig["resolution"] = 12;
    updateConfig["unit"] = "celsius";
    updateConfig["pollMs"] = 3000;
    updateConfig["reportDeltaCentiCelsius"] = 2;
    updateConfig["reportAlways"] = false;
    DeviceConfigUpdateRequest updateRequest{};
    error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(Ds18b20TemperatureSensorDeviceApiAdapter::instance().parseUpdateConfigRequest(updateDoc.as<JsonObjectConst>(),
                                                                                                           runtime, updateRequest, error),
                             error);
    TEST_ASSERT_TRUE(updateRequest.depsProvided);
    TEST_ASSERT_EQUAL_UINT8(1, updateRequest.depCount);
    TEST_ASSERT_EQUAL_UINT32(45, updateRequest.deps[0].deviceId);
    TEST_ASSERT_TRUE(decodeDs18b20TemperatureSensorConfig(reinterpret_cast<const uint8_t*>(updateRequest.configBlob.data()),
                                                          updateRequest.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(12, parsed.resolution);
    TEST_ASSERT_EQUAL_UINT16(2, parsed.reportDeltaCentiCelsius);

    StaticJsonDocument<128> missingUpdateConfigDoc;
    missingUpdateConfigDoc["command"] = "update_config";
    DeviceConfigUpdateRequest missingUpdateRequest{};
    TEST_ASSERT_FALSE(Ds18b20TemperatureSensorDeviceApiAdapter::instance().parseUpdateConfigRequest(
        missingUpdateConfigDoc.as<JsonObjectConst>(), runtime, missingUpdateRequest, error));
    TEST_ASSERT_NOT_NULL(error);

    StaticJsonDocument<256> missingDependencyDoc;
    missingDependencyDoc["name"] = "bad";
    JsonObject missingConfig = missingDependencyDoc.createNestedObject("config");
    missingConfig["address"] = "28FF641D621603AD";
    TEST_ASSERT_FALSE(Ds18b20TemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(missingDependencyDoc.as<JsonObjectConst>(),
                                                                                              request, error));

    StaticJsonDocument<256> badAddressDoc;
    badAddressDoc["name"] = "bad-address";
    JsonArray badDeps = badAddressDoc.createNestedArray("deps");
    JsonObject badDep = badDeps.createNestedObject();
    badDep["role"] = "onewire_bus";
    badDep["deviceId"] = 44;
    JsonObject badConfig = badAddressDoc.createNestedObject("config");
    badConfig["address"] = "10FF641D6216037B";
    TEST_ASSERT_FALSE(
        Ds18b20TemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(badAddressDoc.as<JsonObjectConst>(), request, error));
}

void test_ds18b20_runtime_reads_addressed_temperature_and_configures_resolution() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 9);
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);

    Ds18b20TemperatureSensorDevice sensor(makeSensorConfig());
    bindSensorDependency(sensor, 1001, bus.deviceId());
    sensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    driveSensorUntilReading(sensor);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sensor.status()));
    TEST_ASSERT_TRUE(sensor.reading().valid);
    TEST_ASSERT_EQUAL_INT32(23500, sensor.reading().milliCelsius);
    TEST_ASSERT_EQUAL_UINT32(bus.generation(), sensor.lastDependencyGeneration());
    TEST_ASSERT_TRUE(driver.selectedSeen);
    const Ds18b20TemperatureSensorConfigV1 expectedConfig = makeSensorConfig();
    TEST_ASSERT_EQUAL_MEMORY(expectedConfig.address.bytes, driver.selected.bytes, sizeof(driver.selected.bytes));
    TEST_ASSERT_TRUE(std::find(driver.writes.begin(), driver.writes.end(), kDs18b20CommandWriteScratchpad) != driver.writes.end());
    TEST_ASSERT_TRUE(std::find(driver.writes.begin(), driver.writes.end(), kDs18b20CommandConvertT) != driver.writes.end());
    TEST_ASSERT_TRUE(std::find(driver.writes.begin(), driver.writes.end(), kDs18b20CommandReadScratchpad) != driver.writes.end());
}

void test_ds18b20_runtime_serializes_fahrenheit_output_and_quiet_delta() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 12);
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);

    Ds18b20TemperatureSensorConfigV1 config = makeSensorConfig();
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    config.reportDeltaCentiCelsius = 50;
    Ds18b20TemperatureSensorDevice sensor(config);
    bindSensorDependency(sensor, 1002, bus.deviceId());
    sensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    driveSensorUntilReading(sensor);
    IDeviceRuntime* runtime = &sensor;
    TEST_ASSERT_TRUE(runtime->runtimeStateDirty());
    runtime->clearRuntimeStateDirty();

    driver.setTemperatureRaw(0x0179, 12);
    for (uint32_t now = 1300; now < 2400; now += 50U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_TRUE(sensor.reading().valid);
    TEST_ASSERT_FALSE(runtime->runtimeStateDirty());

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Ds18b20TemperatureSensorDeviceApiAdapter::instance().writeDeviceJson(sensor, sensor.status(), output);
    TEST_ASSERT_EQUAL_STRING("ds18b20_temperature_sensor", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("fahrenheit", output["runtime"]["temperature"]["unit"].as<const char*>());

    const std::string upsert = PortalWebSocketMessages::buildDeviceUpsert(
        sensor, sensor.status(), 99, false, &Ds18b20TemperatureSensorDeviceApiAdapter::instance(), "device_updated");
    DynamicJsonDocument wsDoc(4096);
    TEST_ASSERT_FALSE(deserializeJson(wsDoc, upsert));
    TEST_ASSERT_EQUAL_STRING("device.upsert", wsDoc["topic"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("device_updated", wsDoc["payload"]["eventKind"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("fahrenheit", wsDoc["payload"]["runtime"]["temperature"]["unit"].as<const char*>());
    TEST_ASSERT_TRUE(wsDoc["payload"]["runtime"]["temperature"]["valid"].as<bool>());
}

void test_ds18b20_runtime_report_always_marks_repeated_reading_dirty() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 12);
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);

    Ds18b20TemperatureSensorConfigV1 config = makeSensorConfig();
    config.reportAlways = 1;
    Ds18b20TemperatureSensorDevice sensor(config);
    bindSensorDependency(sensor, 1003, bus.deviceId());
    sensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    driveSensorUntilReading(sensor);
    IDeviceRuntime* runtime = &sensor;
    runtime->clearRuntimeStateDirty();

    const uint32_t previousMeasuredAt = sensor.reading().measuredAtMs;
    tickSensorUntilMeasuredAtChanges(sensor, previousMeasuredAt, previousMeasuredAt + config.pollMs);
    TEST_ASSERT_TRUE(sensor.reading().valid);
    TEST_ASSERT_NOT_EQUAL(previousMeasuredAt, sensor.reading().measuredAtMs);
    TEST_ASSERT_TRUE(runtime->runtimeStateDirty());
}

void test_ds18b20_runtime_publishes_invalid_crc_and_recovers() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 12);
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);

    Ds18b20TemperatureSensorDevice sensor(makeSensorConfig());
    bindSensorDependency(sensor, 1004, bus.deviceId());
    sensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    driveSensorUntilReading(sensor);
    TEST_ASSERT_TRUE(sensor.reading().valid);
    IDeviceRuntime* runtime = &sensor;
    runtime->clearRuntimeStateDirty();

    driver.corruptScratchpadCrc();
    for (uint32_t now = 1300; now <= 5800; now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_FALSE(sensor.reading().valid);
    TEST_ASSERT_EQUAL_STRING("crc_error", sensor.outputStatus());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(sensor.status()));

    StaticJsonDocument<1024> invalidDoc;
    JsonObject invalidOutput = invalidDoc.to<JsonObject>();
    Ds18b20TemperatureSensorDeviceApiAdapter::instance().writeDeviceJson(sensor, sensor.status(), invalidOutput);
    TEST_ASSERT_FALSE(invalidOutput["runtime"]["temperature"]["valid"].as<bool>());

    driver.setTemperatureRaw(0x0180, 12);
    for (uint32_t now = 5900; now < 29000 && !sensor.reading().valid; now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_FALSE(sensor.reading().valid);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(sensor.status()));

    for (uint32_t now = 30000; now < 43000 && !sensor.reading().valid; now += 100U) {
        sensor.tick100ms(now);
    }
    TEST_ASSERT_TRUE(sensor.reading().valid);
    TEST_ASSERT_EQUAL_STRING("ok", sensor.outputStatus());
    TEST_ASSERT_EQUAL_UINT8(0, sensor.consecutiveErrors());
}

void test_ds18b20_runtime_handles_missing_device_as_unavailable() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 12);
    driver.resetOk = false;
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);

    Ds18b20TemperatureSensorDevice sensor(makeSensorConfig());
    bindSensorDependency(sensor, 1005, bus.deviceId());
    sensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    sensor.begin(10);
    for (uint32_t now = 11; now < 600; now += 50U) {
        sensor.tick100ms(now);
    }

    TEST_ASSERT_FALSE(sensor.reading().valid);
    TEST_ASSERT_EQUAL_STRING("not_found", sensor.outputStatus());
    TEST_ASSERT_TRUE(sensor.consecutiveErrors() > 0U);
}

void test_ds18b20_runtime_reinitializes_on_dependency_generation_and_own_reconfigure() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 12);
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);

    Ds18b20TemperatureSensorDevice sensor(makeSensorConfig());
    bindSensorDependency(sensor, 1006, bus.deviceId());
    sensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    driveSensorUntilReading(sensor);
    const uint32_t firstGeneration = sensor.lastDependencyGeneration();
    TEST_ASSERT_TRUE(sensor.reading().valid);

    bus.requestReconfigure();
    bus.tick100ms(1200);
    bus.tick100ms(1201);
    TEST_ASSERT_TRUE(bus.generation() > firstGeneration);
    sensor.tick100ms(1202);
    sensor.tick100ms(1203);
    TEST_ASSERT_FALSE(sensor.reading().valid);
    driveSensorUntilReading(sensor, 1204);
    TEST_ASSERT_TRUE(sensor.reading().valid);
    TEST_ASSERT_EQUAL_UINT32(bus.generation(), sensor.lastDependencyGeneration());

    sensor.requestReconfigure();
    sensor.tick100ms(3400);
    sensor.tick100ms(3401);
    TEST_ASSERT_FALSE(sensor.reading().valid);
    driveSensorUntilReading(sensor, 3402);
    TEST_ASSERT_TRUE(sensor.reading().valid);
}

void test_ds18b20_sibling_sensor_failure_does_not_poison_other_runtime() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 12);
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);

    Ds18b20TemperatureSensorDevice failingSensor(makeSensorConfig(0xA1));
    Ds18b20TemperatureSensorDevice healthySensor(makeSensorConfig(0xA2));
    bindSensorDependency(failingSensor, 1007, bus.deviceId());
    bindSensorDependency(healthySensor, 1008, bus.deviceId());
    failingSensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    healthySensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);

    driver.resetOk = false;
    failingSensor.begin(10);
    for (uint32_t now = 11; now < 600; now += 50U) {
        failingSensor.tick100ms(now);
    }
    TEST_ASSERT_FALSE(failingSensor.reading().valid);

    driver.resetOk = true;
    driveSensorUntilReading(healthySensor, 700);
    TEST_ASSERT_TRUE(healthySensor.reading().valid);
    TEST_ASSERT_EQUAL_STRING("ok", healthySensor.outputStatus());
    TEST_ASSERT_FALSE(failingSensor.reading().valid);
}

void test_ds18b20_dependency_scan_defers_child_transaction() {
    FakeOneWireBusDriver driver;
    driver.setTemperatureRaw(0x0178, 12);
    driver.candidates = {makeRom()};
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);
    TEST_ASSERT_TRUE(bus.handleCommand(DeviceCommand{DeviceCommandType::Scan, 1, ""}));

    Ds18b20TemperatureSensorDevice sensor(makeSensorConfig());
    bindSensorDependency(sensor, 1009, bus.deviceId());
    sensor.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    sensor.begin(10);
    sensor.tick100ms(11);
    sensor.tick100ms(31);
    sensor.tick100ms(32);
    TEST_ASSERT_TRUE(bus.scan().inProgress);
    TEST_ASSERT_FALSE(bus.dependencyTransactionActive());
    TEST_ASSERT_FALSE(sensor.reading().valid);
}

void test_onewire_bus_detects_duplicate_dependent_rom_address() {
    FakeOneWireBusDriver driver;
    OneWireBusDevice bus(makeBusConfig(), driver);
    driveDependencyReady(bus);

    Ds18b20TemperatureSensorDevice first(makeSensorConfig(0x11));
    Ds18b20TemperatureSensorDevice second(makeSensorConfig(0x11));
    bindSensorDependency(first, 1010, bus.deviceId());
    bindSensorDependency(second, 1011, bus.deviceId());
    first.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    second.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &bus);
    bus.attachDependentRuntime(&first);
    bus.attachDependentRuntime(&second);

    TEST_ASSERT_TRUE(bus.hasDuplicateDependentRomAddress(first.config().address, &first));
    TEST_ASSERT_TRUE(bus.hasDuplicateDependentRomAddress(second.config().address, &second));
    TEST_ASSERT_FALSE(bus.hasDuplicateDependentRomAddress(makeSensorConfig(0x22).address));
}

void test_ds18b20_adapter_rejects_duplicate_address_on_same_dependency() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(1000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult busResult = registry.create(makeBusCreateRequest("onewire"), 10);
    TEST_ASSERT_TRUE_MESSAGE(busResult.ok(), busResult.validation.message);

    Ds18b20TemperatureSensorConfigV1 config = makeSensorConfig();
    DeviceCreateResult first = registry.create(makeSensorCreateRequest("temp-a", busResult.deviceId, config), 20);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);

    DeviceCreateRequest duplicate = makeSensorCreateRequest("temp-b", busResult.deviceId, config);
    const DeviceValidationResult validation =
        Ds18b20TemperatureSensorDeviceApiAdapter::instance().validateCreateRequest(duplicate, registry);
    TEST_ASSERT_FALSE(validation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(validation.error));
}

void test_ds18b20_adapter_rejects_duplicate_address_on_dependency_change() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(2000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult busA = registry.create(makeBusCreateRequest("onewire-a"), 10);
    DeviceCreateResult busB = registry.create(makeBusCreateRequest("onewire-b"), 20);
    TEST_ASSERT_TRUE_MESSAGE(busA.ok(), busA.validation.message);
    TEST_ASSERT_TRUE_MESSAGE(busB.ok(), busB.validation.message);

    Ds18b20TemperatureSensorConfigV1 addressA = makeSensorConfig(0xA1);
    Ds18b20TemperatureSensorConfigV1 addressB = makeSensorConfig(0xA2);
    DeviceCreateResult first = registry.create(makeSensorCreateRequest("temp-a", busA.deviceId, addressA), 30);
    DeviceCreateResult second = registry.create(makeSensorCreateRequest("temp-b", busB.deviceId, addressA), 40);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);
    TEST_ASSERT_TRUE_MESSAGE(second.ok(), second.validation.message);

    DeviceConfigUpdateRequest duplicateMove{};
    duplicateMove.configVersion = kDs18b20TemperatureSensorConfigVersion;
    duplicateMove.configBlob = encodeDs18b20Payload(addressA);
    duplicateMove.depsProvided = true;
    duplicateMove.depCount = 1;
    duplicateMove.deps[0] = DeviceDependencyLink{DeviceDependencyRole::OneWireBus, busA.deviceId};
    const IDeviceRuntime* secondRuntime = registry.runtime(second.deviceId);
    TEST_ASSERT_NOT_NULL(secondRuntime);
    const DeviceValidationResult duplicateValidation =
        Ds18b20TemperatureSensorDeviceApiAdapter::instance().validateUpdateConfigRequest(*secondRuntime, duplicateMove, registry);
    TEST_ASSERT_FALSE(duplicateValidation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(duplicateValidation.error));

    DeviceConfigUpdateRequest validMove{};
    validMove.configVersion = kDs18b20TemperatureSensorConfigVersion;
    validMove.configBlob = encodeDs18b20Payload(addressB);
    validMove.depsProvided = true;
    validMove.depCount = 1;
    validMove.deps[0] = DeviceDependencyLink{DeviceDependencyRole::OneWireBus, busA.deviceId};
    const DeviceValidationResult validValidation =
        Ds18b20TemperatureSensorDeviceApiAdapter::instance().validateUpdateConfigRequest(*secondRuntime, validMove, registry);
    TEST_ASSERT_TRUE_MESSAGE(validValidation.ok(), validValidation.message);

    const DeviceValidationResult dependencyValidation = Ds18b20TemperatureSensorDeviceApiAdapter::instance().validateSetDepsRequest(
        *secondRuntime,
        std::array<DeviceDependencyLink, kMaxDeviceDependencies>{{DeviceDependencyLink{DeviceDependencyRole::OneWireBus, busB.deviceId}}},
        1, registry);
    TEST_ASSERT_TRUE_MESSAGE(dependencyValidation.ok(), dependencyValidation.message);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_temperature_helpers_convert_units_and_dirty_threshold);
    RUN_TEST(test_ds18b20_config_codec_json_and_validation);
    RUN_TEST(test_ds18b20_protocol_helpers_parse_scratchpad);
    RUN_TEST(test_ds18b20_type_and_api_adapter_are_registered);
    RUN_TEST(test_ds18b20_api_adapter_parses_create_update_and_rejects_invalid_input);
    RUN_TEST(test_ds18b20_runtime_reads_addressed_temperature_and_configures_resolution);
    RUN_TEST(test_ds18b20_runtime_serializes_fahrenheit_output_and_quiet_delta);
    RUN_TEST(test_ds18b20_runtime_report_always_marks_repeated_reading_dirty);
    RUN_TEST(test_ds18b20_runtime_publishes_invalid_crc_and_recovers);
    RUN_TEST(test_ds18b20_runtime_handles_missing_device_as_unavailable);
    RUN_TEST(test_ds18b20_runtime_reinitializes_on_dependency_generation_and_own_reconfigure);
    RUN_TEST(test_ds18b20_sibling_sensor_failure_does_not_poison_other_runtime);
    RUN_TEST(test_ds18b20_dependency_scan_defers_child_transaction);
    RUN_TEST(test_onewire_bus_detects_duplicate_dependent_rom_address);
    RUN_TEST(test_ds18b20_adapter_rejects_duplicate_address_on_same_dependency);
    RUN_TEST(test_ds18b20_adapter_rejects_duplicate_address_on_dependency_change);
    return UNITY_END();
}
