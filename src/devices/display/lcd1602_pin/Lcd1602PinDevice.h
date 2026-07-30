#pragma once

#include "devices/display/hd44780/Hd44780PinLeafDeviceBase.h"
#include "devices/display/lcd1602_pin/Lcd1602PinDeviceConfig.h"

#include <memory>

namespace ewfm {

// 16x2 leaf of Hd44780PinCharacterDisplayDeviceBase -- direct-GPIO sibling of Lcd1602Device (I2C).
// Supplies only geometry (columns=16, rows=2) and its own pin config.
class Lcd1602PinDevice final : public Hd44780PinLeafDeviceBase<Lcd1602PinDevice, Lcd1602PinDeviceConfigV1, decodeLcd1602PinDeviceConfig,
                                                               lcd1602PinDeviceConfigSize> {
public:
    Lcd1602PinDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Lcd1602PinDevice(const Lcd1602PinDeviceConfigV1& config);
    Lcd1602PinDevice(const Lcd1602PinDeviceConfigV1& config, IHd44780PinLineDriver& lineDriver);

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
};

} // namespace ewfm
