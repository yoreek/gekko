#pragma once

#include "devices/display/DisplayLayoutStore.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <vector>

namespace ewfm {

bool parseDisplayLayoutJson(const JsonObjectConst& input, DisplayLayoutRecordV1& layout);
void writeDisplayLayoutJson(const DisplayLayoutRecordV1& layout, JsonObject output);
bool encodeDisplayLayoutBinary(const DisplayLayoutRecordV1& layout, std::vector<uint8_t>& blob);
bool decodeDisplayLayoutBinary(const uint8_t* data, size_t size, DisplayLayoutRecordV1& layout);

} // namespace ewfm
