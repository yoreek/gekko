#include "devices/display/lcd1602/Lcd1602Device.h"

namespace ewfm {

namespace {
constexpr DeviceTypeId kLcd1602DeviceTypeId = 28;
constexpr uint32_t kLcd1602DeviceConfigVersion = 2;
constexpr uint8_t kLcd1602Columns = 16U;
constexpr uint8_t kLcd1602Rows = 2U;
} // namespace

Lcd1602Device::Lcd1602Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Hd44780LeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd1602Columns, kLcd1602Rows, record, configBlob) {}

Lcd1602Device::Lcd1602Device(const Lcd1602DeviceConfigV2& config)
    : Hd44780LeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd1602Columns, kLcd1602Rows, config) {}

DeviceTypeDescriptor Lcd1602Device::descriptor() {
    DeviceTypeDescriptor descriptor = hd44780LeafDeviceDescriptor(kLcd1602DeviceTypeId, "Lcd1602Device", kLcd1602DeviceConfigVersion);
    descriptor.createRuntime = &Lcd1602Device::createRuntime;
    descriptor.validateConfig = &Lcd1602Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Lcd1602Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Lcd1602Device(record, configBlob));
}

DeviceValidationResult Lcd1602Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return validateHd44780LeafDeviceConfig<Lcd1602DeviceConfigV2, decodeLcd1602DeviceConfig>(record, configBlob,
                                                                                             "lcd1602 config is invalid");
}

} // namespace ewfm
