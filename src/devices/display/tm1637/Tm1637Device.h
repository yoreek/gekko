#pragma once

#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/render/SegmentDisplayLayoutRenderer.h"
#include "devices/display/tm1637/Tm1637DeviceConfig.h"
#include "devices/display/tm1637/Tm1637LineDriver.h"
#include "devices/display/tm1637/Tm1637Protocol.h"
#include "devices/display/tm1637/Tm1637SegmentSurface.h"

#include <array>
#include <memory>

namespace ewfm {

// TM1637 owns its CLK/DIO pins outright (like Ds1302RtcDevice) rather than borrowing them from
// switch devices: the protocol needs the DIO direction switched mid-byte to read the ACK bit, which
// a switch runtime cannot express.
class Tm1637Device final : public DisplayDeviceBase {
public:
    Tm1637Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    Tm1637Device(const Tm1637DeviceConfigV2& config, ITm1637LineDriver& lineDriver);
    ~Tm1637Device() override;

    const Tm1637DeviceConfigV2& config() const;
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
    void claimGpioPins(DeviceId* pins) const override;
    void releaseGpioPins(DeviceId* pins) const override;

private:
    const DeviceBaseConfigV1& baseConfig() const override;
    bool writeDisplayFrame();
    uint8_t digitCount() const;

    Tm1637DeviceConfigV2 config_{};
    ITm1637LineDriver& lines_;
    Tm1637SegmentSurface surface_;
    std::array<uint8_t, Tm1637SegmentCodec::kDigitCount> lastFrameBytes_{};
    bool lastFrameValid_{false};
    bool pinsAcquired_{false};
};

} // namespace ewfm
