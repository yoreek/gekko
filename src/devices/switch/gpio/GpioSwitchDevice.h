#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/switch/SwitchDeviceBase.h"
#include "devices/switch/gpio/GpioSwitchDeviceConfig.h"
#include "devices/switch/gpio/IGpioOutputDriver.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

class GpioSwitchDevice final : public SwitchDeviceBase {
public:
    GpioSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    GpioSwitchDevice(const GpioSwitchDeviceConfigV3& config, IGpioOutputDriver& driver);

    uint8_t gpioPin() const;
    const GpioSwitchDeviceConfigV3& config() const override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    DeviceValidationResult configureHardware(uint32_t now) override;
    DeviceValidationResult applyHardwareOutput(bool state, uint32_t now) override;
    void releaseHardware(uint32_t now) override;
    SwitchDeviceConfigV2& mutableConfig() override;

    GpioSwitchDeviceConfigV3 config_{};
    IGpioOutputDriver& driver_;
};

} // namespace ewfm
