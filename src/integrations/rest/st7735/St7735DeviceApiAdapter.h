#pragma once

#include "devices/display/st7735/St7735Device.h"
#include "integrations/rest/display/TypedDisplayDeviceApiAdapter.h"

namespace ewfm {

class St7735DeviceApiAdapter final : public TypedDisplayDeviceApiAdapter<St7735DeviceApiAdapter, St7735Device, St7735DeviceConfigV6> {
public:
    static constexpr const char* kTypeName = "st7735";
    static constexpr const char* kBusDependencyError = "st7735 requires spi bus dependency";

    static const St7735DeviceApiAdapter& instance();
    static bool decodeConfig(const uint8_t* input, size_t size, St7735DeviceConfigV6& config);
    static DeviceRole busRole();
    static DeviceId configBusDeviceId(const St7735DeviceConfigV6& config);
    static void setConfigBusDeviceId(St7735DeviceConfigV6& config, DeviceId busDeviceId);
    static DeviceValidationResult validateBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId,
                                                        const St7735DeviceConfigV6& config, const IDeviceRuntime* ignoreDependent);
};

} // namespace ewfm
