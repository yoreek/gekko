#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/pixel/PixelStripDeviceConfig.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Adafruit_NeoPixel.h>
#endif

namespace ewfm {

// Raw 0..255 brightness, retained across reboot the same way AbstractOutputDevice's
// OutputDeviceRetainedStateRecord<ValueType> persists a live output value -- separate from the
// versioned config blob, so a SetOutput brightness change never bumps configRevision.
struct PixelStripRetainedStateV1 {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0};
    uint8_t brightness{0U};
    // Explicit on/off gate, independent of brightness -- see PixelStripDevice::applyLiveBrightness().
    // Not derived from "brightness == 0" (that comparison is what HA's on/off toggle used to be
    // computed from, and it caused the brightness slider to silently flip an unrelated HA toggle).
    bool on{true};
};

// The hardware backend for one physical WS2812B (and compatible) addressable strip: owns the pin,
// a bounded pixel-color buffer, and (on real hardware) the Adafruit_NeoPixel driver. Deliberately
// passive -- it never animates on its own tick, only handles lifecycle (mirrors I2cBusDevice/
// OneWireBusDevice/SpiBusDevice, all passive hardware owners that still tick at 100ms for the same
// reason). Frame cadence belongs to whichever effect decorator (e.g. PixelEffectSolidDevice,
// PixelEffectAlertDevice) is attached via the exclusive PixelStrip dependency, exactly as
// FadeAnalogOutputDevice owns its own step cadence over the passive LedcAnalogOutputDevice.
class PixelStripDevice final : public DeviceRuntimeBase, public IPixelStripRuntime {
public:
    PixelStripDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit PixelStripDevice(const PixelStripDeviceConfigV1& config);

    const PixelStripDeviceConfigV1& config() const;

    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    bool handleCommand(const DeviceCommand& command) override;
    bool retainedStateDirty() const override;
    void clearRetainedStateDirty() override;
    DeviceValidationResult saveRetainedState(DeviceRetainedDataStore& store) const override;
    DeviceValidationResult loadRetainedState(DeviceRetainedDataStore& store) override;

    const IPixelStripRuntime* pixelStripRuntime() const override {
        return this;
    }
    uint16_t pixelCount() const override;
    bool setPixel(uint16_t index, PixelColor color) override;
    bool fill(PixelColor color) override;
    bool show(uint32_t now) override;
    PixelColor currentPixel(uint16_t index) const override;
    // Live, currently-applied brightness (0..255 raw) -- runtime state, not config. See
    // PixelStripRetainedStateV1.
    uint8_t liveBrightness() const;
    // Explicit on/off gate -- runtime state, not config, not derived from liveBrightness(). See
    // PixelStripRetainedStateV1 and applyLiveBrightness().
    bool liveOn() const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    void claimGpioPins(DeviceId* pins) const override;
    void releaseGpioPins(DeviceId* pins) const override;

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Reconfiguring();
    State Ready();
    State Disabled();
    State Faulted();
    State Deleting();

    DeviceValidationResult initializeHardware(uint32_t now);
    void releaseHardware();
    void applyLiveBrightness();
    bool parseSetOutputBrightness(const DeviceCommand& command, uint8_t& percent) const;
    bool parseSetOutputOn(const DeviceCommand& command, bool& on) const;

    PixelStripDeviceConfigV1 config_{};
    PixelColor buffer_[kMaxPixelStripLength]{};
#if defined(ARDUINO) && !defined(UNIT_TEST)
    Adafruit_NeoPixel strip_{};
#endif
    bool hardwareReady_{false};

    uint8_t liveBrightness_{0U};
    uint8_t retainedBrightness_{0U};
    bool retainedBrightnessAvailable_{false};
    bool liveOn_{true};
    bool retainedOn_{true};
    bool retainedOnAvailable_{false};
    bool retainedStateDirty_{false};
};

} // namespace ewfm
