#pragma once

#include "devices/analog/adc/IAdcInputDriver.h"
#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDeviceConfig.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/switch/gpio/IGpioOutputDriver.h"

#include <ArduinoJson.h>

namespace ewfm {

// A CD74HC4067 16-channel analog multiplexer, presented as an AnalogInputHub: one shared SIG line
// (read through IAdcInputDriver) selected by 4 address lines S0-S3 (+ an optional active-low EN,
// through IGpioOutputDriver). Only one channel can be electrically selected at a time, so
// pollChannelReading arbitrates single-owner access and never blocks -- switching the address
// lines costs one tick (settle time), a read costs the next. No I2C/SPI bus dependency: this hub
// owns its GPIO pins directly, the same way GpioSwitchDevice does.
class Cd74hc4067HubDevice final : public DeviceRuntimeBase, public IAnalogInputHubRuntime {
public:
    Cd74hc4067HubDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Cd74hc4067HubDevice(const Cd74hc4067HubDeviceConfigV1& config);
    Cd74hc4067HubDevice(const Cd74hc4067HubDeviceConfigV1& config, IGpioOutputDriver& gpioDriver, IAdcInputDriver& adcDriver);

    const Cd74hc4067HubDeviceConfigV1& config() const;
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
    void end(uint32_t now) override;

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
    void bumpGeneration();

    Cd74hc4067HubDeviceConfigV1 config_{};
    IGpioOutputDriver& gpioDriver_;
    IAdcInputDriver& adcDriver_;
    uint8_t muxSelectedChannel_{kCd74hc4067UnusedPin};
    uint8_t ownerChannel_{kCd74hc4067UnusedPin};
    DeviceId ownerRequester_{0};
    uint32_t generation_{0};
};

} // namespace ewfm
