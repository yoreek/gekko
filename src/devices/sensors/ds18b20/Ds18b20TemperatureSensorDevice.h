#pragma once

#include "devices/bus/BusDependentDeviceBase.h"
#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/sensors/ds18b20/Ds18b20OneWireProtocol.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorConfig.h"
#include "devices/sensors/temperature/TemperatureReadingPublisher.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>

namespace ewfm {

class Ds18b20TemperatureSensorDevice final : public BusDependentDeviceBase<OneWireBusDevice, DeviceRole::OneWireBus>,
                                             public ITemperatureReadingRuntime {
public:
    Ds18b20TemperatureSensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Ds18b20TemperatureSensorDevice(const Ds18b20TemperatureSensorConfigV1& config);

    const Ds18b20TemperatureSensorConfigV1& config() const;
    const TemperatureReading& reading() const;
    const char* outputStatus() const;
    uint8_t consecutiveErrors() const;
    uint32_t lastDependencyGeneration() const;
    bool latestTemperatureReading(TemperatureReading& reading) const override;
    const char* latestTemperatureStatus() const override;
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

    bool oneWireRomAddress(OneWireRomAddress& address) const override;

private:
    const DeviceBaseConfigV1& baseConfig() const override;

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

    bool readScratchpad(IOneWireBusDriver& driver, uint8_t (&scratchpad)[kDs18b20ScratchpadSize], const char*& error) const;
    bool configureSensor(IOneWireBusDriver& driver, const char*& error) const;
    bool requestConversion(IOneWireBusDriver& driver, const char*& error) const;
    bool readTemperature(IOneWireBusDriver& driver, int32_t& milliCelsius, const char*& error) const;
    bool hasDuplicateOneWireAddress() const;
    void publishReading(int32_t milliCelsius, uint32_t now);
    void invalidateReading(const char* status);
    void recordFailure(const char* status, uint32_t now);
    void deferRetry(uint32_t now);
    TemperatureUnit outputUnit() const;

    Ds18b20TemperatureSensorConfigV1 config_{};
    TemperatureReadingPublisher publisher_{};
    uint32_t lastDependencyGeneration_{0};
    uint32_t conversionDeadline_{0};
    uint32_t nextPollAt_{0};
    uint32_t retryDeadline_{0};
    uint8_t consecutiveErrors_{0};
};

} // namespace ewfm
