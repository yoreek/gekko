#pragma once

#include "devices/expander/Pcf8575ExpanderDevice.h"
#include "integrations/rest/expander/Pcf857xExpanderApiAdapterBase.h"

namespace ewfm {

class Pcf8575ExpanderDeviceApiAdapter final : public Pcf857xExpanderApiAdapterBase<Pcf8575ExpanderDeviceApiAdapter, Pcf8575ExpanderDevice> {
public:
    static constexpr const char* kTypeName = "pcf8575_expander";
};

} // namespace ewfm
