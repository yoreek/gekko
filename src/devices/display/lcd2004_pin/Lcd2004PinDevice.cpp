#include "devices/display/lcd2004_pin/Lcd2004PinDevice.h"

namespace ewfm {

namespace {
constexpr DeviceTypeId kLcd2004PinDeviceTypeId = 35;
constexpr uint32_t kLcd2004PinDeviceConfigVersion = 1;
constexpr uint8_t kLcd2004PinColumns = 20U;
constexpr uint8_t kLcd2004PinRows = 4U;
} // namespace

Lcd2004PinDevice::Lcd2004PinDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Hd44780PinLeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd2004PinColumns, kLcd2004PinRows, record, configBlob) {
}

Lcd2004PinDevice::Lcd2004PinDevice(const Lcd2004PinDeviceConfigV1& config)
    : Hd44780PinLeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd2004PinColumns, kLcd2004PinRows, config) {}

Lcd2004PinDevice::Lcd2004PinDevice(const Lcd2004PinDeviceConfigV1& config, IHd44780PinLineDriver& lineDriver)
    : Hd44780PinLeafDeviceBase(Hd44780CharacterDisplayDeviceBase::initialState(), kLcd2004PinColumns, kLcd2004PinRows, config, lineDriver) {
}

DeviceTypeDescriptor Lcd2004PinDevice::descriptor() {
    DeviceTypeDescriptor descriptor =
        hd44780PinLeafDeviceDescriptor(kLcd2004PinDeviceTypeId, "Lcd2004PinDevice", kLcd2004PinDeviceConfigVersion);
    descriptor.createRuntime = &Lcd2004PinDevice::createRuntime;
    descriptor.validateConfig = &Lcd2004PinDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Lcd2004PinDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Lcd2004PinDevice(record, configBlob));
}

DeviceValidationResult Lcd2004PinDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    for (uint8_t index = 0U; index < record.depCount; ++index) {
        if (record.dependencyLinks() == nullptr || record.dependencyLinks()[index].role != DeviceRole::MetricSource) {
            return {DeviceError::InvalidRelationship, "display metric dependency is invalid"};
        }
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "device config exceeds supported size"};
    }
    Lcd2004PinDeviceConfigV1 config{};
    if (!decodeLcd2004PinDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "device config is invalid"};
    }
    return config.validate();
}

} // namespace ewfm
