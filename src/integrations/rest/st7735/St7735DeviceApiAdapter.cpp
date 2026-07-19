#include "integrations/rest/st7735/St7735DeviceApiAdapter.h"

#include "devices/bus/spi/SpiBusDevice.h"

namespace ewfm {

const St7735DeviceApiAdapter& St7735DeviceApiAdapter::instance() {
    static const St7735DeviceApiAdapter adapter;
    return adapter;
}

bool St7735DeviceApiAdapter::decodeConfig(const uint8_t* input, const size_t size, St7735DeviceConfigV4& config) {
    return decodeSt7735DeviceConfig(input, size, config);
}

DeviceRole St7735DeviceApiAdapter::busRole() {
    return DeviceRole::SpiBus;
}

DeviceId St7735DeviceApiAdapter::configBusDeviceId(const St7735DeviceConfigV4& config) {
    return config.spiBusDeviceId;
}

void St7735DeviceApiAdapter::setConfigBusDeviceId(St7735DeviceConfigV4& config, const DeviceId busDeviceId) {
    config.spiBusDeviceId = busDeviceId;
}

DeviceValidationResult St7735DeviceApiAdapter::validateBusDependency(const DeviceRegistry& registry, const DeviceId busDeviceId,
                                                                     const St7735DeviceConfigV4& config,
                                                                     const IDeviceRuntime* ignoreDependent) {
    const IDeviceRuntime* busRuntime = registry.runtime(busDeviceId);
    if (busRuntime == nullptr || busRuntime->typeId() != SpiBusDevice::descriptor().typeId) {
        return {DeviceError::InvalidRelationship, "st7735 spi bus dependency is missing or invalid"};
    }
    if (busRuntime->hasDuplicateDependentSpiChipSelect(config.chipSelectPin, ignoreDependent)) {
        return {DeviceError::InvalidRelationship, "duplicate st7735 chip select pin on dependency"};
    }
    return {};
}

} // namespace ewfm
