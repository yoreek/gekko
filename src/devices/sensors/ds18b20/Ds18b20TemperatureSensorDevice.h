#pragma once

#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/sensors/ds18b20/Ds18b20OneWireProtocol.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorConfig.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

namespace ewfm {

class Ds18b20TemperatureSensorDevice final : public DeviceRuntimeBase {
public:
    explicit Ds18b20TemperatureSensorDevice(const DeviceRecord& record);
    explicit Ds18b20TemperatureSensorDevice(const Ds18b20TemperatureSensorConfigV1& config);

    const Ds18b20TemperatureSensorConfigV1& config() const;
    const TemperatureReading& reading() const;
    const char* outputStatus() const;
    uint8_t consecutiveErrors() const;
    uint32_t lastParentGeneration() const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRecord& record);
    static DeviceValidationResult validateConfig(const DeviceRecord& record);

private:
    enum class ParentAccessResult : uint8_t {
        Ready = 0,
        Missing = 1,
        Busy = 2,
    };

    State Idle();
    State Starting();
    State PowerUpDelay();
    State ConfigureSensor();
    State RequestConversion();
    State WaitConversion();
    State ReadScratchpad();
    State Ready();
    State RetryBackoff();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    OneWireBusDevice* parentBus() const;
    bool parentBusReady() const;
    bool parentGenerationChanged() const;
    ParentAccessResult beginParentTransaction(OneWireBusDevice::ChildTransaction& transaction) const;
    bool readScratchpad(IOneWireBusDriver& driver, uint8_t (&scratchpad)[kDs18b20ScratchpadSize], const char*& error) const;
    bool configureSensor(IOneWireBusDriver& driver, const char*& error) const;
    bool requestConversion(IOneWireBusDriver& driver, const char*& error) const;
    bool readTemperature(IOneWireBusDriver& driver, int32_t& milliCelsius, const char*& error) const;
    void publishReading(int32_t milliCelsius, uint32_t now);
    void invalidateReading(const char* status);
    void recordFailure(const char* status, uint32_t now);
    void deferRetry(uint32_t now);
    TemperatureUnit outputUnit() const;

    Ds18b20TemperatureSensorConfigV1 config_{};
    TemperatureReading reading_{};
    const char* outputStatus_{"not_ready"};
    uint32_t lastParentGeneration_{0};
    uint32_t conversionDeadline_{0};
    uint32_t nextPollAt_{0};
    uint32_t retryDeadline_{0};
    uint8_t consecutiveErrors_{0};
};

} // namespace ewfm
