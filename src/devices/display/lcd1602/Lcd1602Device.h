#pragma once

#include "devices/display/hd44780/Hd44780LeafDeviceBase.h"
#include "devices/display/lcd1602/Lcd1602DeviceConfig.h"

#include <memory>

namespace ewfm {

// 16x2 leaf of Hd44780CharacterDisplayDeviceBase -- see that header for the shared protocol,
// lifecycle, and rendering. Supplies only geometry (columns=16, rows=2) and its own two-line
// config.
class Lcd1602Device final
    : public Hd44780LeafDeviceBase<Lcd1602Device, Lcd1602DeviceConfigV2, decodeLcd1602DeviceConfig, lcd1602DeviceConfigSize> {
public:
    Lcd1602Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Lcd1602Device(const Lcd1602DeviceConfigV2& config);

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
};

} // namespace ewfm
