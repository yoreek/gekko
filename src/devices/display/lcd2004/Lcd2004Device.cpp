#include "devices/display/lcd2004/Lcd2004Device.h"

namespace ewfm {

namespace {
constexpr DeviceTypeId kLcd2004DeviceTypeId = 30; // 29 is taken by kAht10SensorTypeId.
constexpr uint32_t kLcd2004DeviceConfigVersion = 2;
constexpr uint8_t kLcd2004Columns = 20U;
constexpr uint8_t kLcd2004Rows = 4U;
} // namespace

Lcd2004Device::Lcd2004Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Hd44780LeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd2004Columns, kLcd2004Rows, record, configBlob) {}

Lcd2004Device::Lcd2004Device(const Lcd2004DeviceConfigV2& config)
    : Hd44780LeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd2004Columns, kLcd2004Rows, config) {}

DeviceTypeDescriptor Lcd2004Device::descriptor() {
    DeviceTypeDescriptor descriptor = hd44780LeafDeviceDescriptor(kLcd2004DeviceTypeId, "Lcd2004Device", kLcd2004DeviceConfigVersion);
    descriptor.createRuntime = &Lcd2004Device::createRuntime;
    descriptor.validateConfig = &Lcd2004Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Lcd2004Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Lcd2004Device(record, configBlob));
}

DeviceValidationResult Lcd2004Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return validateI2cConfig<Lcd2004DeviceConfigV2>(record, configBlob, decodeLcd2004DeviceConfig);
}

} // namespace ewfm
