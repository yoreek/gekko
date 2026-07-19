#pragma once

#include "devices/dummy/DummyDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class DummyDeviceApiAdapter final : public TypedDeviceApiAdapter<DummyDeviceApiAdapter, DummyDevice, DummyDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "dummy";
};

} // namespace ewfm
