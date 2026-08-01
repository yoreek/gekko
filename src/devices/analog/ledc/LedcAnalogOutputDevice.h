#pragma once

#include "devices/analog/AnalogOutputDeviceBase.h"
#include "devices/analog/ledc/LedcAnalogOutputDeviceConfig.h"

namespace ewfm {

class LedcAnalogOutputDevice final : public AnalogOutputDeviceBase {
public:
    LedcAnalogOutputDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit LedcAnalogOutputDevice(const LedcAnalogOutputDeviceConfigV1& config);

    const LedcAnalogOutputDeviceConfigV1& config() const override;

    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    void claimGpioPins(DeviceId* pins) const override;
    void releaseGpioPins(DeviceId* pins) const override;

protected:
    DeviceValidationResult configureHardware(uint32_t now) override;
    DeviceValidationResult applyHardwareOutput(uint16_t state, uint32_t now) override;
    void releaseHardware(uint32_t now) override;

private:
    uint32_t dutyMax() const;
    uint32_t outputStateToDuty(uint16_t state) const;

    LedcAnalogOutputDeviceConfigV1 config_{};
    bool hardwareReady_{false};
};

} // namespace ewfm
