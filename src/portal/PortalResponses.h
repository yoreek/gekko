#pragma once

#include "wifi/WifiDriver.h"

#include <cstddef>
#include <string>
#include <vector>

namespace ewfm {

std::string wifiScanResponseJson(const std::vector<WifiNetwork>& networks);
std::string otaStatusResponseJson(size_t freeSketchSpace, bool hasError);

} // namespace ewfm
