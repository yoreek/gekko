#pragma once

#include "devices/bus/BusDependentDeviceBase.h"
#include "devices/bus/BusRuntimeDiagnostics.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"
#include "devices/expander/Pcf857xIoDriver.h"
#include "devices/expander/PortExpanderConfig.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

// Shared behavior for the PCF857x family of I2C GPIO expanders (PCF8574 = 8 channels, PCF8575 =
// 16 channels). These chips are treated as write-only: there is no meaningful register read-back,
// so channel state lives entirely in `driver_` (Pcf857xIoDriver, shared with the embedded PCF8574
// transport used by Hd44780I2cCharacterDisplayDeviceBase). Every requested channel change is
// written to the chip immediately, and the whole bitmask is additionally re-pushed unconditionally
// every `kSyncIntervalMs` as a safety net, because these parts are known to occasionally reset
// (e.g. on a brownout) and silently lose their output state.
//
// Mirrors Ds3231RtcDevice's shape: a single required I2C-bus dependency, obtained per-operation via
// I2cBusDevice::DependencyTransaction, with retry/backoff on transient bus failures. Concrete leaves
// (Pcf8574ExpanderDevice, Pcf8575ExpanderDevice) only supply the channel count, via the base
// constructor.
class Pcf857xExpanderDeviceBase;
using Pcf857xExpanderRuntimeBase =
    I2cDeviceRuntimeBase<Pcf857xExpanderDeviceBase, BusDependentDeviceBase<I2cBusDevice, DeviceRole::I2CBus>>;

class Pcf857xExpanderDeviceBase : public Pcf857xExpanderRuntimeBase, public IPortExpanderRuntime {
public:
    const Pcf857xExpanderConfigV2& config() const;
    bool hasDuplicateDependentChannel(uint8_t channel, const IDeviceRuntime* ignoreDependent = nullptr) const override;
    const IPortExpanderRuntime* portExpanderRuntime() const override;
    uint8_t channelCount() const override;
    bool isChannelOn(uint8_t channel) const override;
    bool requestChannelState(uint8_t channel, bool on, uint32_t now) override;
    uint32_t channelStatesBitmask() const;
    const BusRuntimeDiagnostics& diagnostics() const;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

protected:
    const DeviceBaseConfigV1& baseConfig() const override;

    Pcf857xExpanderDeviceBase(const Pcf857xExpanderConfigV2& config, uint8_t channelCount);

private:
    State Idle();
    State Starting();
    State Syncing();
    State Ready();
    State RetryBackoff();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    bool writeCurrentStates(II2cBusDriver& driver) const;
    void recordSuccess();
    void recordFailure(uint32_t now);

    Pcf857xExpanderConfigV2 config_{};
    Pcf857xIoDriver driver_;
    BusRuntimeDiagnostics diagnostics_{};
    uint32_t lastDependencyGeneration_{0};
    uint32_t nextSyncAt_{0};
    uint32_t retryDeadline_{0};
    uint8_t consecutiveErrors_{0};
};

} // namespace ewfm
