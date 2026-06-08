#pragma once

#include "devices/api/DeviceApiAdapter.h"

namespace ewfm {

class DummyDeviceApiAdapter final : public IDeviceApiAdapter {
public:
    static const DummyDeviceApiAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, std::string& error) const override;
    void writeDeviceJson(const DeviceRecord& record, JsonObject output) const override;
};

} // namespace ewfm
