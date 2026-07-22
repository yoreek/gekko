#include "JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/rtc/ds3231/Ds3231RtcDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/rtc_ds3231/Ds3231RtcDeviceApiAdapter.h"
#include "time/DateTime.h"

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

constexpr uint8_t kDs3231Address = 0x68;
constexpr uint8_t kRegStatus = 0x0F;
constexpr uint8_t kOscillatorStopFlagBit = 0x80;

uint8_t toBcd(uint8_t value) {
    return static_cast<uint8_t>(((value / 10U) << 4U) | (value % 10U));
}

uint8_t fromBcd(uint8_t value) {
    return static_cast<uint8_t>(((value >> 4U) * 10U) + (value & 0x0FU));
}

// Simulates DS3231 register semantics over the II2cBusDriver register-access primitives
// (beginTransmission/write/endTransmission/requestFrom/read), the same primitives
// Ds3231RtcDevice uses through an I2cBusDevice::DependencyTransaction. A single 1-byte write
// followed by endTransmission(false) is a "select register pointer" (repeated start); a
// multi-byte write is "write starting at this register".
class FakeDs3231I2cDriver final : public II2cBusDriver {
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
            const uint8_t reg = pendingWrite[0];
            for (size_t index = 1; index < pendingWrite.size(); ++index) {
                writeRegister(static_cast<uint8_t>(reg + index - 1U), pendingWrite[index]);
            }
            selectedRegister = reg;
        }
        return 0U;
    }

    size_t requestFrom(uint8_t, size_t size, bool) override {
        if (!present) {
            return 0U;
        }
        readBuffer.clear();
        for (size_t index = 0; index < size; ++index) {
            readBuffer.push_back(readRegister(static_cast<uint8_t>(selectedRegister + index)));
        }
        readPos = 0;
        return readBuffer.size();
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

    void setDateTime(const DateTime& dt) {
        timeRegisters[0] = toBcd(dt.second());
        timeRegisters[1] = toBcd(dt.minute());
        timeRegisters[2] = toBcd(dt.hour());
        timeRegisters[3] = toBcd(dt.weekday());
        timeRegisters[4] = toBcd(dt.day());
        timeRegisters[5] = toBcd(dt.month());
        timeRegisters[6] = toBcd(static_cast<uint8_t>(dt.year() - 2000U));
    }

    DateTime readDateTime() const {
        return DateTime(static_cast<uint16_t>(fromBcd(timeRegisters[6]) + 2000U), fromBcd(timeRegisters[5] & 0x7FU),
                        fromBcd(timeRegisters[4]), fromBcd(timeRegisters[2]), fromBcd(timeRegisters[1]), fromBcd(timeRegisters[0] & 0x7FU));
    }

    bool present{true};
    uint8_t timeRegisters[7]{};
    uint8_t statusRegister{0};
    uint8_t lastAddress{0};

private:
    uint8_t readRegister(uint8_t reg) const {
        if (reg == kRegStatus) {
            return statusRegister;
        }
        if (reg < 7U) {
            return timeRegisters[reg];
        }
        return 0xFFU;
    }

    void writeRegister(uint8_t reg, uint8_t value) {
        if (reg == kRegStatus) {
            statusRegister = value;
            return;
        }
        if (reg < 7U) {
            timeRegisters[reg] = value;
        }
    }

    uint8_t selectedRegister{0};
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

Ds3231RtcDeviceConfigV2 makeRtcConfig(bool useForSystemTimeSync = false, uint8_t i2cAddress = 0x68U) {
    Ds3231RtcDeviceConfigV2 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "rtc");
    config.useForSystemTimeSync = useForSystemTimeSync ? 1U : 0U;
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

template <typename Config> BoundedBlob<kMaxDeviceConfigBytes> encodeRtcPayload(const Config& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Config::kMagic, config, buffer, ds3231RtcDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, ds3231RtcDeviceConfigSize(config)));
    return payload;
}

void driveBusReady(I2cBusDevice& bus, uint32_t startNow = 1U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
}

void bindRtcDependency(Ds3231RtcDevice& rtc, DeviceId rtcId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = rtcId;
    record.header.typeId = kDs3231RtcTypeId;
    record.header.configVersion = kDs3231RtcConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::I2CBus, busId};
    record.status = DeviceStatus::Ready;
    rtc.bindDeviceIdentity(record, encodeRtcPayload(rtc.config()));
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

void driveRtcUntilReading(Ds3231RtcDevice& rtc, uint32_t startNow = 10U) {
    rtc.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && !rtc.lastReadOk(); now += 100U) {
        rtc.tick100ms(now);
    }
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

DeviceCreateRequest makeRtcCreateRequest(const char* name, DeviceId busId, const Ds3231RtcDeviceConfigV2& config) {
    DeviceCreateRequest request{};
    request.typeId = kDs3231RtcTypeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::I2CBus, busId};
    request.configVersion = kDs3231RtcConfigVersion;
    request.configBlob = encodeRtcPayload(config);
    return request;
}

} // namespace

void test_ds3231_config_codec_json_and_validation() {
    const Ds3231RtcDeviceConfigV2 config = makeRtcConfig(true, 0x57U);
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeRtcPayload(config);

    Ds3231RtcDeviceConfigV2 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Ds3231RtcDeviceConfigV2::kMagic, payload.data(), payload.size(), decoded));
    TEST_ASSERT_TRUE(decoded.useForSystemTimeSync != 0U);
    TEST_ASSERT_EQUAL_UINT8(0x57U, decoded.i2cAddress);
    TEST_ASSERT_EQUAL_STRING("rtc", decoded.name);

    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    writeDs3231RtcDeviceConfigJson(config, json);
    assertMatchesJsonSchema("schemas/rest/v1/devices/rtc_ds3231.config.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_TRUE(json["useForSystemTimeSync"].as<bool>());
    TEST_ASSERT_EQUAL_UINT8(0x57U, json["i2cAddress"].as<uint8_t>());

    Ds3231RtcDeviceConfigV2 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parseDs3231RtcDeviceConfigJson(json, parsed, error));
    TEST_ASSERT_TRUE(parsed.useForSystemTimeSync != 0U);
    TEST_ASSERT_EQUAL_UINT8(0x57U, parsed.i2cAddress);
    TEST_ASSERT_TRUE(parsed.validate().ok());

    Ds3231RtcDeviceConfigV2 defaults{};
    TEST_ASSERT_EQUAL_UINT8(0x68U, defaults.i2cAddress);

    Ds3231RtcDeviceConfigV2 outOfRange = makeRtcConfig(false, 0x80U);
    TEST_ASSERT_FALSE(outOfRange.validate().ok());
}

void test_ds3231_config_migrates_v1_to_v2() {
    // A legacy "DS3231-1" blob must decode and migrate to V2, preserving its fields.
    EWFM_LEGACY_CONFIG_USE_BEGIN
    Ds3231RtcDeviceConfigV1 legacy{};
    legacy.enabled = 1U;
    std::snprintf(legacy.name, sizeof(legacy.name), "%s", "rtc-legacy");
    legacy.useForSystemTimeSync = 1U;
    legacy.i2cAddress = 0x57U;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ds3231RtcDeviceConfigSize(legacy);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ds3231RtcDeviceConfigV1::kMagic, legacy, buffer, size));
    EWFM_LEGACY_CONFIG_USE_END

    Ds3231RtcDeviceConfigV2 migrated{};
    TEST_ASSERT_TRUE(decodeDs3231RtcDeviceConfig(buffer, size, migrated));
    TEST_ASSERT_EQUAL_STRING("rtc-legacy", migrated.name);
    TEST_ASSERT_TRUE(migrated.enabled != 0U);
    TEST_ASSERT_TRUE(migrated.useForSystemTimeSync != 0U);
    TEST_ASSERT_EQUAL_UINT8(0x57U, migrated.i2cAddress);
}

void test_ds3231_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kDs3231RtcTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::RealTimeClock));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(kDs3231RtcTypeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("rtc_ds3231"));
}

void test_ds3231_runtime_reads_time_from_registers() {
    FakeDs3231I2cDriver driver;
    const DateTime known(2024, 3, 5, 12, 45, 30);
    driver.setDateTime(known);

    I2cBusDevice bus(makeBusConfig(), driver);
    bindBusIdentity(bus, 7001U);
    driveBusReady(bus);

    Ds3231RtcDevice rtc(makeRtcConfig());
    bindRtcDependency(rtc, 2001, bus.deviceId());
    rtc.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveRtcUntilReading(rtc);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(rtc.status()));
    TEST_ASSERT_TRUE(rtc.lastReadOk());

    uint32_t epoch = 0;
    bool lostPower = false;
    TEST_ASSERT_TRUE(rtc.latestTimeReading(epoch, lostPower));
    TEST_ASSERT_EQUAL_UINT32(known.utcUnixtime(), epoch);
    TEST_ASSERT_FALSE(lostPower);
}

void test_ds3231_runtime_uses_configured_i2c_address() {
    FakeDs3231I2cDriver driver;
    driver.setDateTime(DateTime(2024, 3, 5, 12, 45, 30));

    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    // Non-default address - some breakout boards/clones don't answer at the conventional 0x68.
    Ds3231RtcDevice rtc(makeRtcConfig(false, 0x57U));
    bindRtcDependency(rtc, 2006, bus.deviceId());
    rtc.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveRtcUntilReading(rtc);

    TEST_ASSERT_TRUE(rtc.lastReadOk());
    TEST_ASSERT_EQUAL_UINT8(0x57U, driver.lastAddress);

    uint8_t reportedAddress = 0;
    TEST_ASSERT_TRUE(rtc.i2cAddress(reportedAddress));
    TEST_ASSERT_EQUAL_UINT8(0x57U, reportedAddress);
}

void test_ds3231_runtime_reconfigures_on_address_change() {
    FakeDs3231I2cDriver driver;
    driver.setDateTime(DateTime(2024, 3, 5, 12, 45, 30));

    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Ds3231RtcDevice rtc(makeRtcConfig());
    bindRtcDependency(rtc, 2007, bus.deviceId());
    rtc.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveRtcUntilReading(rtc);

    const Ds3231RtcDeviceConfigV2 sameAddress = makeRtcConfig();
    const DeviceConfigUpdatePlan samePlan = rtc.planConfigUpdate(encodeRtcPayload(sameAddress));
    TEST_ASSERT_FALSE(samePlan.resetStateMachine);

    const Ds3231RtcDeviceConfigV2 newAddress = makeRtcConfig(false, 0x57U);
    const DeviceConfigUpdatePlan changedPlan = rtc.planConfigUpdate(encodeRtcPayload(newAddress));
    TEST_ASSERT_TRUE(changedPlan.resetStateMachine);
    TEST_ASSERT_TRUE(changedPlan.endOldConfig);
}

void test_ds3231_runtime_reports_lost_power_flag() {
    FakeDs3231I2cDriver driver;
    driver.setDateTime(DateTime(2024, 1, 1, 0, 0, 0));
    driver.statusRegister = kOscillatorStopFlagBit;

    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Ds3231RtcDevice rtc(makeRtcConfig());
    bindRtcDependency(rtc, 2002, bus.deviceId());
    rtc.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveRtcUntilReading(rtc);

    uint32_t epoch = 0;
    bool lostPower = false;
    TEST_ASSERT_TRUE(rtc.latestTimeReading(epoch, lostPower));
    TEST_ASSERT_TRUE(lostPower);
}

void test_ds3231_runtime_write_time_updates_registers_and_clears_oscillator_stop_flag() {
    FakeDs3231I2cDriver driver;
    driver.setDateTime(DateTime(2020, 1, 1, 0, 0, 0));
    driver.statusRegister = kOscillatorStopFlagBit;

    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Ds3231RtcDevice rtc(makeRtcConfig());
    bindRtcDependency(rtc, 2003, bus.deviceId());
    rtc.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveRtcUntilReading(rtc);

    const DateTime target(2025, 6, 15, 8, 30, 0);
    TEST_ASSERT_TRUE(rtc.writeTime(target.utcUnixtime()));

    const DateTime written = driver.readDateTime();
    TEST_ASSERT_EQUAL_UINT32(target.utcUnixtime(), written.utcUnixtime());
    TEST_ASSERT_EQUAL_UINT8(0U, driver.statusRegister & kOscillatorStopFlagBit);
}

void test_ds3231_runtime_retries_when_chip_absent_then_recovers() {
    FakeDs3231I2cDriver driver;
    driver.setDateTime(DateTime(2024, 3, 5, 12, 0, 0));
    driver.present = false;

    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Ds3231RtcDevice rtc(makeRtcConfig());
    bindRtcDependency(rtc, 2004, bus.deviceId());
    rtc.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    rtc.begin(10U);

    // Three consecutive probe failures (~1s retry backoff each) push the device into Faulted,
    // which then waits out a much longer (30s) backoff before trying again.
    uint32_t now = 10U;
    const uint32_t faultDeadline = now + 40000U;
    for (; now < faultDeadline && rtc.status() != DeviceStatus::Faulted; now += 100U) {
        rtc.tick100ms(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(rtc.status()));
    TEST_ASSERT_FALSE(rtc.lastReadOk());

    driver.present = true;
    const uint32_t recoveryDeadline = now + 40000U;
    for (; now < recoveryDeadline && !rtc.lastReadOk(); now += 100U) {
        rtc.tick100ms(now);
    }
    TEST_ASSERT_TRUE(rtc.lastReadOk());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(rtc.status()));
}

void test_ds3231_api_adapter_writes_runtime_json() {
    FakeDs3231I2cDriver driver;
    driver.setDateTime(DateTime(2024, 3, 5, 12, 45, 30));

    I2cBusDevice bus(makeBusConfig(), driver);
    bindBusIdentity(bus, 7002U);
    driveBusReady(bus);

    Ds3231RtcDevice rtc(makeRtcConfig(true));
    bindRtcDependency(rtc, 2005, bus.deviceId());
    rtc.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveRtcUntilReading(rtc);

    StaticJsonDocument<1024> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    Ds3231RtcDeviceApiAdapter::instance().writeDeviceJson(rtc, rtc.status(), output);
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-rtc_ds3231.response.schema.json", outputDoc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_STRING("rtc_ds3231", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_TRUE(output["config"]["useForSystemTimeSync"].as<bool>());
    TEST_ASSERT_EQUAL_UINT8(0x68U, output["config"]["i2cAddress"].as<uint8_t>());
    TEST_ASSERT_TRUE(output["runtime"]["lastReadOk"].as<bool>());
    TEST_ASSERT_FALSE(output["runtime"]["oscillatorStopped"].as<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["currentEpochUtc"].as<uint32_t>() > 0U);
}

void test_ds3231_adapter_rejects_second_active_sync_device() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(3000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    // Both configs default to the same 0x68 address, so two devices on the *same* bus would
    // already collide on address alone - put the second one on its own bus to isolate the
    // at-most-one-active-sync rule from the (also-enforced, separately-tested-below) duplicate
    // address rule.
    DeviceCreateResult busAResult = registry.create(makeBusCreateRequest("i2c-a"), 10);
    TEST_ASSERT_TRUE_MESSAGE(busAResult.ok(), busAResult.validation.message);
    DeviceCreateResult busBResult = registry.create(makeBusCreateRequest("i2c-b"), 11);
    TEST_ASSERT_TRUE_MESSAGE(busBResult.ok(), busBResult.validation.message);

    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "rtc_ds3231";
    JsonObject createConfig = createDoc.createNestedObject("config");
    createConfig["name"] = "rtc-a";
    createConfig["enabled"] = true;
    JsonArray deps = createConfig.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "i2c_bus";
    dep["deviceId"] = busAResult.deviceId;
    createConfig["i2cAddress"] = 0x68U;
    createConfig["useForSystemTimeSync"] = true;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-rtc_ds3231.request.schema.json", createDoc.as<JsonVariantConst>());

    DeviceCreateRequest parsedCreate{};
    const char* createError = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Ds3231RtcDeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), parsedCreate, createError), createError);

    DeviceCreateResult first = registry.create(parsedCreate, 20);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);

    const Ds3231RtcDeviceConfigV2 activeConfig = makeRtcConfig(true);
    const DeviceCreateRequest second = makeRtcCreateRequest("rtc-b", busBResult.deviceId, activeConfig);
    const DeviceValidationResult validation = Ds3231RtcDeviceApiAdapter::instance().validateCreateRequest(second, registry);
    TEST_ASSERT_FALSE(validation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(validation.error));

    // A second device that does NOT request the sync flag is unaffected.
    const Ds3231RtcDeviceConfigV2 inactiveConfig = makeRtcConfig(false);
    const DeviceCreateRequest third = makeRtcCreateRequest("rtc-c", busBResult.deviceId, inactiveConfig);
    const DeviceValidationResult thirdValidation = Ds3231RtcDeviceApiAdapter::instance().validateCreateRequest(third, registry);
    TEST_ASSERT_TRUE_MESSAGE(thirdValidation.ok(), thirdValidation.message);
}

void test_ds3231_adapter_rejects_duplicate_address_on_same_bus_but_allows_distinct_addresses() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(4000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult busResult = registry.create(makeBusCreateRequest("i2c"), 10);
    TEST_ASSERT_TRUE_MESSAGE(busResult.ok(), busResult.validation.message);

    StaticJsonDocument<256> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["i2cAddress"] = 0x57U;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-rtc_ds3231.request.schema.json", updateDoc.as<JsonVariantConst>());

    const DeviceCreateResult first = registry.create(makeRtcCreateRequest("rtc-a", busResult.deviceId, makeRtcConfig(false, 0x68U)), 20);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);

    // Same bus, same (default) address as the device just created above - rejected.
    const DeviceCreateRequest duplicateAddress = makeRtcCreateRequest("rtc-b", busResult.deviceId, makeRtcConfig(false, 0x68U));
    const DeviceValidationResult duplicateValidation =
        Ds3231RtcDeviceApiAdapter::instance().validateCreateRequest(duplicateAddress, registry);
    TEST_ASSERT_FALSE(duplicateValidation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(duplicateValidation.error));

    // Same bus, distinct address - two DS3231 modules (or a DS3231 plus another I2C device) can
    // legitimately share one bus as long as their addresses differ.
    const DeviceCreateRequest distinctAddress = makeRtcCreateRequest("rtc-c", busResult.deviceId, makeRtcConfig(false, 0x57U));
    const DeviceValidationResult distinctValidation =
        Ds3231RtcDeviceApiAdapter::instance().validateCreateRequest(distinctAddress, registry);
    TEST_ASSERT_TRUE_MESSAGE(distinctValidation.ok(), distinctValidation.message);
}
