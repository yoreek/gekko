#pragma once

#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/st7735/St7735DeviceConfig.h"

#include <ArduinoJson.h>
#include <memory>

class Adafruit_ST7735;

namespace ewfm {

class St7735CanvasSurface;

class St7735Device final : public DisplayDeviceBase {
public:
    St7735Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    ~St7735Device() override;

    const St7735DeviceConfigV5& config() const;
    bool spiChipSelectPin(uint8_t& pin) const override;
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

    St7735DeviceConfigV5 config_{};
    Adafruit_ST7735* display_{nullptr};
    St7735CanvasSurface* surface_{nullptr};
};

} // namespace ewfm
