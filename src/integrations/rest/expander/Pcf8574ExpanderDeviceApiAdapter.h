#pragma once

#include "devices/expander/Pcf8574ExpanderDevice.h"
#include "integrations/rest/expander/Pcf857xExpanderApiAdapterBase.h"

namespace ewfm {

class Pcf8574ExpanderDeviceApiAdapter final : public Pcf857xExpanderApiAdapterBase<Pcf8574ExpanderDeviceApiAdapter, Pcf8574ExpanderDevice> {
public:
    static constexpr const char* kTypeName = "pcf8574_expander";
};

} // namespace ewfm
