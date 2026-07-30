#include "integrations/rest/lcd1602/Lcd1602DeviceApiAdapter.h"

#include "integrations/rest/common/I2cDeviceApiSupport.h"

namespace ewfm {

const Lcd1602DeviceApiAdapter& Lcd1602DeviceApiAdapter::instance() {
    static const Lcd1602DeviceApiAdapter adapter;
    return adapter;
}

bool Lcd1602DeviceApiAdapter::decodeConfig(const uint8_t* input, const size_t size, Lcd1602DeviceConfigV2& config) {
    return decodeLcd1602DeviceConfig(input, size, config);
}

DeviceRole Lcd1602DeviceApiAdapter::busRole() {
    return DeviceRole::I2CBus;
}

DeviceId Lcd1602DeviceApiAdapter::configBusDeviceId(const Lcd1602DeviceConfigV2& config) {
    (void)config;
    return 0;
}

void Lcd1602DeviceApiAdapter::setConfigBusDeviceId(Lcd1602DeviceConfigV2& config, const DeviceId busDeviceId) {
    (void)config;
    (void)busDeviceId;
}

DeviceValidationResult Lcd1602DeviceApiAdapter::validateBusDependency(const DeviceRegistry& registry, const DeviceId busDeviceId,
                                                                      const Lcd1602DeviceConfigV2& config,
                                                                      const IDeviceRuntime* ignoreDependent) {
    return validateI2cBusDependency(registry, busDeviceId, config.i2cAddress, ignoreDependent);
}

} // namespace ewfm
