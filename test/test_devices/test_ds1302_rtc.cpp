#include "JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/rtc/ds1302/Ds1302RtcDevice.h"
#include "devices/rtc/ds3231/Ds3231RtcDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/rtc_ds1302/Ds1302RtcDeviceApiAdapter.h"
#include "integrations/rest/rtc_ds3231/Ds3231RtcDeviceApiAdapter.h"
#include "time/DateTime.h"

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

constexpr uint8_t kRegSeconds = 0x80;
constexpr uint8_t kRegMinutes = 0x82;
constexpr uint8_t kRegHour = 0x84;
constexpr uint8_t kRegDate = 0x86;
constexpr uint8_t kRegMonth = 0x88;
constexpr uint8_t kRegDay = 0x8A;
constexpr uint8_t kRegYear = 0x8C;
constexpr uint8_t kClockHaltBit = 0x80U;

uint8_t toBcd(uint8_t value) {
    return static_cast<uint8_t>(((value / 10U) << 4U) | (value % 10U));
}

uint8_t fromBcd(uint8_t value) {
    return static_cast<uint8_t>(((value >> 4U) * 10U) + (value & 0x0FU));
}

// Reconstructs DS1302's byte-level register protocol (command byte, then a written or read data
// byte, framed by one RST high/low cycle each) from the bit-level IDs1302LineDriver calls
// Ds1302Protocol issues - the same relationship FakeDs3231I2cDriver has to II2cBusDriver's
// register-access primitives, one level lower since DS1302 has no byte-oriented bus API at all.
class FakeDs1302LineDriver final : public IDs1302LineDriver {
public:
    bool setClk(uint8_t, bool high) override {
        if (!present) {
            return false;
        }
        if (high && !clkHigh_) {
            onClockRisingEdge();
        }
        clkHigh_ = high;
        return true;
    }

    bool setRst(uint8_t, bool high) override {
        if (!present) {
            return false;
        }
        if (high && !rstHigh_) {
            beginTransaction();
        } else if (!high && rstHigh_) {
            endTransaction();
        }
        rstHigh_ = high;
        return true;
    }

    bool setDataOutput(uint8_t) override {
        return present;
    }

    bool setDataInput(uint8_t) override {
        return present;
    }

    bool writeData(uint8_t, bool high) override {
        if (!present) {
            return false;
        }
        pendingWriteBit_ = high;
        return true;
    }

    bool readData(uint8_t, bool& level) override {
        if (!present) {
            return false;
        }
        level = currentReadBit();
        return true;
    }

    void waitMicros(uint32_t) override {}

    void setRegister(uint8_t regKey, uint8_t value) {
        registers_[indexOf(regKey)] = value;
    }

    uint8_t getRegister(uint8_t regKey) const {
        return registers_[indexOf(regKey)];
    }

    bool present{true};

private:
    enum class Phase { Idle, Command, WriteData, ReadData };

    static uint8_t indexOf(uint8_t regKey) {
        return static_cast<uint8_t>((regKey - 0x80U) / 2U);
    }

    void beginTransaction() {
        phase_ = Phase::Command;
        bitIndex_ = 0U;
        commandByte_ = 0U;
        dataByte_ = 0U;
    }

    void endTransaction() {
        if (phase_ == Phase::WriteData && bitIndex_ == 8U) {
            registers_[indexOf(static_cast<uint8_t>(commandByte_ & 0xFEU))] = dataByte_;
        }
        phase_ = Phase::Idle;
    }

    bool currentReadBit() const {
        if (phase_ != Phase::ReadData) {
            return false;
        }
        return ((readByteValue_ >> bitIndex_) & 0x01U) != 0U;
    }

    void onClockRisingEdge() {
        switch (phase_) {
        case Phase::Command:
            commandByte_ = static_cast<uint8_t>(commandByte_ | (pendingWriteBit_ ? (1U << bitIndex_) : 0U));
            ++bitIndex_;
            if (bitIndex_ == 8U) {
                const bool isRead = (commandByte_ & 0x01U) != 0U;
                bitIndex_ = 0U;
                if (isRead) {
                    readByteValue_ = registers_[indexOf(static_cast<uint8_t>(commandByte_ & 0xFEU))];
                    phase_ = Phase::ReadData;
                } else {
                    phase_ = Phase::WriteData;
                }
            }
            break;
        case Phase::WriteData:
            dataByte_ = static_cast<uint8_t>(dataByte_ | (pendingWriteBit_ ? (1U << bitIndex_) : 0U));
            ++bitIndex_;
            break;
        case Phase::ReadData:
            ++bitIndex_;
            break;
        case Phase::Idle:
        default:
            break;
        }
    }

    Phase phase_{Phase::Idle};
    bool clkHigh_{false};
    bool rstHigh_{false};
    bool pendingWriteBit_{false};
    uint8_t bitIndex_{0U};
    uint8_t commandByte_{0U};
    uint8_t dataByte_{0U};
    uint8_t readByteValue_{0U};
    uint8_t registers_[8]{};
};

void setDriverDateTime(FakeDs1302LineDriver& driver, const DateTime& dt) {
    driver.setRegister(kRegSeconds, toBcd(dt.second()));
    driver.setRegister(kRegMinutes, toBcd(dt.minute()));
    driver.setRegister(kRegHour, toBcd(dt.hour()));
    driver.setRegister(kRegDate, toBcd(dt.day()));
    driver.setRegister(kRegMonth, toBcd(dt.month()));
    driver.setRegister(kRegDay, toBcd(dt.weekday()));
    driver.setRegister(kRegYear, toBcd(static_cast<uint8_t>(dt.year() - 2000U)));
}

DateTime readDriverDateTime(const FakeDs1302LineDriver& driver) {
    return DateTime(static_cast<uint16_t>(fromBcd(driver.getRegister(kRegYear)) + 2000U), fromBcd(driver.getRegister(kRegMonth) & 0x1FU),
                    fromBcd(driver.getRegister(kRegDate) & 0x3FU), fromBcd(driver.getRegister(kRegHour) & 0x3FU),
                    fromBcd(driver.getRegister(kRegMinutes) & 0x7FU), fromBcd(driver.getRegister(kRegSeconds) & 0x7FU));
}

Ds1302RtcDeviceConfigV1 makeRtcConfig(bool useForSystemTimeSync = false) {
    Ds1302RtcDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "rtc");
    config.useForSystemTimeSync = useForSystemTimeSync ? 1U : 0U;
    config.clkPin = 25U;
    config.dataPin = 26U;
    config.rstPin = 27U;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeRtcPayload(const Ds1302RtcDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ds1302RtcDeviceConfigV1::kMagic, config, buffer, ds1302RtcDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, ds1302RtcDeviceConfigSize(config)));
    return payload;
}

void bindIdentity(Ds1302RtcDevice& rtc, DeviceId deviceId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = kDs1302RtcTypeId;
    record.header.configVersion = kDs1302RtcConfigVersion;
    record.header.configRevision = 1U;
    record.status = DeviceStatus::Ready;
    rtc.bindDeviceIdentity(record, encodeRtcPayload(rtc.config()));
}

void driveRtcUntilReading(Ds1302RtcDevice& rtc, uint32_t startNow = 10U) {
    rtc.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && !rtc.lastReadOk(); now += 100U) {
        rtc.tick100ms(now);
    }
}

DeviceCreateRequest makeRtcCreateRequest(const char* name, const Ds1302RtcDeviceConfigV1& config) {
    DeviceCreateRequest request{};
    request.typeId = kDs1302RtcTypeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.configVersion = kDs1302RtcConfigVersion;
    request.configBlob = encodeRtcPayload(config);
    return request;
}

} // namespace

void test_ds1302_config_codec_json_and_validation() {
    const Ds1302RtcDeviceConfigV1 config = makeRtcConfig(true);
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeRtcPayload(config);

    Ds1302RtcDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Ds1302RtcDeviceConfigV1::kMagic, payload.data(), payload.size(), decoded));
    TEST_ASSERT_TRUE(decoded.useForSystemTimeSync != 0U);
    TEST_ASSERT_EQUAL_UINT8(25U, decoded.clkPin);
    TEST_ASSERT_EQUAL_UINT8(26U, decoded.dataPin);
    TEST_ASSERT_EQUAL_UINT8(27U, decoded.rstPin);
    TEST_ASSERT_EQUAL_STRING("rtc", decoded.name);

    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    writeDs1302RtcDeviceConfigJson(config, json);
    assertMatchesJsonSchema("schemas/rest/v1/devices/rtc_ds1302.config.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_TRUE(json["useForSystemTimeSync"].as<bool>());
    TEST_ASSERT_EQUAL_UINT8(25U, json["clkPin"].as<uint8_t>());

    Ds1302RtcDeviceConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parseDs1302RtcDeviceConfigJson(json, parsed, error));
    TEST_ASSERT_TRUE(parsed.useForSystemTimeSync != 0U);
    TEST_ASSERT_EQUAL_UINT8(26U, parsed.dataPin);
    TEST_ASSERT_TRUE(parsed.validate().ok());
}

void test_ds1302_config_rejects_duplicate_pins() {
    Ds1302RtcDeviceConfigV1 config = makeRtcConfig();
    config.dataPin = config.clkPin;
    TEST_ASSERT_FALSE(config.validate().ok());
}

void test_ds1302_config_rejects_invalid_use_for_system_time_sync() {
    Ds1302RtcDeviceConfigV1 config = makeRtcConfig();
    config.useForSystemTimeSync = 2U;
    TEST_ASSERT_FALSE(config.validate().ok());
}

void test_ds1302_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = typeRegistry.find(kDs1302RtcTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::RealTimeClock));
    TEST_ASSERT_EQUAL_UINT8(0U, descriptor->maxDependents);

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(kDs1302RtcTypeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("rtc_ds1302"));
}

void test_ds1302_runtime_reads_time_from_registers() {
    FakeDs1302LineDriver driver;
    const DateTime known(2024, 3, 5, 12, 45, 30);
    setDriverDateTime(driver, known);

    Ds1302RtcDevice rtc(makeRtcConfig(), driver);
    bindIdentity(rtc, 2001U);
    driveRtcUntilReading(rtc);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(rtc.status()));
    TEST_ASSERT_TRUE(rtc.lastReadOk());

    uint32_t epoch = 0;
    bool lostPower = false;
    TEST_ASSERT_TRUE(rtc.latestTimeReading(epoch, lostPower));
    TEST_ASSERT_EQUAL_UINT32(known.utcUnixtime(), epoch);
    TEST_ASSERT_FALSE(lostPower);
}

void test_ds1302_runtime_reconfigures_on_pin_change() {
    FakeDs1302LineDriver driver;
    setDriverDateTime(driver, DateTime(2024, 3, 5, 12, 45, 30));

    Ds1302RtcDevice rtc(makeRtcConfig(), driver);
    bindIdentity(rtc, 2002U);
    driveRtcUntilReading(rtc);

    const Ds1302RtcDeviceConfigV1 samePins = makeRtcConfig();
    const DeviceConfigUpdatePlan samePlan = rtc.planConfigUpdate(encodeRtcPayload(samePins));
    TEST_ASSERT_FALSE(samePlan.resetStateMachine);

    Ds1302RtcDeviceConfigV1 changedPins = makeRtcConfig();
    changedPins.clkPin = 32U;
    const DeviceConfigUpdatePlan changedPlan = rtc.planConfigUpdate(encodeRtcPayload(changedPins));
    TEST_ASSERT_TRUE(changedPlan.resetStateMachine);
    TEST_ASSERT_TRUE(changedPlan.endOldConfig);
}

void test_ds1302_runtime_lost_power_flag_always_false() {
    // Unlike DS3231's oscillator-stop flag, DS1302's clock-halt (CH) bit only reflects whether we
    // last told it to halt, not real backup-battery loss - so it is deliberately never surfaced as
    // "lost power", even when the register shows CH set.
    FakeDs1302LineDriver driver;
    DateTime dt(2024, 1, 1, 0, 0, 0);
    setDriverDateTime(driver, dt);
    driver.setRegister(kRegSeconds, static_cast<uint8_t>(toBcd(dt.second()) | kClockHaltBit));

    Ds1302RtcDevice rtc(makeRtcConfig(), driver);
    bindIdentity(rtc, 2003U);
    driveRtcUntilReading(rtc);

    uint32_t epoch = 0;
    bool lostPower = true;
    TEST_ASSERT_TRUE(rtc.latestTimeReading(epoch, lostPower));
    TEST_ASSERT_FALSE(lostPower);
}

void test_ds1302_runtime_write_time_updates_registers_and_starts_clock() {
    FakeDs1302LineDriver driver;
    setDriverDateTime(driver, DateTime(2020, 1, 1, 0, 0, 0));
    driver.setRegister(kRegSeconds, static_cast<uint8_t>(toBcd(0U) | kClockHaltBit));

    Ds1302RtcDevice rtc(makeRtcConfig(), driver);
    bindIdentity(rtc, 2004U);
    driveRtcUntilReading(rtc);

    const DateTime target(2025, 6, 15, 8, 30, 0);
    TEST_ASSERT_TRUE(rtc.writeTime(target.utcUnixtime()));

    const DateTime written = readDriverDateTime(driver);
    TEST_ASSERT_EQUAL_UINT32(target.utcUnixtime(), written.utcUnixtime());
    TEST_ASSERT_EQUAL_UINT8(0U, driver.getRegister(kRegSeconds) & kClockHaltBit);
}

void test_ds1302_runtime_retries_when_chip_absent_then_recovers() {
    FakeDs1302LineDriver driver;
    setDriverDateTime(driver, DateTime(2024, 3, 5, 12, 0, 0));
    driver.present = false;

    Ds1302RtcDevice rtc(makeRtcConfig(), driver);
    bindIdentity(rtc, 2005U);
    rtc.begin(10U);

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

void test_ds1302_api_adapter_writes_runtime_json() {
    FakeDs1302LineDriver driver;
    setDriverDateTime(driver, DateTime(2024, 3, 5, 12, 45, 30));

    Ds1302RtcDevice rtc(makeRtcConfig(true), driver);
    bindIdentity(rtc, 2006U);
    driveRtcUntilReading(rtc);

    StaticJsonDocument<1024> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    Ds1302RtcDeviceApiAdapter::instance().writeDeviceJson(rtc, rtc.status(), output);
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-rtc_ds1302.response.schema.json", outputDoc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_STRING("rtc_ds1302", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_TRUE(output["config"]["useForSystemTimeSync"].as<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["lastReadOk"].as<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["currentEpochUtc"].as<uint32_t>() > 0U);
}

void test_ds1302_adapter_rejects_deps() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "rtc_ds1302";
    JsonObject createConfig = createDoc.createNestedObject("config");
    createConfig["name"] = "rtc-a";
    createConfig["enabled"] = true;
    JsonArray deps = createConfig.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "i2c_bus";
    dep["deviceId"] = 1U;

    DeviceCreateRequest parsedCreate{};
    const char* createError = nullptr;
    TEST_ASSERT_FALSE(Ds1302RtcDeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), parsedCreate, createError));
    TEST_ASSERT_NOT_NULL(createError);
}

void test_ds1302_adapter_rejects_second_active_sync_device() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(3000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    const DeviceCreateResult first = registry.create(makeRtcCreateRequest("rtc-a", makeRtcConfig(true)), 20);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);

    // Same type, second active-sync request - rejected.
    const DeviceCreateRequest second = makeRtcCreateRequest("rtc-b", makeRtcConfig(true));
    const DeviceValidationResult validation = Ds1302RtcDeviceApiAdapter::instance().validateCreateRequest(second, registry);
    TEST_ASSERT_FALSE(validation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(validation.error));

    // A second device that does NOT request the sync flag is unaffected.
    const DeviceCreateRequest third = makeRtcCreateRequest("rtc-c", makeRtcConfig(false));
    const DeviceValidationResult thirdValidation = Ds1302RtcDeviceApiAdapter::instance().validateCreateRequest(third, registry);
    TEST_ASSERT_TRUE_MESSAGE(thirdValidation.ok(), thirdValidation.message);
}

void test_ds1302_adapter_rejects_active_sync_when_ds3231_already_active() {
    // The "at most one active sync RTC" rule now lives in the shared RtcDeviceApiSupport helper
    // (validateAtMostOneActiveRtcSync), used by both Ds3231RtcDeviceApiAdapter and
    // Ds1302RtcDeviceApiAdapter - it must arbitrate across RTC types, not just within one.
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(3100);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    I2cBusDeviceConfigV1 busConfig{};
    busConfig.enabled = 1U;
    std::snprintf(busConfig.name, sizeof(busConfig.name), "%s", "i2c-bus");
    busConfig.sdaPin = 18;
    busConfig.sclPin = 19;
    busConfig.internalPullup = 1U;
    busConfig.frequencyHz = 400000U;
    DeviceCreateRequest busRequest{};
    busRequest.typeId = I2cBusDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(busRequest.assignName("i2c"));
    busRequest.setEnabled(true);
    busRequest.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    uint8_t busBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, busConfig, busBuffer, i2cBusDeviceConfigSize(busConfig)));
    TEST_ASSERT_TRUE(busRequest.configBlob.assign(busBuffer, i2cBusDeviceConfigSize(busConfig)));
    const DeviceCreateResult busResult = registry.create(busRequest, 10);
    TEST_ASSERT_TRUE_MESSAGE(busResult.ok(), busResult.validation.message);

    Ds3231RtcDeviceConfigV2 ds3231Config{};
    ds3231Config.enabled = 1U;
    std::snprintf(ds3231Config.name, sizeof(ds3231Config.name), "%s", "ds3231-active");
    ds3231Config.useForSystemTimeSync = 1U;
    DeviceCreateRequest ds3231Request{};
    ds3231Request.typeId = kDs3231RtcTypeId;
    TEST_ASSERT_TRUE(ds3231Request.assignName("ds3231-active"));
    ds3231Request.setEnabled(true);
    ds3231Request.depCount = 1;
    ds3231Request.deps[0] = {DeviceRole::I2CBus, busResult.deviceId};
    ds3231Request.configVersion = kDs3231RtcConfigVersion;
    uint8_t ds3231Buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(Ds3231RtcDeviceConfigV2::kMagic, ds3231Config, ds3231Buffer, ds3231RtcDeviceConfigSize(ds3231Config)));
    TEST_ASSERT_TRUE(ds3231Request.configBlob.assign(ds3231Buffer, ds3231RtcDeviceConfigSize(ds3231Config)));
    const DeviceCreateResult ds3231Result = registry.create(ds3231Request, 20);
    TEST_ASSERT_TRUE_MESSAGE(ds3231Result.ok(), ds3231Result.validation.message);

    // A DS1302 requesting active sync must be rejected by the shared cross-type rule.
    const DeviceCreateRequest ds1302Request = makeRtcCreateRequest("ds1302-a", makeRtcConfig(true));
    const DeviceValidationResult validation = Ds1302RtcDeviceApiAdapter::instance().validateCreateRequest(ds1302Request, registry);
    TEST_ASSERT_FALSE(validation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(validation.error));
}
