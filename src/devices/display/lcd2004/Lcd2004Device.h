#pragma once

#include "devices/display/hd44780/Hd44780CharacterDisplayDeviceBase.h"
#include "devices/display/lcd2004/Lcd2004DeviceConfig.h"

#include <memory>

namespace ewfm {

// 20x4 leaf of Hd44780CharacterDisplayDeviceBase -- see that header for the shared protocol,
// lifecycle, and rendering (including the 4-row DDRAM addressing quirk). Supplies only geometry
// (columns=20, rows=4) and its own four-line config.
class Lcd2004Device final : public Hd44780CharacterDisplayDeviceBase {
public:
    Lcd2004Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Lcd2004Device(const Lcd2004DeviceConfigV1& config);

    const Lcd2004DeviceConfigV1& config() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;
    const Hd44780ChannelConfigV1& channelConfig() const override;
    const char* lineTemplate(uint8_t row) const override;

    Lcd2004DeviceConfigV1 config_{};
};

} // namespace ewfm
