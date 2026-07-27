#pragma once

#include "devices/display/hd44780/Hd44780LeafDeviceBase.h"
#include "devices/display/lcd2004/Lcd2004DeviceConfig.h"

#include <memory>

namespace ewfm {

// 20x4 leaf of Hd44780CharacterDisplayDeviceBase -- see that header for the shared protocol,
// lifecycle, and rendering (including the 4-row DDRAM addressing quirk). Supplies only geometry
// (columns=20, rows=4) and its own four-line config.
class Lcd2004Device final
    : public Hd44780LeafDeviceBase<Lcd2004Device, Lcd2004DeviceConfigV2, decodeLcd2004DeviceConfig, lcd2004DeviceConfigSize> {
public:
    Lcd2004Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Lcd2004Device(const Lcd2004DeviceConfigV2& config);

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
};

} // namespace ewfm
