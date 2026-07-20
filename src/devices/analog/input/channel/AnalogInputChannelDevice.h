#pragma once

#include "devices/analog/input/AnalogInputHubChannelDeviceBase.h"
#include "devices/analog/input/channel/AnalogInputChannelDeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

// One channel of whatever hub is wired up via the generic DeviceRole::AnalogInputHub dependency --
// see AnalogInputHubChannelDeviceBase for the shared polling/arbitration protocol. Hub-agnostic by
// design: the exact same device works unchanged against an Ads1115HubDevice or a
// Cd74hc4067HubDevice (or any future AnalogInputHub), so there is exactly one channel typeId, not
// one per hub chip -- the attached hub's own channelCount() is what actually bounds `channel`,
// enforced dynamically by the REST adapter (validateAnalogInputHubDependency), not by this type.
class AnalogInputChannelDevice final : public AnalogInputHubChannelDeviceBase {
public:
    AnalogInputChannelDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit AnalogInputChannelDevice(const AnalogInputChannelDeviceConfigV1& config);

    const AnalogInputChannelDeviceConfigV1& config() const;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    uint8_t channel() const override;
    uint8_t adcSampleCount() const override;
    bool reportAlways() const override;
    uint16_t reportDeltaMilliVolts() const override;
    uint32_t pollIntervalMs() const override;
    bool channelEnabled() const override;

    AnalogInputChannelDeviceConfigV1 config_{};
};

} // namespace ewfm
