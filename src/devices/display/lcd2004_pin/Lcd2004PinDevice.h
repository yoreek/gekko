#pragma once

#include "devices/display/hd44780/Hd44780PinLeafDeviceBase.h"
#include "devices/display/lcd2004_pin/Lcd2004PinDeviceConfig.h"

#include <memory>

namespace ewfm {

// 20x4 leaf of Hd44780PinCharacterDisplayDeviceBase -- direct-GPIO sibling of Lcd2004Device (I2C).
// Supplies only geometry (columns=20, rows=4) and its own pin config.
class Lcd2004PinDevice final : public Hd44780PinLeafDeviceBase<Lcd2004PinDevice, Lcd2004PinDeviceConfigV1, decodeLcd2004PinDeviceConfig,
                                                               lcd2004PinDeviceConfigSize> {
public:
    Lcd2004PinDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Lcd2004PinDevice(const Lcd2004PinDeviceConfigV1& config);
    Lcd2004PinDevice(const Lcd2004PinDeviceConfigV1& config, IHd44780PinLineDriver& lineDriver);

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
};

} // namespace ewfm
