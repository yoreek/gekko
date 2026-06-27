#pragma once

#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/ssd1306/Ssd1306DeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class Ssd1306Device final : public DisplayDeviceBase {
public:
    Ssd1306Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

    const Ssd1306DeviceConfigV1& config() const;
    bool enabled() const override;
    const char* name() const override;
    bool i2cAddress(uint8_t& address) const override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    void writeDisplayConfigJson(JsonObject output) const override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    Ssd1306DeviceConfigV1 config_{};
};

} // namespace ewfm
