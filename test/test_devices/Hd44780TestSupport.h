#pragma once

#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/ConfigCodec.h"
#include "devices/display/lcd1602/Lcd1602DeviceConfig.h"
#include "devices/display/lcd2004/Lcd2004DeviceConfig.h"
#include "devices/expander/Pcf8574ExpanderDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "metrics/MetricValueResolver.h"
#include "wifi/WifiDriver.h"

#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <unity.h>
#include <vector>

namespace ewfm::test_support {

class FakeI2cDriver final : public II2cBusDriver {
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
    }
    uint8_t endTransmission(bool) override {
        ++writeCount;
        return 0U;
    }
    size_t requestFrom(uint8_t, size_t, bool) override {
        return 0U;
    }
    size_t write(uint8_t) override {
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

    uint8_t lastAddress{0};
    uint32_t writeCount{0};
};

class FakeWifiDriver final : public IWifiDriver {
public:
    bool begin() override {
        return true;
    }
    bool beginStation(const WiFiCredentials&) override {
        return true;
    }
    void disconnect() override {}
    void clearStationCredentials() override {}
    bool startSetupAp(const std::string&, const std::string&) override {
        return true;
    }
    void stopSetupAp() override {}
    WifiDriverStatus status() const override {
        return WifiDriverStatus::Connected;
    }
    bool networkStackReady() const override {
        return true;
    }
    bool stationReady() const override {
        return true;
    }
    bool setupApReady() const override {
        return false;
    }
    std::string stationIp() const override {
        return "192.168.1.50";
    }
    std::string setupApIp() const override {
        return "";
    }
    bool startScan() override {
        return true;
    }
    bool scanComplete(std::vector<WifiNetwork>&, size_t) override {
        return false;
    }
    std::string macSuffix() const override {
        return "abcd";
    }
};

inline I2cBusDeviceConfigV1 makeBusConfig() {
    I2cBusDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "i2c-bus");
    config.sdaPin = 18;
    config.sclPin = 19;
    config.internalPullup = 1U;
    config.frequencyHz = 400000U;
    return config;
}

inline Pcf857xExpanderConfigV2 makeExpanderConfig(uint8_t i2cAddress = 0x20U) {
    Pcf857xExpanderConfigV2 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "expander");
    config.i2cAddress = i2cAddress;
    return config;
}

inline PortExpanderSwitchDeviceConfigV3 makeSwitchConfig(const char* name, uint8_t channel) {
    PortExpanderSwitchDeviceConfigV3 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    config.channel = channel;
    return config;
}

inline BoundedBlob<kMaxDeviceConfigBytes> encodeBusPayload(const I2cBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, config, buffer, i2cBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, i2cBusDeviceConfigSize(config)));
    return payload;
}

inline BoundedBlob<kMaxDeviceConfigBytes> encodeExpanderPayload(const Pcf857xExpanderConfigV2& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, config, buffer, pcf857xExpanderConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, pcf857xExpanderConfigSize(config)));
    return payload;
}

inline BoundedBlob<kMaxDeviceConfigBytes> encodeSwitchPayload(const PortExpanderSwitchDeviceConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, config, buffer, portExpanderSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, portExpanderSwitchDeviceConfigSize(config)));
    return payload;
}

inline void driveBusReady(I2cBusDevice& bus, uint32_t startNow = 1U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
}

inline void bindExpanderDependency(Pcf8574ExpanderDevice& expander, DeviceId expanderId, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = expanderId;
    record.header.typeId = Pcf8574ExpanderDevice::descriptor().typeId;
    record.header.configVersion = Pcf8574ExpanderDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::I2CBus, busId};
    record.status = DeviceStatus::Ready;
    expander.bindDeviceIdentity(record, encodeExpanderPayload(expander.config()));
}

inline void driveExpanderUntilReady(IDeviceRuntime& expander, uint32_t startNow = 10U) {
    expander.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && expander.status() != DeviceStatus::Ready; now += 1U) {
        expander.tick1s(now);
    }
}

inline DeviceCreateRequest makeBusCreateRequest(const char* name) {
    DeviceCreateRequest request{};
    request.typeId = I2cBusDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeBusPayload(makeBusConfig());
    return request;
}

inline DeviceCreateRequest makeExpanderCreateRequest(const char* name, DeviceId busId) {
    DeviceCreateRequest request{};
    request.typeId = Pcf8574ExpanderDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::I2CBus, busId};
    request.configVersion = Pcf8574ExpanderDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeExpanderPayload(makeExpanderConfig());
    return request;
}

inline DeviceCreateRequest makeSwitchCreateRequest(const char* name, DeviceId expanderId, uint8_t channel) {
    DeviceCreateRequest request{};
    request.typeId = PortExpanderSwitchDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::PortExpander, expanderId};
    request.configVersion = PortExpanderSwitchDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeSwitchPayload(makeSwitchConfig(name, channel));
    return request;
}

inline void bindSwitchDependency(PortExpanderSwitchDevice& switchDevice, DeviceId switchId, DeviceId expanderId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = switchId;
    record.header.typeId = PortExpanderSwitchDevice::descriptor().typeId;
    record.header.configVersion = PortExpanderSwitchDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceRole::PortExpander, expanderId};
    record.status = DeviceStatus::Ready;
    switchDevice.bindDeviceIdentity(record, encodeSwitchPayload(switchDevice.config()));
}

inline void driveSwitchUntilReady(PortExpanderSwitchDevice& switchDevice, uint32_t startNow = 20U) {
    switchDevice.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 5000U && switchDevice.status() != DeviceStatus::Ready; now += 10U) {
        switchDevice.tickFastLoop(now);
    }
}

inline std::unique_ptr<PortExpanderSwitchDevice> makeSwitchRuntime(const char* name, DeviceId switchId, DeviceId expanderId,
                                                                   uint8_t channel, IDeviceRuntime& expanderRuntime) {
    auto switchDevice = std::make_unique<PortExpanderSwitchDevice>(makeSwitchConfig(name, channel));
    bindSwitchDependency(*switchDevice, switchId, expanderId);
    switchDevice->setDependencyRuntime(DeviceRole::PortExpander, &expanderRuntime);
    driveSwitchUntilReady(*switchDevice);
    return switchDevice;
}

} // namespace ewfm::test_support
