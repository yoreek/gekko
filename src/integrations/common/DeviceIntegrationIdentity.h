#pragma once

#include "devices/core/DeviceTypes.h"

#include <string>

namespace ewfm {

std::string makeExternalDeviceId(const std::string& controllerIdentity, DeviceId deviceId);

} // namespace ewfm
