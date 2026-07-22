#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/expander/Pcf8574ExpanderDevice.h"
#include "devices/expander/Pcf8575ExpanderDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/expander/Pcf8574ExpanderDeviceApiAdapter.h"
#include "integrations/rest/expander/Pcf8575ExpanderDeviceApiAdapter.h"
#include "integrations/rest/expander/PortExpanderSwitchDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

// Simulates a PCF857x expander over the II2cBusDriver register-access primitives. Unlike DS3231,
// these chips are write-only from the firmware's perspective: a transmission is just "write N
// bytes starting at the register pointer" (there is no register-select step). `chipRegister`
// models the chip's actual output latch, kept separate from the firmware's in-memory bitmask so
// tests can simulate the chip silently losing its state (e.g. on a brownout).
class FakePcf857xI2cDriver final : public II2cBusDriver {
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
        chipRegister = 0U;
        for (size_t index = 0; index < pendingWrite.size() && index < 4U; ++index) {
            chipRegister |= static_cast<uint32_t>(pendingWrite[index]) << (8U * index);
        }
        lastWriteSize = pendingWrite.size();
        ++writeCount;
        return 0U;
    }

    size_t requestFrom(uint8_t, size_t, bool) override {
        return 0U;
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
        return 0;
    }

    int read() override {
        return -1;
    }

    void flush() override {}

    bool present{true};
    uint8_t lastAddress{0};
    uint32_t chipRegister{0};
    uint32_t writeCount{0};
    size_t lastWriteSize{0};

private:
    std::vector<uint8_t> pendingWrite;
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

Pcf857xExpanderConfigV2 makeExpanderConfig(uint8_t i2cAddress = 0x20U, bool inverted = false) {
    Pcf857xExpanderConfigV2 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "expander");
    config.i2cAddress = i2cAddress;
    config.inverted = inverted ? 1U : 0U;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeBusPayload(const I2cBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, config, buffer, i2cBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, i2cBusDeviceConfigSize(config)));
    return payload;
}

template <typename Config> BoundedBlob<kMaxDeviceConfigBytes> encodeExpanderPayload(const Config& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Config::kMagic, config, buffer, pcf857xExpanderConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, pcf857xExpanderConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeSwitchPayload(const PortExpanderSwitchDeviceConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, config, buffer, portExpanderSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, portExpanderSwitchDeviceConfigSize(config)));
    return payload;
}

EWFM_LEGACY_CONFIG_USE_BEGIN
BoundedBlob<kMaxDeviceConfigBytes> encodeLegacySwitchPayload(const PortExpanderSwitchDevicePersistedConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    size_t pos = 0U;
    TEST_ASSERT_TRUE(appendFixedConfigSegment(SwitchDeviceConfigV1::kMagic, config.switchConfig, buffer, sizeof(buffer), pos));
    TEST_ASSERT_TRUE(
        appendFixedConfigSegment(PortExpanderSwitchDeviceConfigV1::kMagic, config.expanderConfig, buffer, sizeof(buffer), pos));
    TEST_ASSERT_TRUE(payload.assign(buffer, pos));
    return payload;
}
EWFM_LEGACY_CONFIG_USE_END

void driveBusReady(I2cBusDevice& bus, uint32_t startNow = 1U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
}

template <typename ExpanderT> void bindExpanderDependency(ExpanderT& expander, DeviceId expanderId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = expanderId;
    record.header.typeId = ExpanderT::descriptor().typeId;
    record.header.configVersion = ExpanderT::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::I2CBus, busId};
    record.status = DeviceStatus::Ready;
    expander.bindDeviceIdentity(record, encodeExpanderPayload(expander.config()));
}

void driveExpanderUntilReady(IDeviceRuntime& expander, uint32_t startNow = 10U) {
    expander.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && expander.status() != DeviceStatus::Ready; now += 1U) {
        expander.tick1s(now);
    }
}

void bindSwitchDependency(PortExpanderSwitchDevice& sw, DeviceId switchId, DeviceId expanderId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = switchId;
    record.header.typeId = PortExpanderSwitchDevice::descriptor().typeId;
    record.header.configVersion = PortExpanderSwitchDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::PortExpander, expanderId};
    record.status = DeviceStatus::Ready;
    sw.bindDeviceIdentity(record, DeviceConfigBlob{});
}

void driveSwitchUntilReady(PortExpanderSwitchDevice& sw, uint32_t startNow = 10U) {
    sw.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && sw.status() != DeviceStatus::Ready; now += 10U) {
        sw.tickFastLoop(now);
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

DeviceCreateRequest makeExpanderCreateRequest(DeviceTypeId typeId, uint32_t configVersion, const char* name, DeviceId busId,
                                              const Pcf857xExpanderConfigV2& config) {
    DeviceCreateRequest request{};
    request.typeId = typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::I2CBus, busId};
    request.configVersion = configVersion;
    request.configBlob = encodeExpanderPayload(config);
    return request;
}

DeviceCreateRequest makeSwitchCreateRequest(const char* name, DeviceId expanderId, uint8_t channel) {
    DeviceCreateRequest request{};
    request.typeId = PortExpanderSwitchDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::PortExpander, expanderId};
    request.configVersion = PortExpanderSwitchDevice::descriptor().currentConfigVersion;
    PortExpanderSwitchDeviceConfigV3 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    config.channel = channel;
    request.configBlob = encodeSwitchPayload(config);
    return request;
}

} // namespace

void test_pcf857x_expander_config_codec_json_and_validation() {
    const Pcf857xExpanderConfigV2 config = makeExpanderConfig(0x21U, true);
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeExpanderPayload(config);

    Pcf857xExpanderConfigV2 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(0x21U, decoded.i2cAddress);
    TEST_ASSERT_TRUE(decoded.inverted != 0U);
    TEST_ASSERT_EQUAL_STRING("expander", decoded.name);

    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    writePcf857xExpanderConfigJson(config, json);
    TEST_ASSERT_EQUAL_UINT8(0x21U, json["i2cAddress"].as<uint8_t>());
    TEST_ASSERT_TRUE(json["inverted"].as<bool>());

    Pcf857xExpanderConfigV2 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parsePcf857xExpanderConfigJson(json, parsed, error));
    TEST_ASSERT_EQUAL_UINT8(0x21U, parsed.i2cAddress);
    TEST_ASSERT_TRUE(parsed.validate().ok());

    const Pcf857xExpanderConfigV2 defaults{};
    TEST_ASSERT_EQUAL_UINT8(0x20U, defaults.i2cAddress);

    const Pcf857xExpanderConfigV2 outOfRange = makeExpanderConfig(0x80U);
    TEST_ASSERT_FALSE(outOfRange.validate().ok());
}

void test_pcf857x_expander_config_migrates_v1_to_v2() {
    // A legacy "PX857X1" blob must decode and migrate to V2, preserving its fields.
    EWFM_LEGACY_CONFIG_USE_BEGIN
    Pcf857xExpanderConfigV1 legacy{};
    legacy.enabled = 1U;
    std::snprintf(legacy.name, sizeof(legacy.name), "%s", "expander-legacy");
    legacy.i2cAddress = 0x21U;
    legacy.inverted = 1U;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = pcf857xExpanderConfigSize(legacy);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Pcf857xExpanderConfigV1::kMagic, legacy, buffer, size));
    EWFM_LEGACY_CONFIG_USE_END

    Pcf857xExpanderConfigV2 migrated{};
    TEST_ASSERT_TRUE(decodePcf857xExpanderConfig(buffer, size, migrated));
    TEST_ASSERT_EQUAL_STRING("expander-legacy", migrated.name);
    TEST_ASSERT_TRUE(migrated.enabled != 0U);
    TEST_ASSERT_EQUAL_UINT8(0x21U, migrated.i2cAddress);
    TEST_ASSERT_TRUE(migrated.inverted != 0U);
}

void test_port_expander_switch_config_codec_json_and_validation() {
    PortExpanderSwitchDeviceConfigV3 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "switch");
    config.channel = 3U;

    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeSwitchPayload(config);

    PortExpanderSwitchDeviceConfigV3 decoded{};
    TEST_ASSERT_TRUE(decodePortExpanderSwitchDeviceConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(3U, decoded.channel);

    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    TEST_ASSERT_EQUAL_UINT8(3U, json["channel"].as<uint8_t>());

    PortExpanderSwitchDeviceConfigV3 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parsed.parseJson(json, error));
    TEST_ASSERT_EQUAL_UINT8(3U, parsed.channel);

    TEST_ASSERT_TRUE(portExpanderSwitchChannelIsValid(kMaxPortExpanderChannel));
    TEST_ASSERT_FALSE(portExpanderSwitchChannelIsValid(static_cast<uint8_t>(kMaxPortExpanderChannel + 1U)));
}

void test_port_expander_switch_config_migrates_v1_blob() {
    PortExpanderSwitchDeviceConfigV3 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "legacy-switch");
    current.restorePreviousState = 1U;
    current.startupState = kSwitchOutputOn;
    current.safeState = kSwitchOutputOff;
    current.inverted = 1U;
    current.channel = 12U;

    EWFM_LEGACY_CONFIG_USE_BEGIN
    PortExpanderSwitchDevicePersistedConfigV1 legacy{};
    EWFM_LEGACY_CONFIG_USE_END
    legacy.switchConfig.enabled = current.enabled;
    std::snprintf(legacy.switchConfig.name, sizeof(legacy.switchConfig.name), "%s", current.name);
    legacy.switchConfig.restorePreviousState = current.restorePreviousState;
    legacy.switchConfig.startupState = 1U;
    legacy.switchConfig.safeState = 2U;
    legacy.switchConfig.inverted = current.inverted;
    legacy.expanderConfig.channel = current.channel;

    PortExpanderSwitchDeviceConfigV3 decoded{};
    const BoundedBlob<kMaxDeviceConfigBytes> legacyPayload = encodeLegacySwitchPayload(legacy);
    TEST_ASSERT_TRUE(decodePortExpanderSwitchDeviceConfig(legacyPayload.data(), legacyPayload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(current.enabled, decoded.enabled);
    TEST_ASSERT_EQUAL_STRING(current.name, decoded.name);
    TEST_ASSERT_TRUE(current.startupState == decoded.startupState);
    TEST_ASSERT_TRUE(current.safeState == decoded.safeState);
    TEST_ASSERT_EQUAL_UINT8(current.channel, decoded.channel);

    uint8_t currentBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, decoded, currentBuffer,
                                           portExpanderSwitchDeviceConfigSize(decoded)));
    TEST_ASSERT_TRUE(portExpanderSwitchDeviceConfigSize(decoded) < legacyPayload.size());
}

void test_port_expander_types_and_api_adapters_are_registered() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();

    const DeviceTypeDescriptor* pcf8574Descriptor = typeRegistry.find(Pcf8574ExpanderDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(pcf8574Descriptor);
    TEST_ASSERT_TRUE(pcf8574Descriptor->providedRoles.contains(DeviceRole::PortExpander));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("pcf8574_expander"));

    const DeviceTypeDescriptor* pcf8575Descriptor = typeRegistry.find(Pcf8575ExpanderDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(pcf8575Descriptor);
    TEST_ASSERT_TRUE(pcf8575Descriptor->providedRoles.contains(DeviceRole::PortExpander));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("pcf8575_expander"));

    const DeviceTypeDescriptor* switchDescriptor = typeRegistry.find(PortExpanderSwitchDevice::descriptor().typeId);
    TEST_ASSERT_NOT_NULL(switchDescriptor);
    TEST_ASSERT_TRUE(switchDescriptor->providedRoles.contains(DeviceRole::Switch));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("port_expander_switch"));
}

void test_port_expander_switch_registry_migrates_v1_blob_on_begin() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    I2cBusDeviceConfigV1 busConfig = makeBusConfig();
    Pcf857xExpanderConfigV2 expanderConfig = makeExpanderConfig();
    PortExpanderSwitchDeviceConfigV3 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "legacy-channel");
    current.restorePreviousState = 1U;
    current.startupState = kSwitchOutputOn;
    current.safeState = kSwitchOutputOff;
    current.inverted = 1U;
    current.channel = 3U;

    EWFM_LEGACY_CONFIG_USE_BEGIN
    PortExpanderSwitchDevicePersistedConfigV1 legacy{};
    EWFM_LEGACY_CONFIG_USE_END
    legacy.switchConfig.enabled = current.enabled;
    std::snprintf(legacy.switchConfig.name, sizeof(legacy.switchConfig.name), "%s", current.name);
    legacy.switchConfig.restorePreviousState = current.restorePreviousState;
    legacy.switchConfig.startupState = 1U;
    legacy.switchConfig.safeState = 2U;
    legacy.switchConfig.inverted = current.inverted;
    legacy.expanderConfig.channel = current.channel;

    DeviceRegistryEntry busRecord{};
    busRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    busRecord.header.deviceId = 7001U;
    busRecord.header.typeId = I2cBusDevice::descriptor().typeId;
    busRecord.header.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    busRecord.header.configRevision = 1U;
    busRecord.status = DeviceStatus::Ready;

    DeviceRegistryEntry expanderRecord{};
    expanderRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    expanderRecord.header.deviceId = 7002U;
    expanderRecord.header.typeId = Pcf8574ExpanderDevice::descriptor().typeId;
    expanderRecord.header.configVersion = Pcf8574ExpanderDevice::descriptor().currentConfigVersion;
    expanderRecord.header.configRevision = 1U;
    expanderRecord.depCount = 1U;
    expanderRecord.deps[0] = {DeviceRole::I2CBus, busRecord.header.deviceId};
    expanderRecord.status = DeviceStatus::Ready;

    DeviceRegistryEntry switchRecord{};
    switchRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    switchRecord.header.deviceId = 7003U;
    switchRecord.header.typeId = PortExpanderSwitchDevice::descriptor().typeId;
    switchRecord.header.configVersion = 1U;
    switchRecord.header.configRevision = 2U;
    switchRecord.depCount = 1U;
    switchRecord.deps[0] = {DeviceRole::PortExpander, expanderRecord.header.deviceId};
    switchRecord.status = DeviceStatus::Ready;

    DeviceRegistrySnapshot snapshot{};
    snapshot.records = {busRecord, expanderRecord, switchRecord};
    snapshot.indexEntries = {{busRecord.header.deviceId, busRecord.header.typeId},
                             {expanderRecord.header.deviceId, expanderRecord.header.typeId},
                             {switchRecord.header.deviceId, switchRecord.header.typeId}};
    DeviceConfigBlobMap configBlobs{};
    configBlobs[busRecord.header.deviceId] = encodeBusPayload(busConfig);
    configBlobs[expanderRecord.header.deviceId] = encodeExpanderPayload(expanderConfig);
    configBlobs[switchRecord.header.deviceId] = encodeLegacySwitchPayload(legacy);
    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());

    SequentialDeviceIdSource ids(8000U);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE_MESSAGE(registry.begin(0U).ok(), "registry begin failed");

    const PortExpanderSwitchDevice* runtime = static_cast<const PortExpanderSwitchDevice*>(registry.runtime(switchRecord.header.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_UINT32(PortExpanderSwitchDevice::descriptor().currentConfigVersion, runtime->configVersion());
    TEST_ASSERT_EQUAL_UINT8(current.channel, runtime->channel());
    TEST_ASSERT_EQUAL_STRING(current.name, runtime->config().name);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
}

void test_pcf8574_runtime_writes_single_byte_on_sync() {
    FakePcf857xI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8574ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 3001, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveExpanderUntilReady(expander);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(expander.status()));
    TEST_ASSERT_EQUAL_UINT32(1U, driver.writeCount);
    TEST_ASSERT_EQUAL_UINT(1U, driver.lastWriteSize);
    TEST_ASSERT_EQUAL_UINT8(0x20U, driver.lastAddress);
}

void test_pcf8575_runtime_writes_two_bytes_low_high_order_on_sync() {
    FakePcf857xI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8575ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 3002, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveExpanderUntilReady(expander);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(expander.status()));
    TEST_ASSERT_EQUAL_UINT32(1U, driver.writeCount);
    TEST_ASSERT_EQUAL_UINT(2U, driver.lastWriteSize);

    TEST_ASSERT_TRUE(expander.requestChannelState(9, true, expander.uptime()));
    // Channel 9 lives in the high byte (bit 1 of the second byte) - proves the two-byte
    // low/high write order, which a single-byte PCF8574 write could never exercise.
    TEST_ASSERT_EQUAL_UINT32((1U << 9), driver.chipRegister);
}

void test_port_expander_switch_requests_channel_state_updates_expander_bitmask_immediately() {
    FakePcf857xI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8574ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 3003, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveExpanderUntilReady(expander);

    PortExpanderSwitchDeviceConfigV3 switchConfig{};
    switchConfig.enabled = 1U;
    std::snprintf(switchConfig.name, sizeof(switchConfig.name), "%s", "ch3");
    switchConfig.channel = 3U;
    PortExpanderSwitchDevice sw(switchConfig);
    bindSwitchDependency(sw, 3010, expander.deviceId());
    sw.setDependencyRuntime(DeviceRole::PortExpander, &expander);
    driveSwitchUntilReady(sw);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(sw.status()));

    const uint32_t writesBeforeToggle = driver.writeCount;
    TEST_ASSERT_TRUE(sw.requestOutputState(kSwitchOutputOn, 20000U));
    TEST_ASSERT_TRUE(expander.isChannelOn(3));
    TEST_ASSERT_TRUE(driver.writeCount > writesBeforeToggle);
    TEST_ASSERT_EQUAL_UINT32((1U << 3), driver.chipRegister);
}

void test_pcf8574_periodically_resyncs_channel_states_surviving_simulated_chip_reset() {
    FakePcf857xI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8574ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 3004, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);

    uint32_t now = 10U;
    expander.begin(now);
    now += 1U;
    expander.tick1s(now); // Starting -> Syncing
    now += 1U;
    expander.tick1s(now); // Syncing -> Ready (performs the initial full-register write)
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(expander.status()));

    TEST_ASSERT_TRUE(expander.requestChannelState(2, true, now));
    TEST_ASSERT_EQUAL_UINT32((1U << 2), driver.chipRegister);

    // Simulate the chip losing its output state (e.g. a brownout) independent of firmware's
    // in-memory bitmask, which still remembers channel 2 as on.
    driver.chipRegister = 0U;
    const uint32_t writesBeforeWait = driver.writeCount;
    const uint32_t syncDeadline = now + 10000U; // matches Pcf857xExpanderDeviceBase's kSyncIntervalMs

    // Advance up to just before the 10s deadline - no unconditional resync should fire yet.
    for (; now < syncDeadline - 1U; now += 500U) {
        expander.tick1s(now);
    }
    TEST_ASSERT_EQUAL_UINT32(writesBeforeWait, driver.writeCount);
    TEST_ASSERT_EQUAL_UINT32(0U, driver.chipRegister);

    // Cross the deadline - the periodic safety-net resync restores the register from the
    // firmware's in-memory bitmask even though nothing "changed" from the switch's perspective.
    // Two ticks are needed past the deadline: the first transitions Ready -> Syncing, the second
    // is where Syncing actually performs the transaction and write (SM_GOTO only changes state,
    // it does not run the target state's body within the same tick).
    for (; now < syncDeadline + 1000U; now += 500U) {
        expander.tick1s(now);
    }
    TEST_ASSERT_TRUE(driver.writeCount > writesBeforeWait);
    TEST_ASSERT_EQUAL_UINT32((1U << 2), driver.chipRegister);
}

void test_pcf8574_runtime_retries_when_bus_write_fails_then_recovers() {
    FakePcf857xI2cDriver driver;
    driver.present = false;

    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8574ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 3005, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    expander.begin(10U);

    uint32_t now = 10U;
    const uint32_t faultDeadline = now + 40000U;
    for (; now < faultDeadline && expander.status() != DeviceStatus::Faulted; now += 100U) {
        expander.tick1s(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(expander.status()));

    driver.present = true;
    const uint32_t recoveryDeadline = now + 40000U;
    for (; now < recoveryDeadline && expander.status() != DeviceStatus::Ready; now += 100U) {
        expander.tick1s(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(expander.status()));
}

void test_port_expander_switch_adapter_rejects_duplicate_channel_on_same_expander_but_allows_distinct_channels() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(5000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult busResult = registry.create(makeBusCreateRequest("i2c"), 10);
    TEST_ASSERT_TRUE_MESSAGE(busResult.ok(), busResult.validation.message);

    DeviceCreateResult expanderResult = registry.create(makeExpanderCreateRequest(Pcf8574ExpanderDevice::descriptor().typeId,
                                                                                  Pcf8574ExpanderDevice::descriptor().currentConfigVersion,
                                                                                  "expander", busResult.deviceId, makeExpanderConfig()),
                                                        11);
    TEST_ASSERT_TRUE_MESSAGE(expanderResult.ok(), expanderResult.validation.message);

    const DeviceCreateResult first = registry.create(makeSwitchCreateRequest("ch0", expanderResult.deviceId, 0U), 20);
    TEST_ASSERT_TRUE_MESSAGE(first.ok(), first.validation.message);

    const DeviceCreateRequest duplicateChannel = makeSwitchCreateRequest("ch0-dup", expanderResult.deviceId, 0U);
    const DeviceValidationResult duplicateValidation =
        PortExpanderSwitchDeviceApiAdapter::instance().validateCreateRequest(duplicateChannel, registry);
    TEST_ASSERT_FALSE(duplicateValidation.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(duplicateValidation.error));

    const DeviceCreateRequest distinctChannel = makeSwitchCreateRequest("ch1", expanderResult.deviceId, 1U);
    const DeviceValidationResult distinctValidation =
        PortExpanderSwitchDeviceApiAdapter::instance().validateCreateRequest(distinctChannel, registry);
    TEST_ASSERT_TRUE_MESSAGE(distinctValidation.ok(), distinctValidation.message);
}

void test_port_expander_switch_can_depend_on_either_pcf8574_or_pcf8575() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(6000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult busResult = registry.create(makeBusCreateRequest("i2c"), 10);
    TEST_ASSERT_TRUE_MESSAGE(busResult.ok(), busResult.validation.message);

    DeviceCreateResult pcf8574Result = registry.create(makeExpanderCreateRequest(Pcf8574ExpanderDevice::descriptor().typeId,
                                                                                 Pcf8574ExpanderDevice::descriptor().currentConfigVersion,
                                                                                 "pcf8574", busResult.deviceId, makeExpanderConfig(0x20U)),
                                                       11);
    TEST_ASSERT_TRUE_MESSAGE(pcf8574Result.ok(), pcf8574Result.validation.message);

    DeviceCreateResult pcf8575Result = registry.create(makeExpanderCreateRequest(Pcf8575ExpanderDevice::descriptor().typeId,
                                                                                 Pcf8575ExpanderDevice::descriptor().currentConfigVersion,
                                                                                 "pcf8575", busResult.deviceId, makeExpanderConfig(0x21U)),
                                                       12);
    TEST_ASSERT_TRUE_MESSAGE(pcf8575Result.ok(), pcf8575Result.validation.message);

    const DeviceCreateResult switchOn74 = registry.create(makeSwitchCreateRequest("sw-74", pcf8574Result.deviceId, 0U), 20);
    TEST_ASSERT_TRUE_MESSAGE(switchOn74.ok(), switchOn74.validation.message);

    const DeviceCreateResult switchOn75 = registry.create(makeSwitchCreateRequest("sw-75", pcf8575Result.deviceId, 15U), 21);
    TEST_ASSERT_TRUE_MESSAGE(switchOn75.ok(), switchOn75.validation.message);

    // Channel 15 is out of range for an 8-channel PCF8574 but valid for a 16-channel PCF8575 -
    // proves the switch's dependency validation is generic over channelCount(), not hardcoded to
    // one chip family.
    const DeviceCreateRequest invalidChannelOn74 = makeSwitchCreateRequest("sw-74-bad", pcf8574Result.deviceId, 15U);
    const DeviceValidationResult invalidValidation =
        PortExpanderSwitchDeviceApiAdapter::instance().validateCreateRequest(invalidChannelOn74, registry);
    TEST_ASSERT_FALSE(invalidValidation.ok());
}

void test_pcf8574_api_adapter_writes_runtime_json() {
    FakePcf857xI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8574ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 3006, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveExpanderUntilReady(expander);
    TEST_ASSERT_TRUE(expander.requestChannelState(1, true, expander.uptime()));

    StaticJsonDocument<1024> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    Pcf8574ExpanderDeviceApiAdapter::instance().writeDeviceJson(expander, expander.status(), output);
    TEST_ASSERT_EQUAL_STRING("pcf8574_expander", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(0x20U, output["config"]["i2cAddress"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(8U, output["runtime"]["channelCount"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT32((1U << 1), output["runtime"]["channelStates"].as<uint32_t>());
}

void test_port_expander_switch_api_adapter_writes_runtime_json() {
    FakePcf857xI2cDriver driver;
    I2cBusDevice bus(makeBusConfig(), driver);
    driveBusReady(bus);

    Pcf8574ExpanderDevice expander(makeExpanderConfig());
    bindExpanderDependency(expander, 3007, bus.deviceId());
    expander.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    driveExpanderUntilReady(expander);

    PortExpanderSwitchDeviceConfigV3 switchConfig{};
    switchConfig.enabled = 1U;
    std::snprintf(switchConfig.name, sizeof(switchConfig.name), "%s", "ch5");
    switchConfig.channel = 5U;
    PortExpanderSwitchDevice sw(switchConfig);
    bindSwitchDependency(sw, 3011, expander.deviceId());
    sw.setDependencyRuntime(DeviceRole::PortExpander, &expander);
    driveSwitchUntilReady(sw);
    TEST_ASSERT_TRUE(sw.requestOutputState(kSwitchOutputOn, 20000U));

    StaticJsonDocument<1024> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    PortExpanderSwitchDeviceApiAdapter::instance().writeDeviceJson(sw, sw.status(), output);
    TEST_ASSERT_EQUAL_STRING("port_expander_switch", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(5U, output["config"]["channel"].as<uint8_t>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["state"].as<bool>());
}
