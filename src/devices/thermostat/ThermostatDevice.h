#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/thermostat/ThermostatDeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class ThermostatDevice final : public DeviceRuntimeBase {
public:
    ThermostatDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit ThermostatDevice(const ThermostatDeviceConfigV1& config);

    const ThermostatDeviceConfigV1& config() const;
    bool enabled() const override;
    const char* name() const override;
    const TemperatureReading& latestTemperature() const;
    OutputState desiredOutputState() const;
    OutputState actualOutputState() const;
    const char* controlStatus() const;
    uint32_t lastCheckAtMs() const;
    void setDependencyRuntime(DeviceDependencyRole role, IDeviceRuntime* dependencyRuntime) override;
    void setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    void writeDeviceJson(JsonObject output) const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    State Idle();
    State Starting();
    State Reconfiguring();
    State Ready();
    State CheckTemperature();
    State ApplyOutput();
    State RetryBackoff();
    State DependencyBlocked();
    State Disabled();
    State Faulted();
    State Deleting();

    void refreshCapabilityCache();
    bool dependenciesAvailable() const;
    bool dependenciesReady() const;
    bool dependencyIsDisabled() const;
    bool dependencyBlocked() const;
    bool sensorReadingTimedOut(uint32_t now) const;
    bool sensorReadingInRange(const TemperatureReading& reading) const;
    OutputState desiredStateForTemperature(const TemperatureReading& reading) const;
    bool requestSwitchOutput(OutputState state, uint32_t now, bool safetyOff);
    bool requestSafeOff(uint32_t now);
    void publishTemperature(const TemperatureReading& reading, const char* status);
    void invalidateTemperature(const char* status);
    void scheduleNextCheck(uint32_t now);
    bool minSwitchIntervalElapsed(uint32_t now) const;
    bool switchSupports(OutputState state) const;
    bool switchCapable() const;

    ThermostatDeviceConfigV1 config_{};
    TemperatureReading latestTemperature_{};
    OutputState desiredOutputState_{OutputState::Off};
    OutputState actualOutputState_{OutputState::Off};
    OutputState pendingOutputState_{OutputState::Off};
    const ITemperatureReadingRuntime* temperatureSensor_{nullptr};
    ISwitchOutputRuntime* switchOutput_{nullptr};
    const char* controlStatus_{"not_ready"};
    uint32_t lastCheckAtMs_{0};
    uint32_t nextCheckAtMs_{0};
    uint32_t sensorInvalidSinceMs_{0};
    uint32_t retryDeadlineMs_{0};
    uint32_t lastOrdinarySwitchChangeAtMs_{0};
    bool pendingSafetyOff_{false};
};

} // namespace ewfm
