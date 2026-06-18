#pragma once

#include "integrations/common/DeviceApiAdapter.h"

namespace ewfm {

class OneWireBusDeviceApiAdapter final : public IDeviceApiAdapter {
public:
    static const OneWireBusDeviceApiAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, std::string& error) const override;
    void writeDeviceJson(const DeviceRecord& record, const IDeviceRuntime* runtime, JsonObject output) const override;
};

} // namespace ewfm
