#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/switch/BinarySwitchDeviceBase.h"
#include "devices/switch/TriStateSwitchDeviceBase.h"
#include "devices/thermostat/ThermostatDevice.h"
#include "devices/thermostat/ThermostatDeviceConfig.h"
#include "integrations/rest/thermostat/ThermostatDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

struct OutputWrite {
    OutputState state{OutputState::Off};
    bool physicalLevel{false};
    uint32_t now{0};
};

class FakeTemperatureSensor final : public DeviceRuntimeBase, public ITemperatureReadingRuntime {
public:
    FakeTemperatureSensor() : DeviceRuntimeBase((PState)&FakeTemperatureSensor::Idle) {
        status_ = DeviceStatus::Ready;
    }

    void setReading(int32_t milliCelsius, uint32_t measuredAtMs, bool valid = true) {
        reading_.milliCelsius = milliCelsius;
        reading_.measuredAtMs = measuredAtMs;
        reading_.valid = valid;
    }

    void setDeviceStatus(DeviceStatus status) {
        status_ = status;
    }

    void begin(uint32_t now) override {
        (void)now;
        status_ = DeviceStatus::Ready;
    }

    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }
    bool latestTemperatureReading(TemperatureReading& reading) const override {
        reading = reading_;
        return true;
    }
    const char* latestTemperatureStatus() const override {
        return status_ == DeviceStatus::Ready ? "ok" : "not_ready";
    }
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override {
        return this;
    }

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }

    TemperatureReading reading_{};
};

class FakeMissingCapabilityRuntime final : public DeviceRuntimeBase {
public:
    FakeMissingCapabilityRuntime() : DeviceRuntimeBase((PState)&FakeMissingCapabilityRuntime::Idle) {
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

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }
};

class FakeTriStateSwitch final : public TriStateSwitchDeviceBase {
public:
    explicit FakeTriStateSwitch(const SwitchDeviceConfigV1& config) : TriStateSwitchDeviceBase(config) {}

    uint8_t configureCalls{0};
    uint8_t releaseCalls{0};
    bool configureOk{true};
    bool applyOk{true};
    std::vector<OutputWrite> writes{};

private:
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = switchDeviceConfigSize(switchConfig());
        return encodeSwitchDeviceConfig(switchConfig(), buffer, size) && configBlob.assign(buffer, size);
    }

    DeviceValidationResult configureHardware(uint32_t now) override {
        (void)now;
        ++configureCalls;
        return configureOk ? DeviceValidationResult{} : DeviceValidationResult{DeviceError::InvalidConfig, "configure failed"};
    }

    DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) override {
        writes.push_back({state, physicalLevel, now});
        return applyOk ? DeviceValidationResult{} : DeviceValidationResult{DeviceError::StorageError, "apply failed"};
    }

    void releaseHardware(uint32_t now) override {
        (void)now;
        ++releaseCalls;
    }
};

struct RegistrySwitchRuntime final : public TriStateSwitchDeviceBase {
    RegistrySwitchRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
        : RegistrySwitchRuntime([&configBlob]() {
              SwitchDeviceConfigV1 config{};
              (void)decodeSwitchDeviceConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }()) {
        bindDeviceIdentity(record, configBlob);
    }

    explicit RegistrySwitchRuntime(const SwitchDeviceConfigV1& config) : TriStateSwitchDeviceBase(config) {}

    std::vector<OutputWrite> writes{};

private:
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = switchDeviceConfigSize(switchConfig());
        return encodeSwitchDeviceConfig(switchConfig(), buffer, size) && configBlob.assign(buffer, size);
    }

    DeviceValidationResult configureHardware(uint32_t now) override {
        (void)now;
        return {};
    }

    DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) override {
        writes.push_back({state, physicalLevel, now});
        return {};
    }

    void releaseHardware(uint32_t now) override {
        (void)now;
    }
};

std::unique_ptr<IDeviceRuntime> createRegistrySwitchRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new RegistrySwitchRuntime(record, configBlob));
}

DeviceValidationResult validateRegistrySwitchConfig(const DeviceRegistryEntry&, const DeviceConfigBlob& configBlob) {
    SwitchDeviceConfigV1 config{};
    if (!decodeSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "switch config is invalid"};
    }
    return {};
}

SwitchDeviceConfigV1 makeSwitchConfig(OutputState startup = OutputState::Off, OutputState safe = OutputState::Off) {
    SwitchDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "switch");
    config.restorePreviousState = true;
    config.startupState = startup;
    config.safeState = safe;
    config.inverted = false;
    return config;
}

ThermostatDeviceConfigV1 makeThermostatConfig(ThermostatMode mode, int32_t targetMilliCelsius) {
    ThermostatDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "thermostat");
    config.mode = static_cast<uint8_t>(mode);
    config.algorithm = static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis);
    config.targetMilliCelsius = targetMilliCelsius;
    config.minSafeMilliCelsius = 0;
    config.maxSafeMilliCelsius = 50000;
    config.hysteresisCentiCelsius = 50;
    config.checkIntervalMs = 100;
    config.sensorTimeoutMs = 200;
    config.retryAfterErrorMs = 300;
    config.minSwitchIntervalMs = 150;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeThermostatPayload(const ThermostatDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeThermostatDeviceConfig(config, buffer, thermostatDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, thermostatDeviceConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeSwitchPayload(const SwitchDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeSwitchDeviceConfig(config, buffer, switchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, switchDeviceConfigSize(config)));
    return payload;
}

void startSwitch(FakeTriStateSwitch& device) {
    device.begin(10);
    device.tickFastLoop(11);
}

void startThermostat(ThermostatDevice& device, uint32_t now = 10) {
    device.begin(now);
    device.tick100ms(now + 1U);
}

void bindThermostatIdentity(ThermostatDevice& device, DeviceId thermostatId, DeviceId temperatureSensorId, DeviceId switchId) {
    const BoundedBlob<kMaxDeviceConfigBytes> configBlob = encodeThermostatPayload(device.config());
    DeviceRegistryEntry record{};
    record.header.deviceId = thermostatId;
    record.header.typeId = kThermostatDeviceTypeId;
    record.header.configVersion = kThermostatDeviceConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.depCount = 2U;
    record.deps[0] = {DeviceDependencyRole::TemperatureSensor, temperatureSensorId};
    record.deps[1] = {DeviceDependencyRole::Switch, switchId};
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, configBlob);
}

} // namespace

void test_thermostat_config_codec_and_validation() {
    ThermostatDeviceConfigV1 config = makeThermostatConfig(ThermostatMode::Heat, 25000);
    config.hysteresisCentiCelsius = 75;
    config.checkIntervalMs = 1000;
    config.sensorTimeoutMs = 2000;
    config.retryAfterErrorMs = 3000;
    config.minSwitchIntervalMs = 4000;

    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeThermostatPayload(config);
    ThermostatDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeThermostatDeviceConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(config.mode, decoded.mode);
    TEST_ASSERT_EQUAL_UINT8(config.algorithm, decoded.algorithm);
    TEST_ASSERT_EQUAL_INT32(config.targetMilliCelsius, decoded.targetMilliCelsius);
    TEST_ASSERT_EQUAL_UINT16(config.hysteresisCentiCelsius, decoded.hysteresisCentiCelsius);
    TEST_ASSERT_TRUE(validateThermostatDeviceConfig(decoded).ok());

    decoded.targetMilliCelsius = decoded.maxSafeMilliCelsius + 1;
    TEST_ASSERT_FALSE(validateThermostatDeviceConfig(decoded).ok());
}

void test_thermostat_parser_accepts_spa_milli_celsius_fields() {
    DynamicJsonDocument doc(512);
    JsonObject input = doc.to<JsonObject>();
    input["mode"] = "heat";
    input["algorithm"] = "hysteresis";
    input["targetCelsius"] = 28.0;
    input["minSafeCelsius"] = 0.0;
    input["maxSafeCelsius"] = 50.0;
    input["hysteresisCelsius"] = 0.5;
    input["checkIntervalMs"] = 1000;
    input["sensorTimeoutMs"] = 6000;
    input["retryAfterErrorMs"] = 30000;
    input["minSwitchIntervalMs"] = 5000;

    ThermostatDeviceConfigV1 config = makeThermostatConfig(ThermostatMode::Heat, 25000);
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parseThermostatDeviceConfigJson(input, config, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_INT32(28000, config.targetMilliCelsius);
    TEST_ASSERT_EQUAL_INT32(0, config.minSafeMilliCelsius);
    TEST_ASSERT_EQUAL_INT32(50000, config.maxSafeMilliCelsius);
    TEST_ASSERT_EQUAL_UINT16(50, config.hysteresisCentiCelsius);
}

void test_thermostat_api_adapter_partial_update_preserves_mode_and_thresholds() {
    ThermostatDevice thermostat(makeThermostatConfig(ThermostatMode::Heat, 25000));

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["targetCelsius"] = 24.0;

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        ThermostatDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), thermostat, request, error), error);

    ThermostatDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeThermostatDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                   parsed));
    TEST_ASSERT_EQUAL_INT32(24000, parsed.targetMilliCelsius);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ThermostatMode::Heat), parsed.mode);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis), parsed.algorithm);
    TEST_ASSERT_EQUAL_UINT16(50, parsed.hysteresisCentiCelsius);
    TEST_ASSERT_EQUAL_UINT32(100, parsed.checkIntervalMs);
    TEST_ASSERT_EQUAL_UINT32(200, parsed.sensorTimeoutMs);
    TEST_ASSERT_EQUAL_UINT32(300, parsed.retryAfterErrorMs);
    TEST_ASSERT_EQUAL_UINT32(150, parsed.minSwitchIntervalMs);
}

void test_device_type_registry_contains_thermostat() {
    DeviceTypeRegistry registry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = registry.find(kThermostatDeviceTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("ThermostatDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->ticks100ms);
    TEST_ASSERT_FALSE(descriptor->supportsCommands);
}

void test_runtime_capabilities_are_exposed() {
    FakeTemperatureSensor sensor;
    FakeTriStateSwitch switchDevice(makeSwitchConfig());

    TemperatureReading reading{};
    TEST_ASSERT_NOT_NULL(sensor.temperatureReadingRuntime());
    TEST_ASSERT_TRUE(sensor.temperatureReadingRuntime()->latestTemperatureReading(reading));
    TEST_ASSERT_NOT_NULL(switchDevice.switchOutputRuntime());
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(switchDevice.switchOutputRuntime()->currentOutputState()));
}

void test_registry_captures_retained_state_after_internal_switch_output_change() {
    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    DeviceRegistryStore registryStore(storage);
    TEST_ASSERT_TRUE(registryStore.begin(false));

    DeviceTypeRegistry types;
    DeviceTypeDescriptor descriptor{};
    descriptor.typeId = 77;
    descriptor.name = "RegistrySwitchRuntime";
    descriptor.currentConfigVersion = 1;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.createRuntime = &createRegistrySwitchRuntime;
    descriptor.validateConfig = &validateRegistrySwitchConfig;
    TEST_ASSERT_TRUE(types.registerDescriptor(descriptor));

    SequentialDeviceIdSource ids(500);
    DeviceRegistry registry(registryStore, types, ids, &retainedStore);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    SwitchDeviceConfigV1 config = makeSwitchConfig();
    DeviceCreateRequest request{};
    request.typeId = descriptor.typeId;
    request.name = "registry-switch";
    request.enabled = true;
    request.configVersion = 1;
    request.configBlob = encodeSwitchPayload(config);

    const DeviceCreateResult created = registry.create(request, 20);
    TEST_ASSERT_TRUE(created.ok());
    registry.tickFastLoop(21);

    IDeviceRuntime* runtime = registry.runtime(created.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);
    ISwitchOutputRuntime* switchRuntime = const_cast<ISwitchOutputRuntime*>(runtime->switchOutputRuntime());
    TEST_ASSERT_NOT_NULL(switchRuntime);
    TEST_ASSERT_TRUE(switchRuntime->requestOutputState(OutputState::On, 21));

    registry.tickFastLoop(22);
    TEST_ASSERT_EQUAL_UINT32(1U, registry.dirtyRetainedStateIds().size());
    TEST_ASSERT_EQUAL_UINT32(created.deviceId, registry.dirtyRetainedStateIds()[0]);
}

void test_thermostat_heats_holds_and_cools() {
    ThermostatDevice thermostat(makeThermostatConfig(ThermostatMode::Heat, 25000));
    FakeTemperatureSensor sensor;
    FakeTriStateSwitch switchDevice(makeSwitchConfig());

    bindThermostatIdentity(thermostat, 10U, 11U, 12U);
    thermostat.setDependencyRuntime(DeviceDependencyRole::TemperatureSensor, &sensor);
    thermostat.setDependencyRuntime(DeviceDependencyRole::Switch, &switchDevice);
    startSwitch(switchDevice);
    startThermostat(thermostat, 100);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(thermostat.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(switchDevice.outputState()));

    sensor.setReading(24000, 110);
    thermostat.tick100ms(200);
    thermostat.tick100ms(201);
    thermostat.tick100ms(202);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(switchDevice.outputState()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(thermostat.desiredOutputState()));

    sensor.setReading(25200, 210);
    thermostat.tick100ms(303);
    thermostat.tick100ms(304);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(switchDevice.outputState()));

    sensor.setReading(26000, 350);
    thermostat.tick100ms(405);
    thermostat.tick100ms(406);
    thermostat.tick100ms(407);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(switchDevice.outputState()));
}

void test_thermostat_waits_for_sensor_timeout_and_resets_on_valid_reading() {
    ThermostatDevice thermostat(makeThermostatConfig(ThermostatMode::Cool, 22000));
    FakeTemperatureSensor sensor;
    FakeTriStateSwitch switchDevice(makeSwitchConfig());

    bindThermostatIdentity(thermostat, 20U, 21U, 22U);
    thermostat.setDependencyRuntime(DeviceDependencyRole::TemperatureSensor, &sensor);
    thermostat.setDependencyRuntime(DeviceDependencyRole::Switch, &switchDevice);
    startSwitch(switchDevice);
    sensor.setReading(24000, 0, false);
    startThermostat(thermostat, 100);
    auto tickRange = [&](uint32_t from, uint32_t to) {
        for (uint32_t now = from; now <= to; ++now) {
            thermostat.tick100ms(now);
        }
    };

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(thermostat.status()));
    TEST_ASSERT_TRUE(std::strcmp(thermostat.controlStatus(), "not_ready") == 0);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(switchDevice.outputState()));

    tickRange(200, 210);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(thermostat.status()));
    TEST_ASSERT_TRUE(std::strcmp(thermostat.controlStatus(), "sensor_timeout") == 0);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(switchDevice.outputState()));

    sensor.setReading(24000, 250, true);
    tickRange(301, 305);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(thermostat.status()));
    TEST_ASSERT_TRUE(std::strcmp(thermostat.controlStatus(), "ok") == 0);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(switchDevice.outputState()));

    sensor.setReading(24000, 0, false);
    tickRange(402, 700);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(thermostat.status()));
    TEST_ASSERT_TRUE(std::strcmp(thermostat.controlStatus(), "retry_backoff") == 0);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(switchDevice.outputState()));

    ThermostatDevice blocked(makeThermostatConfig(ThermostatMode::Heat, 25000));
    FakeMissingCapabilityRuntime missingSensor;
    FakeTriStateSwitch blockedSwitch(makeSwitchConfig());
    bindThermostatIdentity(blocked, 30U, 31U, 32U);
    blocked.setDependencyRuntime(DeviceDependencyRole::TemperatureSensor, &missingSensor);
    blocked.setDependencyRuntime(DeviceDependencyRole::Switch, &blockedSwitch);
    startSwitch(blockedSwitch);
    startThermostat(blocked, 1000);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(blocked.status()));
}
