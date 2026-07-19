#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/sensors/binary/BinarySensorDeviceConfig.h"
#include "devices/sensors/binary/IGpioInputDriver.h"

#include <ArduinoJson.h>

namespace ewfm {

// A debounced digital GPIO input (float switch, leak probe, door contact, ...) exposed as a
// Condition-role provider, so anything that consumes IStatusRuntime (auto_switch conditions,
// dosing pump low-level sensor) can depend on it uniformly. The raw pin level must stay stable
// for debounceMs before the reported level flips; the very first successful read seeds the
// stable level immediately so consumers see a real value right after startup instead of a
// debounce-window blind spot. isActive() = stable level XOR inverted.
class BinarySensorDevice final : public DeviceRuntimeBase, public IStatusRuntime {
public:
    BinarySensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    BinarySensorDevice(const BinarySensorDeviceConfigV1& config, IGpioInputDriver& driver);

    const BinarySensorDeviceConfigV1& config() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    const IStatusRuntime* statusRuntime() const override {
        return this;
    }

    bool isActive() const override;
    bool hasReading() const;
    bool rawLevel() const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    bool configureHardware();
    void releaseHardware();
    void resetSampling();
    void sampleInput(uint32_t now);

    BinarySensorDeviceConfigV1 config_{};
    IGpioInputDriver& driver_;
    bool hardwareConfigured_{false};
    bool hasStableLevel_{false};
    bool stableLevel_{false};
    bool candidateLevel_{false};
    uint32_t candidateSinceMs_{0U};
    bool rawLevel_{false};
};

} // namespace ewfm
