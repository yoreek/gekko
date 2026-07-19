#pragma once

#include "devices/expander/Pcf857xExpanderDeviceBase.h"

namespace ewfm {

// PCF8574: 8-channel I2C GPIO expander, single-byte register write.
class Pcf8574ExpanderDevice final : public Pcf857xExpanderDeviceBase {
public:
    Pcf8574ExpanderDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Pcf8574ExpanderDevice(const Pcf857xExpanderConfigV1& config);
    explicit Pcf8574ExpanderDevice(const Pcf857xExpanderConfigV2& config);

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    uint8_t channelCountImpl() const override;
    bool writeChannelStates(II2cBusDriver& driver, uint32_t states) const override;
};

} // namespace ewfm
