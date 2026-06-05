#pragma once

#include "wifi/WifiDriver.h"

#include <cstddef>
#include <vector>

class AsyncResponseStream;

namespace ewfm {

void writeWifiScanResponseJson(::AsyncResponseStream& out, const std::vector<WifiNetwork>& networks);
void writeOtaStatusResponseJson(::AsyncResponseStream& out, size_t freeSketchSpace, bool hasError);

} // namespace ewfm
