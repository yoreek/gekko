#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/switch/SwitchDeviceBase.h"
#include "devices/switch/SwitchDeviceConfig.h"
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

struct GpioSwitchDeviceConfigV2 : SwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "GSW2";
    uint8_t gpioPin{2};
};

struct GpioSwitchDeviceConfigV3 : SwitchDeviceConfigV2 {
    static constexpr char kMagic[] = "GSW3";
    uint8_t gpioPin{2};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    void migrateFrom(const GpioSwitchDeviceConfigV2& legacy);
};
#pragma pack(pop)

constexpr size_t gpioSwitchDeviceConfigSize(const GpioSwitchDevicePersistedConfigV1&) {
    return sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1) + sizeof(GpioSwitchDeviceConfigV1::kMagic) - 1U +
           sizeof(GpioSwitchDeviceConfigV1);
}

constexpr size_t gpioSwitchDeviceConfigSize(const GpioSwitchDeviceConfigV2&) {
    return sizeof(GpioSwitchDeviceConfigV2::kMagic) - 1U + sizeof(GpioSwitchDeviceConfigV2);
}

constexpr size_t gpioSwitchDeviceConfigSize(const GpioSwitchDeviceConfigV3&) {
    return sizeof(GpioSwitchDeviceConfigV3::kMagic) - 1U + sizeof(GpioSwitchDeviceConfigV3);
}

bool decodeGpioSwitchDeviceConfig(const uint8_t* blob, size_t size, GpioSwitchDeviceConfigV3& config);
bool gpioSwitchPinIsValid(uint8_t pin);

class GpioSwitchDevice final : public SwitchDeviceBase {
public:
    GpioSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    GpioSwitchDevice(const GpioSwitchDeviceConfigV3& config, IGpioOutputDriver& driver);

    uint8_t gpioPin() const;
    const GpioSwitchDeviceConfigV3& config() const override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    DeviceValidationResult configureHardware(uint32_t now) override;
    DeviceValidationResult applyHardwareOutput(bool state, uint32_t now) override;
    void releaseHardware(uint32_t now) override;
    SwitchDeviceConfigV2& mutableConfig() override;

    GpioSwitchDeviceConfigV3 config_{};
    IGpioOutputDriver& driver_;
};

} // namespace ewfm
