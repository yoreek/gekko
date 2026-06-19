#pragma once

#include "devices/bus/onewire/OneWireRomAddress.h"

#include <memory>

namespace ewfm {

IOneWireBusDriver& defaultArduinoOneWireBusDriver();
std::unique_ptr<IOneWireBusDriver> createArduinoOneWireBusDriver();

} // namespace ewfm
