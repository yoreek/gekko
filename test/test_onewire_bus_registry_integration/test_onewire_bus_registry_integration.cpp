#include "config/MemoryConfigStorage.h"
#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"

#include <array>
#include <cstdio>
#include <unity.h>

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
        return true;
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

    void select(const OneWireRomAddress&) override {}

    void skip() override {}

    void write(uint8_t, bool = false) override {}

    uint8_t read() override {
        return 0;
    }

    uint8_t readBit() override {
        return 0;
    }

    uint8_t crc8(const uint8_t* data, size_t len) const override {
        uint8_t crc = 0;
        for (size_t index = 0; index < len; ++index) {
            uint8_t inbyte = data[index];
            for (uint8_t bit = 0; bit < 8; ++bit) {
                const uint8_t mix = static_cast<uint8_t>((crc ^ inbyte) & 0x01U);
                crc >>= 1U;
                if (mix != 0U) {
                    crc ^= 0x8CU;
                }
                inbyte >>= 1U;
            }
        }
        return crc;
    }

    bool beginOk{true};
    bool began{false};
    bool depowered{false};
    uint8_t lastPin{0};
    bool lastInternalPullup{false};
    size_t searchIndex{0};
    std::vector<uint8_t> releasePins{};
    std::vector<OneWireRomAddress> candidates{};
};

FakeOneWireBusDriver* gDriver = nullptr;

BoundedBlob<kMaxDeviceConfigBytes> encodeOneWirePayload(const OneWireBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeOneWireBusDeviceConfig(config, buffer, oneWireBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, oneWireBusDeviceConfigSize(config)));
    return payload;
}

std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    OneWireBusDeviceConfigV1 config{};
    TEST_ASSERT_TRUE(decodeOneWireBusDeviceConfig(reinterpret_cast<const uint8_t*>(configBlob.data()), configBlob.size(), config));
    return std::unique_ptr<IDeviceRuntime>(new OneWireBusDevice(config, *gDriver));
}

OneWireRomAddress makeRom(uint8_t family, const std::array<uint8_t, 6>& serial, const FakeOneWireBusDriver& driver) {
    OneWireRomAddress address{};
    address.bytes[0] = family;
    for (size_t index = 0; index < serial.size(); ++index) {
        address.bytes[index + 1] = serial[index];
    }
    address.bytes[7] = driver.crc8(address.bytes, 7);
    return address;
}

OneWireBusDeviceConfigV1 makeConfig(uint8_t pin) {
    OneWireBusDeviceConfigV1 config{};
    config.base.enabled = 1;
    std::snprintf(config.base.name, sizeof(config.base.name), "%s", "onewire");
    config.gpioPin = pin;
    config.internalPullup = 0;
    return config;
}

DeviceCreateRequest makeCreateRequest(uint8_t pin) {
    DeviceCreateRequest request{};
    request.typeId = OneWireBusDevice::descriptor().typeId;
    request.name = "onewire";
    request.enabled = true;
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeOneWirePayload(makeConfig(pin));
    request.persistencePolicy = DevicePersistencePolicy::Delayed;
    return request;
}

const OneWireBusDevice* runtimeAsOneWire(DeviceRegistry& registry, DeviceId id) {
    return static_cast<const OneWireBusDevice*>(registry.runtime(id));
}

} // namespace

void test_onewire_registry_create_scan_reconfigure_disable_and_delete() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(1000);

    FakeOneWireBusDriver driver;
    const OneWireRomAddress valid = makeRom(0x28, {0xFF, 0x64, 0x1D, 0x62, 0x16, 0x03}, driver);
    driver.candidates = {valid};
    gDriver = &driver;

    DeviceTypeRegistry types;
    DeviceTypeDescriptor descriptor = OneWireBusDevice::descriptor();
    descriptor.createRuntime = &createRuntime;
    TEST_ASSERT_TRUE(types.registerDescriptor(descriptor));

    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult created = registry.create(makeCreateRequest(4), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    TEST_ASSERT_NOT_NULL(registry.runtime(created.deviceId));

    registry.tick100ms(11);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(registry.effectiveStatus(created.deviceId)));
    TEST_ASSERT_TRUE(registry.command(DeviceCommand{DeviceCommandType::Scan, created.deviceId, ""}, 20).ok());
    TEST_ASSERT_FALSE(registry.command(DeviceCommand{DeviceCommandType::Scan, created.deviceId, ""}, 21).ok());

    registry.tick100ms(22);
    registry.tick100ms(23);
    const OneWireBusDevice* runtime = runtimeAsOneWire(registry, created.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_TRUE(runtime->scan().ready);
    TEST_ASSERT_EQUAL_UINT8(1, runtime->scan().deviceCount);

    DeviceMutationResult disabled = registry.setEnabled(created.deviceId, false, 30, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(disabled.ok(), disabled.validation.message);
    registry.tick100ms(31);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(registry.effectiveStatus(created.deviceId)));
    runtime = runtimeAsOneWire(registry, created.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_FALSE(runtime->scan().ready);
    TEST_ASSERT_EQUAL_UINT8(0, runtime->scan().deviceCount);

    DeviceMutationResult updated =
        registry.updateConfig(created.deviceId, encodeOneWirePayload(makeConfig(17)), OneWireBusDevice::descriptor().currentConfigVersion,
                              40, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(updated.ok(), updated.validation.message);
    registry.tick100ms(41);
    runtime = runtimeAsOneWire(registry, created.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_UINT8(17, runtime->config().gpioPin);

    DeviceMutationResult removed = registry.remove(created.deviceId, 50, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE_MESSAGE(removed.ok(), removed.validation.message);
    TEST_ASSERT_NULL(registry.runtime(created.deviceId));
}

void test_onewire_registry_update_config_restarts_bus_and_advances_generation() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(1100);

    FakeOneWireBusDriver driver;
    gDriver = &driver;

    DeviceTypeRegistry types;
    DeviceTypeDescriptor descriptor = OneWireBusDevice::descriptor();
    descriptor.createRuntime = &createRuntime;
    TEST_ASSERT_TRUE(types.registerDescriptor(descriptor));

    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult created = registry.create(makeCreateRequest(4), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    registry.tick100ms(11);

    const OneWireBusDevice* runtimeBefore = runtimeAsOneWire(registry, created.deviceId);
    TEST_ASSERT_NOT_NULL(runtimeBefore);
    const uint32_t generationBefore = runtimeBefore->generation();
    TEST_ASSERT_TRUE(driver.began);
    TEST_ASSERT_EQUAL_UINT8(4, driver.lastPin);

    OneWireBusDeviceConfigV1 pinChange = makeConfig(17);
    DeviceMutationResult pinUpdated =
        registry.updateConfig(created.deviceId, encodeOneWirePayload(pinChange), OneWireBusDevice::descriptor().currentConfigVersion, 20,
                              DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(pinUpdated.ok(), pinUpdated.validation.message);
    TEST_ASSERT_TRUE(driver.depowered);
    registry.tick100ms(21);
    registry.tick100ms(22);
    const OneWireBusDevice* runtimeAfterPin = runtimeAsOneWire(registry, created.deviceId);
    TEST_ASSERT_NOT_NULL(runtimeAfterPin);
    TEST_ASSERT_TRUE(runtimeAfterPin->generation() > generationBefore);
    TEST_ASSERT_EQUAL_UINT8(17, driver.lastPin);
    const uint32_t generationAfterPin = runtimeAfterPin->generation();

    OneWireBusDeviceConfigV1 pullupChange = pinChange;
    pullupChange.internalPullup = 1;
    DeviceMutationResult pullupUpdated =
        registry.updateConfig(created.deviceId, encodeOneWirePayload(pullupChange), OneWireBusDevice::descriptor().currentConfigVersion, 30,
                              DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(pullupUpdated.ok(), pullupUpdated.validation.message);
    registry.tick100ms(31);
    registry.tick100ms(32);
    const OneWireBusDevice* runtimeAfterPullup = runtimeAsOneWire(registry, created.deviceId);
    TEST_ASSERT_NOT_NULL(runtimeAfterPullup);
    TEST_ASSERT_TRUE(runtimeAfterPullup->generation() > generationAfterPin);
    TEST_ASSERT_TRUE(driver.lastInternalPullup);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_onewire_registry_create_scan_reconfigure_disable_and_delete);
    RUN_TEST(test_onewire_registry_update_config_restarts_bus_and_advances_generation);
    return UNITY_END();
}
