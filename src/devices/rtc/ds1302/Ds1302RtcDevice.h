#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/rtc/common/RtcDeviceRuntimeBase.h"
#include "devices/rtc/ds1302/Ds1302LineDriver.h"
#include "devices/rtc/ds1302/Ds1302RtcDeviceConfig.h"

#include <ArduinoJson.h>
#include <memory>

namespace ewfm {

class Ds1302RtcDevice;
using Ds1302RtcDeviceBase = RtcDeviceRuntimeBase<Ds1302RtcDevice, DeviceRuntimeBase>;

// DS1302 hardware RTC, bit-banged over 3 direct GPIO pins (CLK/DAT/RST) - no shared bus, unlike
// Ds3231RtcDevice's I2C bus-dependency template chain. Shares RtcDeviceRuntimeBase's bookkeeping
// with Ds3231RtcDevice; the dependency-bus states are simply absent here since this device owns its
// pins outright and has nothing to wait ready for.
class Ds1302RtcDevice final : public Ds1302RtcDeviceBase {
public:
    Ds1302RtcDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    Ds1302RtcDevice(const Ds1302RtcDeviceConfigV1& config, IDs1302LineDriver& lineDriver);

    const Ds1302RtcDeviceConfigV1& config() const;
    bool writeTime(uint32_t utcEpoch) override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    void claimGpioPins(DeviceId* pins) const override;
    void releaseGpioPins(DeviceId* pins) const override;

private:
    State Idle();
    State Starting();
    State Reading();
    State Ready();
    State RetryBackoff();
    State Disabled();
    State Faulted();
    State Deleting();

    Ds1302RtcDeviceConfigV1 config_{};
    IDs1302LineDriver& lineDriver_;
};

} // namespace ewfm
