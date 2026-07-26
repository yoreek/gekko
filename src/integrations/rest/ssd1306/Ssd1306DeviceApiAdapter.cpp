#include "integrations/rest/ssd1306/Ssd1306DeviceApiAdapter.h"

#include "integrations/rest/common/I2cDeviceApiSupport.h"

namespace ewfm {

const Ssd1306DeviceApiAdapter& Ssd1306DeviceApiAdapter::instance() {
    static const Ssd1306DeviceApiAdapter adapter;
    return adapter;
}

bool Ssd1306DeviceApiAdapter::decodeConfig(const uint8_t* input, const size_t size, Ssd1306DeviceConfigV6& config) {
    return decodeSsd1306DeviceConfig(input, size, config);
}

DeviceRole Ssd1306DeviceApiAdapter::busRole() {
    return DeviceRole::I2CBus;
}

DeviceId Ssd1306DeviceApiAdapter::configBusDeviceId(const Ssd1306DeviceConfigV6& config) {
    (void)config;
    return 0;
}

void Ssd1306DeviceApiAdapter::setConfigBusDeviceId(Ssd1306DeviceConfigV6& config, const DeviceId busDeviceId) {
    (void)config;
    (void)busDeviceId;
}

DeviceValidationResult Ssd1306DeviceApiAdapter::validateBusDependency(const DeviceRegistry& registry, const DeviceId busDeviceId,
                                                                      const Ssd1306DeviceConfigV6& config,
                                                                      const IDeviceRuntime* ignoreDependent) {
    return validateI2cBusDependency(registry, busDeviceId, config.i2cAddress, ignoreDependent);
}

} // namespace ewfm
