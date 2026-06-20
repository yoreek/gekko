#pragma once

#include "integrations/common/DeviceApiAdapter.h"

namespace ewfm {

class DummyDeviceApiAdapter final : public IDeviceApiAdapter {
public:
    static const DummyDeviceApiAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const override;
    void writeDeviceJson(const IDeviceRuntime& runtime, JsonObject output) const override;
};

} // namespace ewfm
