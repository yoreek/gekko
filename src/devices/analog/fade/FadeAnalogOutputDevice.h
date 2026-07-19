#pragma once

#include "devices/analog/AnalogOutputDecoratorDeviceBase.h"
#include "devices/analog/fade/FadeAnalogOutputDeviceConfig.h"

namespace ewfm {

class FadeAnalogOutputDevice final : public AnalogOutputDecoratorDeviceBase {
public:
    FadeAnalogOutputDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit FadeAnalogOutputDevice(const FadeAnalogOutputDeviceConfigV1& config);

    const FadeAnalogOutputDeviceConfigV1& config() const;
    uint16_t targetOutputState() const;
    bool transitioning() const;

    bool requestOutputState(uint16_t state, uint32_t now) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;
    void onReadyTick(uint32_t now) override;
    void onTargetAttached(uint32_t now) override;

    FadeAnalogOutputDeviceConfigV1 config_{};
    uint16_t targetState_{0U};
    uint32_t lastStepAt_{0U};
    bool targetInitialized_{false};
};

} // namespace ewfm
