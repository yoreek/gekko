#pragma once

#include "devices/analog/adc/IAdcInputDriver.h"
#include "devices/analog/input/AdcSampleAccumulator.h"
#include "devices/analog/input/AnalogInputReadingPublisher.h"
#include "devices/analog/input/port/AnalogPortInputDeviceConfig.h"
#include "devices/core/DeviceRuntimeBase.h"

#include <ArduinoJson.h>

namespace ewfm {

// The ESP32's own ADC exposed as an AnalogInput-role device: reads one pin directly, with no hub
// in between. See AnalogInputHubChannelDeviceBase for the hub-dependent counterpart (ADS1115,
// CD74HC4067).
class AnalogPortInputDevice final : public DeviceRuntimeBase, public IAnalogInputRuntime {
public:
    AnalogPortInputDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit AnalogPortInputDevice(const AnalogPortInputDeviceConfigV1& config);
    AnalogPortInputDevice(const AnalogPortInputDeviceConfigV1& config, IAdcInputDriver& driver);

    const AnalogPortInputDeviceConfigV1& config() const;
    const AnalogInputReading& reading() const;
    const char* outputStatus() const;
    bool latestAnalogInputReading(AnalogInputReading& reading) const override;
    const char* latestAnalogInputStatus() const override;
    const IAnalogInputRuntime* analogInputRuntime() const override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
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

    bool configurePin();
    void releaseHardware(uint32_t now);
    void performReading(uint32_t now);

    AnalogPortInputDeviceConfigV1 config_{};
    IAdcInputDriver& driver_;
    AdcSampleAccumulator accumulator_{};
    AnalogInputReadingPublisher publisher_{};
    uint32_t nextPollAt_{0};
};

} // namespace ewfm
