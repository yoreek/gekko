#pragma once

#include "devices/display/DisplayLayoutProfile.h"

namespace ewfm {

DeviceValidationResult validateDisplayLayoutWidget(const DisplayLayoutWidgetV1& widget, const DisplayLayoutProfile& profile);
DeviceValidationResult validateDisplayLayout(const DisplayLayoutRecordV1& layout, const DisplayLayoutProfile& profile);

} // namespace ewfm
