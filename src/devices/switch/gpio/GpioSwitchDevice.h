#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/switch/SwitchDeviceConfig.h"
#include "devices/switch/TriStateSwitchDeviceBase.h"
#include "devices/switch/gpio/IGpioOutputDriver.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct GpioSwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "GSW-CHILD-1";
    uint8_t gpioPin{2};
};

struct GpioSwitchDevicePersistedConfigV1 {
    SwitchDeviceConfigV1 switchConfig{};
    GpioSwitchDeviceConfigV1 gpioConfig{};
};
#pragma pack(pop)

constexpr size_t gpioSwitchDeviceConfigSize(const GpioSwitchDevicePersistedConfigV1&) {
    return sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1) + sizeof(GpioSwitchDeviceConfigV1::kMagic) - 1U +
           sizeof(GpioSwitchDeviceConfigV1);
}

bool encodeGpioSwitchDeviceConfig(const GpioSwitchDevicePersistedConfigV1& config, uint8_t* blob, size_t capacity);
bool decodeGpioSwitchDeviceConfig(const uint8_t* blob, size_t size, GpioSwitchDevicePersistedConfigV1& config);
bool parseGpioSwitchDeviceConfigJson(const JsonObjectConst& input, GpioSwitchDevicePersistedConfigV1& config, const char*& error);
void writeGpioSwitchDeviceConfigJson(const GpioSwitchDevicePersistedConfigV1& config, JsonObject output);
bool gpioSwitchPinIsValid(uint8_t pin);

class GpioSwitchDevice final : public TriStateSwitchDeviceBase {
public:
    GpioSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    GpioSwitchDevice(const GpioSwitchDevicePersistedConfigV1& config, IGpioOutputDriver& driver);

    uint8_t gpioPin() const;
    const GpioSwitchDeviceConfigV1& gpioConfig() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override;
    void writeDeviceJson(JsonObject output) const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    DeviceValidationResult configureHardware(uint32_t now) override;
    DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) override;
    void releaseHardware(uint32_t now) override;

    GpioSwitchDeviceConfigV1 config_{};
    IGpioOutputDriver& driver_;
};

} // namespace ewfm
