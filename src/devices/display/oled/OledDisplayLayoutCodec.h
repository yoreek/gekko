#pragma once

#include "devices/display/oled/OledDisplayLayoutStore.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <vector>

namespace ewfm {

bool parseOledDisplayLayoutJson(const JsonObjectConst& input, OledDisplayLayoutRecordV1& layout);
void writeOledDisplayLayoutJson(const OledDisplayLayoutRecordV1& layout, JsonObject output);
bool encodeOledDisplayLayoutBinary(const OledDisplayLayoutRecordV1& layout, std::vector<uint8_t>& blob);
bool decodeOledDisplayLayoutBinary(const uint8_t* data, size_t size, OledDisplayLayoutRecordV1& layout);

} // namespace ewfm
