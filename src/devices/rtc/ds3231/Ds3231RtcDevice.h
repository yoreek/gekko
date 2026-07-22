#pragma once

#include "devices/bus/BusDependentDeviceBase.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"
#include "devices/rtc/ds3231/Ds3231RtcDeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

// DS3231 hardware RTC, read over I2C (address configurable, defaults to the conventional 0x68).
// Mirrors Ds18b20TemperatureSensorDevice's shape: single bus dependency, tick100ms-driven poll,
// retry backoff on transient I2C failures. Unlike DS18B20 there is no async conversion delay, so
// a single "Reading" state performs the whole register transaction per poll.
class Ds3231RtcDevice;
using Ds3231RtcDeviceBase = I2cDeviceRuntimeBase<Ds3231RtcDevice, BusDependentDeviceBase<I2cBusDevice, DeviceRole::I2CBus>>;

class Ds3231RtcDevice final : public Ds3231RtcDeviceBase, public IRealTimeClockRuntime {
public:
    Ds3231RtcDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Ds3231RtcDevice(const Ds3231RtcDeviceConfigV2& config);

    const Ds3231RtcDeviceConfigV2& config() const;
    const IRealTimeClockRuntime* realTimeClockRuntime() const override;
    bool latestTimeReading(uint32_t& outUtcEpoch, bool& outLostPower) const override;
    bool writeTime(uint32_t utcEpoch) override;
    bool useForSystemTimeSync() const override;
    bool lastReadOk() const;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Reading();
    State Ready();
    State RetryBackoff();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    bool readTimeRegisters(II2cBusDriver& driver, uint32_t& outEpoch, bool& outLostPower) const;
    bool writeTimeRegisters(II2cBusDriver& driver, uint32_t utcEpoch) const;
    void recordSuccess(uint32_t epoch, bool lostPower);
    void recordFailure(uint32_t now);

    Ds3231RtcDeviceConfigV2 config_{};
    uint32_t lastDependencyGeneration_{0};
    uint32_t nextPollAt_{0};
    uint32_t retryDeadline_{0};
    uint8_t consecutiveErrors_{0};
    uint32_t lastEpoch_{0};
    bool lastLostPower_{false};
    bool hasReading_{false};
    bool lastReadOk_{false};
};

} // namespace ewfm
