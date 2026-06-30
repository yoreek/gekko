#pragma once

#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/ssd1306/Ssd1306DeviceConfig.h"

#include <ArduinoJson.h>
#include <memory>

class Adafruit_SSD1306;

namespace ewfm {

class Ssd1306CanvasSurface;

class Ssd1306Device final : public DisplayDeviceBase {
public:
    Ssd1306Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    ~Ssd1306Device() override;

    const Ssd1306DeviceConfigV3& config() const;
    bool enabled() const override;
    const char* name() const override;
    bool i2cAddress(uint8_t& address) const override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    void writeDisplayConfigJson(JsonObject output) const override;
    bool initializeDisplayHardware(uint32_t now) override;
    void releaseDisplayHardware(uint32_t now) override;
    IDisplayRenderSurface* renderSurface() const override;
    void onDisplayFrameRendered(const DisplayLayoutRenderResult& result) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    Ssd1306DeviceConfigV3 config_{};
#if defined(ARDUINO) && !defined(UNIT_TEST)
    std::unique_ptr<::Adafruit_SSD1306> display_{};
    std::unique_ptr<Ssd1306CanvasSurface> surface_{};
#endif
};

} // namespace ewfm
