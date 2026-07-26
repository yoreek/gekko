#pragma once

#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"
#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/ssd1306/Ssd1306DeviceConfig.h"

#include <ArduinoJson.h>
#include <memory>

class Adafruit_SSD1306;

namespace ewfm {

class Ssd1306CanvasSurface;

class Ssd1306Device;
using Ssd1306DeviceBase = I2cDeviceRuntimeBase<Ssd1306Device, DisplayDeviceBase>;

class Ssd1306Device final : public Ssd1306DeviceBase {
public:
    Ssd1306Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    ~Ssd1306Device() override;

    const Ssd1306DeviceConfigV6& config() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
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
    const DeviceBaseConfigV1& baseConfig() const override;

    Ssd1306DeviceConfigV6 config_{};
#if defined(ARDUINO) && !defined(UNIT_TEST)
    std::unique_ptr<::Adafruit_SSD1306> display_{};
    std::unique_ptr<Ssd1306CanvasSurface> surface_{};
#endif
};

} // namespace ewfm
