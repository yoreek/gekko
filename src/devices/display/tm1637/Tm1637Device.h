#pragma once

#include "devices/core/DeviceDependencyValidation.h"
#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/render/SegmentDisplayLayoutRenderer.h"
#include "devices/display/tm1637/Tm1637DeviceConfig.h"
#include "devices/display/tm1637/Tm1637LineDriver.h"
#include "devices/display/tm1637/Tm1637Protocol.h"
#include "devices/display/tm1637/Tm1637SegmentSurface.h"

#include <array>
#include <memory>

namespace ewfm {

class Tm1637Device final : public DisplayDeviceBase {
public:
    Tm1637Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Tm1637Device(const Tm1637DeviceConfigV1& config);
    ~Tm1637Device() override;

    const Tm1637DeviceConfigV1& config() const;
    DisplayLayoutProfile displayProfile() const override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    void writeDisplayConfigJson(JsonObject output) const override;
    bool initializeDisplayHardware(uint32_t now) override;
    void releaseDisplayHardware(uint32_t now) override;
    bool clearDisplay(uint16_t color) override;
    DisplayLayoutRenderResult renderDisplayFrame(const MetricValueResolver& resolver, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;
    void setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) override;
    void setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) override;
    void refreshLineCache();
    bool dependenciesReady() const;
    bool writeDisplayFrame(uint32_t now);
    uint8_t digitCount() const;

    Tm1637DeviceConfigV1 config_{};
    Tm1637SegmentSurface surface_;
    ISwitchOutputRuntime* clockLine_{nullptr};
    ISwitchOutputRuntime* dataLine_{nullptr};
    std::array<uint8_t, Tm1637SegmentCodec::kDigitCount> lastFrameBytes_{};
    bool lastFrameValid_{false};
};

} // namespace ewfm
