#include "devices/display/lcd1602_pin/Lcd1602PinDevice.h"

namespace ewfm {

namespace {
constexpr DeviceTypeId kLcd1602PinDeviceTypeId = 34;
constexpr uint32_t kLcd1602PinDeviceConfigVersion = 1;
constexpr uint8_t kLcd1602PinColumns = 16U;
constexpr uint8_t kLcd1602PinRows = 2U;
} // namespace

Lcd1602PinDevice::Lcd1602PinDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Hd44780PinLeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd1602PinColumns, kLcd1602PinRows, record, configBlob) {
}

Lcd1602PinDevice::Lcd1602PinDevice(const Lcd1602PinDeviceConfigV1& config)
    : Hd44780PinLeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd1602PinColumns, kLcd1602PinRows, config) {}

Lcd1602PinDevice::Lcd1602PinDevice(const Lcd1602PinDeviceConfigV1& config, IHd44780PinLineDriver& lineDriver)
    : Hd44780PinLeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd1602PinColumns, kLcd1602PinRows, config, lineDriver) {
}

DeviceTypeDescriptor Lcd1602PinDevice::descriptor() {
    DeviceTypeDescriptor descriptor =
        hd44780PinLeafDeviceDescriptor(kLcd1602PinDeviceTypeId, "Lcd1602PinDevice", kLcd1602PinDeviceConfigVersion);
    descriptor.createRuntime = &Lcd1602PinDevice::createRuntime;
    descriptor.validateConfig = &Lcd1602PinDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Lcd1602PinDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Lcd1602PinDevice(record, configBlob));
}

DeviceValidationResult Lcd1602PinDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    for (uint8_t index = 0U; index < record.depCount; ++index) {
        if (record.dependencyLinks() == nullptr || record.dependencyLinks()[index].role != DeviceRole::MetricSource) {
            return {DeviceError::InvalidRelationship, "display metric dependency is invalid"};
        }
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "device config exceeds supported size"};
    }
    Lcd1602PinDeviceConfigV1 config{};
    if (!decodeLcd1602PinDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "device config is invalid"};
    }
    return config.validate();
}

} // namespace ewfm
