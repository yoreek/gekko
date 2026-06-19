#pragma once

#include "integrations/common/DeviceApiAdapter.h"

namespace ewfm {

class GpioSwitchDeviceApiAdapter final : public IDeviceApiAdapter {
public:
    static const GpioSwitchDeviceApiAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, std::string& error) const override;
    bool parseUpdateConfigRequest(const JsonObjectConst& input, const DeviceRecord& record, DeviceConfigUpdateRequest& request,
                                  std::string& error) const override;
    void writeDeviceJson(const DeviceRecord& record, const IDeviceRuntime* runtime, JsonObject output) const override;
};

} // namespace ewfm
