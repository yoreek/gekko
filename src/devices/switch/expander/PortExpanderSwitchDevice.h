#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/switch/SwitchDeviceBase.h"
#include "devices/switch/expander/PortExpanderSwitchDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

// A single channel of any PCF857x-family port expander, presented as an ordinary output switch, so it can be depended on by
// anything requiring DeviceRole::Switch (e.g. Thermostat) with zero consumer-side changes. Hardware
// access goes through the DeviceRole::PortExpander dependency's IPortExpanderRuntime interface
// instead of a raw GPIO driver -- this class is deliberately expander-implementation-agnostic, so
// it works unchanged with a PCF8574, a PCF8575, or any future expander that implements that role.
class PortExpanderSwitchDevice final : public SwitchDeviceBase {
public:
    PortExpanderSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit PortExpanderSwitchDevice(const PortExpanderSwitchDeviceConfigV3& config);

    uint8_t channel() const;
    const PortExpanderSwitchDeviceConfigV3& config() const override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    bool expanderChannel(uint8_t& channel) const override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    DeviceValidationResult configureHardware(uint32_t now) override;
    DeviceValidationResult applyHardwareOutput(bool state, uint32_t now) override;
    void releaseHardware(uint32_t now) override;
    SwitchDeviceConfigV2& mutableConfig() override;

    IPortExpanderRuntime* dependencyExpander() const;

    PortExpanderSwitchDeviceConfigV3 config_{};
};

} // namespace ewfm
