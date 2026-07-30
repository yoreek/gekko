#include "integrations/rest/lcd2004/Lcd2004DeviceApiAdapter.h"

#include "integrations/rest/common/I2cDeviceApiSupport.h"

namespace ewfm {

const Lcd2004DeviceApiAdapter& Lcd2004DeviceApiAdapter::instance() {
    static const Lcd2004DeviceApiAdapter adapter;
    return adapter;
}

bool Lcd2004DeviceApiAdapter::decodeConfig(const uint8_t* input, const size_t size, Lcd2004DeviceConfigV2& config) {
    return decodeLcd2004DeviceConfig(input, size, config);
}

DeviceRole Lcd2004DeviceApiAdapter::busRole() {
    return DeviceRole::I2CBus;
}

DeviceId Lcd2004DeviceApiAdapter::configBusDeviceId(const Lcd2004DeviceConfigV2& config) {
    (void)config;
    return 0;
}

void Lcd2004DeviceApiAdapter::setConfigBusDeviceId(Lcd2004DeviceConfigV2& config, const DeviceId busDeviceId) {
    (void)config;
    (void)busDeviceId;
}

DeviceValidationResult Lcd2004DeviceApiAdapter::validateBusDependency(const DeviceRegistry& registry, const DeviceId busDeviceId,
                                                                      const Lcd2004DeviceConfigV2& config,
                                                                      const IDeviceRuntime* ignoreDependent) {
    return validateI2cBusDependency(registry, busDeviceId, config.i2cAddress, ignoreDependent);
}

} // namespace ewfm
