#pragma once

#include <ArduinoJson.h>
#include <string>

namespace ewfm {

struct DummyDeviceConfigV1;
struct DummyDeviceConfigV2;

std::string encodeDummyDeviceConfig(const DummyDeviceConfigV1& config);
std::string encodeDummyDeviceConfig(const DummyDeviceConfigV2& config);
bool decodeDummyDeviceConfig(const std::string& blob, DummyDeviceConfigV2& config);
bool parseDummyDeviceConfigJson(const JsonObjectConst& input, uint32_t configVersion, DummyDeviceConfigV2& config, std::string& error);
void writeDummyDeviceConfigJson(const DummyDeviceConfigV2& config, JsonObject output);

} // namespace ewfm
