#include "devices/bus/i2c/I2cAddress.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/i2c_bus/I2cBusDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeI2cBusDriver final : public II2cBusDriver {
public:
    bool begin(uint8_t sdaPin, uint8_t sclPin, uint32_t frequencyHz, bool internalPullup) override {
        began = true;
        ++beginCount;
        lastSdaPin = sdaPin;
        lastSclPin = sclPin;
        lastFrequencyHz = frequencyHz;
        lastInternalPullup = internalPullup;
        currentClockHz = frequencyHz;
        return beginOk;
    }

    bool end() override {
        ++endCount;
        ended = true;
        return endOk;
    }

    bool setClock(uint32_t frequencyHz) override {
        ++setClockCount;
        currentClockHz = frequencyHz;
        return setClockOk;
    }

    uint32_t getClock() const override {
        return currentClockHz;
    }

    void beginTransmission(uint8_t address) override {
        lastAddress = address;
    }

    uint8_t endTransmission(bool) override {
        return 0U;
    }

    size_t requestFrom(uint8_t, size_t size, bool) override {
        return size;
    }

    size_t write(uint8_t data) override {
        lastWrite = data;
        return 1U;
    }

    size_t write(const uint8_t*, size_t quantity) override {
        return quantity;
    }

    int available() override {
        return 0;
    }

    int read() override {
        return -1;
    }

    void flush() override {}

    bool beginOk{true};
    bool endOk{true};
    bool setClockOk{true};
    bool began{false};
    bool ended{false};
    uint32_t beginCount{0};
    uint32_t endCount{0};
    uint32_t setClockCount{0};
    uint8_t lastSdaPin{0};
    uint8_t lastSclPin{0};
    uint32_t lastFrequencyHz{0};
    bool lastInternalPullup{false};
    uint8_t lastAddress{0};
    uint8_t lastWrite{0};
    uint32_t currentClockHz{0};
};

class FakeI2cDependentRuntime final : public DeviceRuntimeBase {
public:
    explicit FakeI2cDependentRuntime(uint8_t address) : DeviceRuntimeBase((PState)&FakeI2cDependentRuntime::Idle), address_(address) {}

    bool i2cAddress(uint8_t& address) const override {
        address = address_;
        return true;
    }

    bool handleCommand(const DeviceCommand&) override {
        return false;
    }

private:
    State Idle();
    uint8_t address_{0};
};

#undef SM_CLASS
#define SM_CLASS FakeI2cDependentRuntime
SM_STATE(FakeI2cDependentRuntime::Idle) {
    status_ = DeviceStatus::Ready;
}
#undef SM_CLASS

I2cBusDeviceConfigV1 makeConfig(uint8_t sdaPin = 18, uint8_t sclPin = 19, bool internalPullup = true, uint32_t frequencyHz = 400000U) {
    I2cBusDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "i2c-bus");
    config.sdaPin = sdaPin;
    config.sclPin = sclPin;
    config.internalPullup = internalPullup ? 1U : 0U;
    config.frequencyHz = frequencyHz;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodePayload(const I2cBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeI2cBusDeviceConfig(config, buffer, i2cBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, i2cBusDeviceConfigSize(config)));
    return payload;
}

void driveBusToReady(I2cBusDevice& bus, uint32_t startNow = 10U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
}

} // namespace

void test_i2c_address_helpers_format_parse_and_validate() {
    I2cAddress address{0x3CU};
    char formatted[3]{};
    TEST_ASSERT_TRUE(formatI2cAddress(address, formatted));
    TEST_ASSERT_EQUAL_STRING("3C", formatted);

    I2cAddress parsed{};
    TEST_ASSERT_TRUE(parseI2cAddress("0x3c", parsed));
    TEST_ASSERT_EQUAL_UINT8(0x3CU, parsed.value);
    TEST_ASSERT_TRUE(i2cAddressIsValid(parsed));

    TEST_ASSERT_FALSE(parseI2cAddress("0x80", parsed));
    TEST_ASSERT_FALSE(i2cAddressIsValid(I2cAddress{0x80U}));
}

void test_i2c_bus_config_codec_and_validation() {
    const I2cBusDeviceConfigV1 config = makeConfig();
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodePayload(config);

    I2cBusDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeI2cBusDeviceConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(config.sdaPin, decoded.sdaPin);
    TEST_ASSERT_EQUAL_UINT8(config.sclPin, decoded.sclPin);
    TEST_ASSERT_EQUAL_UINT8(config.internalPullup, decoded.internalPullup);
    TEST_ASSERT_EQUAL_UINT32(config.frequencyHz, decoded.frequencyHz);

    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    writeI2cBusDeviceConfigJson(config, json);

    I2cBusDeviceConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parseI2cBusDeviceConfigJson(json, parsed, error));
    TEST_ASSERT_EQUAL_UINT8(config.sdaPin, parsed.sdaPin);
    TEST_ASSERT_EQUAL_UINT8(config.sclPin, parsed.sclPin);
    TEST_ASSERT_TRUE(parsed.validate().ok());

    I2cBusDeviceConfigV1 invalid = config;
    invalid.sdaPin = invalid.sclPin;
    TEST_ASSERT_FALSE(invalid.validate().ok());
}

void test_i2c_default_registries_include_bus() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(typeRegistry.find(I2cBusDevice::descriptor().typeId));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(I2cBusDevice::descriptor().typeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("i2c_bus"));
}

void test_i2c_bus_runtime_lifecycle_and_duplicate_address_detection() {
    FakeI2cBusDriver driver;
    I2cBusDevice bus(makeConfig(), driver);
    driveBusToReady(bus);

    TEST_ASSERT_TRUE(driver.began);
    TEST_ASSERT_EQUAL_UINT8(18U, driver.lastSdaPin);
    TEST_ASSERT_EQUAL_UINT8(19U, driver.lastSclPin);
    TEST_ASSERT_EQUAL_UINT32(400000U, driver.lastFrequencyHz);
    TEST_ASSERT_TRUE(driver.lastInternalPullup);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
    TEST_ASSERT_EQUAL_UINT32(1U, bus.generation());

    I2cBusDevice::DependencyTransaction first = bus.beginDependencyTransaction();
    TEST_ASSERT_TRUE(first);
    TEST_ASSERT_TRUE(bus.dependencyTransactionActive());

    I2cBusDevice::DependencyTransaction second = bus.beginDependencyTransaction();
    TEST_ASSERT_FALSE(second);

    first.release();
    TEST_ASSERT_FALSE(bus.dependencyTransactionActive());

    FakeI2cDependentRuntime dependentA(0x3CU);
    FakeI2cDependentRuntime dependentB(0x27U);
    bus.attachDependentRuntime(&dependentA);
    bus.attachDependentRuntime(&dependentB);
    TEST_ASSERT_TRUE(bus.hasDuplicateDependentI2cAddress(0x3CU, nullptr));
    TEST_ASSERT_FALSE(bus.hasDuplicateDependentI2cAddress(0x28U, nullptr));
    TEST_ASSERT_FALSE(bus.hasDuplicateDependentI2cAddress(0x3CU, &dependentA));
}

void test_i2c_bus_runtime_reconfigures_and_advances_generation() {
    FakeI2cBusDriver driver;
    I2cBusDevice bus(makeConfig(), driver);
    driveBusToReady(bus);

    const uint32_t generationBefore = bus.generation();
    I2cBusDeviceConfigV1 updated = makeConfig(21U, 22U, false, 100000U);
    const BoundedBlob<kMaxDeviceConfigBytes> updatedPayload = encodePayload(updated);
    const DeviceConfigUpdatePlan plan = bus.planConfigUpdate(updatedPayload);
    TEST_ASSERT_TRUE(plan.endOldConfig);
    TEST_ASSERT_TRUE(plan.resetStateMachine);
    TEST_ASSERT_TRUE(bus.applyConfig(updatedPayload, 20U));

    bus.requestReconfigure();
    bus.tick100ms(21U);
    bus.tick100ms(22U);

    TEST_ASSERT_TRUE(driver.ended);
    TEST_ASSERT_EQUAL_UINT32(2U, driver.beginCount);
    TEST_ASSERT_EQUAL_UINT32(generationBefore + 1U, bus.generation());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
    TEST_ASSERT_EQUAL_UINT8(21U, driver.lastSdaPin);
    TEST_ASSERT_EQUAL_UINT8(22U, driver.lastSclPin);
    TEST_ASSERT_FALSE(driver.lastInternalPullup);
}

void test_i2c_bus_api_adapter_parses_and_serializes_runtime() {
    StaticJsonDocument<256> doc;
    doc["typeName"] = "i2c_bus";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "i2c-bus";
    config["enabled"] = true;
    config["sdaPin"] = 18;
    config["sclPin"] = 19;
    config["internalPullup"] = true;
    config["frequencyHz"] = 100000;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(I2cBusDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(I2cBusDevice::descriptor().typeId, request.typeId);

    I2cBusDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(
        decodeI2cBusDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(18U, parsed.sdaPin);
    TEST_ASSERT_EQUAL_UINT8(19U, parsed.sclPin);
    TEST_ASSERT_TRUE(parsed.internalPullup != 0U);

    FakeI2cBusDriver driver;
    I2cBusDevice runtime(makeConfig(), driver);
    runtime.begin(0U);
    runtime.tick100ms(1U);
    StaticJsonDocument<1024> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    I2cBusDeviceApiAdapter::instance().writeDeviceJson(runtime, runtime.status(), output);
    TEST_ASSERT_EQUAL_STRING("i2c_bus", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_TRUE(output["config"]["internalPullup"].as<bool>());
    TEST_ASSERT_EQUAL_UINT32(1U, output["runtime"]["generation"].as<uint32_t>());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_i2c_address_helpers_format_parse_and_validate);
    RUN_TEST(test_i2c_bus_config_codec_and_validation);
    RUN_TEST(test_i2c_default_registries_include_bus);
    RUN_TEST(test_i2c_bus_runtime_lifecycle_and_duplicate_address_detection);
    RUN_TEST(test_i2c_bus_runtime_reconfigures_and_advances_generation);
    RUN_TEST(test_i2c_bus_api_adapter_parses_and_serializes_runtime);
    return UNITY_END();
}
