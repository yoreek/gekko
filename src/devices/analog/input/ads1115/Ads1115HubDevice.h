#pragma once

#include "devices/analog/input/ads1115/Ads1115HubDeviceConfig.h"
#include "devices/bus/BusDependentDeviceBase.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"

#include <ArduinoJson.h>

namespace ewfm {

class Ads1115HubDevice;
using Ads1115HubRuntimeBase = I2cDeviceRuntimeBase<Ads1115HubDevice, BusDependentDeviceBase<I2cBusDevice, DeviceRole::I2CBus>>;

// An ADS1115 16-bit I2C ADC, presented as an AnalogInputHub with 4 single-ended channels
// (AIN0..AIN3). Each pollChannelReading call drives one step of the single-shot
// write-config -> wait-conversion-time -> read-conversion protocol; only one channel's conversion
// is ever in flight, matching the chip's single physical ADC core.
class Ads1115HubDevice final : public Ads1115HubRuntimeBase, public IAnalogInputHubRuntime {
public:
    Ads1115HubDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Ads1115HubDevice(const Ads1115HubDeviceConfigV1& config);

    const Ads1115HubDeviceConfigV1& config() const;
    const IAnalogInputHubRuntime* analogInputHubRuntime() const override;
    bool hasDuplicateDependentChannel(uint8_t channel, const IDeviceRuntime* ignoreDependent = nullptr) const override;

    uint8_t channelCount() const override;
    uint32_t generation() const override;
    AnalogInputHubPollResult pollChannelReading(uint8_t channel, DeviceId requester, uint32_t now, AnalogInputReading& outReading,
                                                const char*& outStatus) override;
    void releaseChannelRequest(uint8_t channel, DeviceId requester) override;

    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    enum class ConversionPhase : uint8_t { Idle, Converting };

    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Ready();
    State RetryBackoff();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    void releaseOwnership();
    void bumpGeneration();
    void recordFailure(uint32_t now);

    Ads1115HubDeviceConfigV1 config_{};
    uint32_t lastDependencyGeneration_{0};
    uint32_t ownGeneration_{0};
    uint32_t retryDeadline_{0};
    uint8_t consecutiveErrors_{0};

    uint8_t ownerChannel_{0xFF};
    DeviceId ownerRequester_{0};
    ConversionPhase phase_{ConversionPhase::Idle};
    uint32_t conversionDeadline_{0};
};

} // namespace ewfm
