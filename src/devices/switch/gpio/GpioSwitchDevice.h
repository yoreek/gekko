#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/switch/TriStateSwitchDeviceBase.h"
#include "devices/switch/gpio/IGpioOutputDriver.h"

#include <ArduinoJson.h>
#include <string>

namespace ewfm {

#pragma pack(push, 1)
struct GpioSwitchDeviceConfigV1 {
    static constexpr uint32_t kMagicKey = 0x47535731UL;
    uint8_t enabled{1};
    uint8_t restorePreviousState{0};
    uint8_t startupState{static_cast<uint8_t>(OutputState::Off)};
    uint8_t safeState{static_cast<uint8_t>(OutputState::Off)};
    uint8_t inverted{0};
    uint8_t gpioPin{2};
};
#pragma pack(pop)

std::string encodeGpioSwitchDeviceConfig(const GpioSwitchDeviceConfigV1& config);
bool decodeGpioSwitchDeviceConfig(const std::string& blob, GpioSwitchDeviceConfigV1& config);
bool parseGpioSwitchDeviceConfigJson(const JsonObjectConst& input, GpioSwitchDeviceConfigV1& config, std::string& error);
void writeGpioSwitchDeviceConfigJson(const GpioSwitchDeviceConfigV1& config, JsonObject output);
SwitchDeviceConfigV1 toSwitchDeviceConfig(const GpioSwitchDeviceConfigV1& config);
bool gpioSwitchPinIsValid(uint8_t pin);

class GpioSwitchDevice final : public TriStateSwitchDeviceBase {
public:
    explicit GpioSwitchDevice(const DeviceRecord& record);
    GpioSwitchDevice(const GpioSwitchDeviceConfigV1& config, IGpioOutputDriver& driver);

    uint8_t gpioPin() const;
    const GpioSwitchDeviceConfigV1& gpioConfig() const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRecord& record);
    static DeviceValidationResult validateConfig(const DeviceRecord& record);

private:
    DeviceValidationResult configureHardware(uint32_t now) override;
    DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) override;
    void releaseHardware(uint32_t now) override;

    GpioSwitchDeviceConfigV1 config_{};
    IGpioOutputDriver& driver_;
};

} // namespace ewfm
