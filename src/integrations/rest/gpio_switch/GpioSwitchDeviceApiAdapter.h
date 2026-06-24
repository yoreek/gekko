#pragma once

#include "integrations/common/DeviceApiAdapter.h"

namespace ewfm {

class GpioSwitchDeviceApiAdapter final : public IDeviceApiAdapter {
public:
    static const GpioSwitchDeviceApiAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const override;
    bool parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const override;
    void writeDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, JsonObject output) const override;
};

} // namespace ewfm
