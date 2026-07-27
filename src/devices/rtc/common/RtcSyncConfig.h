#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

// Shared by every RTC config's useForSystemTimeSync field (DS1302, DS3231, ...) - identical
// coercion/validation/serialization logic that would otherwise be duplicated per config struct.
// Free functions over the field by reference, not a base class: the RTC config structs already sit
// at different points in the DeviceBaseConfigV1/I2cDeviceConfigV1 inheritance chain (Ds3231's
// occupies the slot under I2cDeviceConfigV1), so a shared ancestor for just this field would need
// multiple inheritance over DeviceBaseConfigV1 - incompatible with the #pragma pack(1) /
// trivially-copyable requirement ConfigCodec.h's blob codec relies on. Mirrors I2cAddress.h.
bool parseRtcSyncFieldJson(const JsonObjectConst& input, uint8_t& useForSystemTimeSync, const char*& error);
DeviceValidationResult validateRtcSyncField(uint8_t useForSystemTimeSync);
void writeRtcSyncFieldJson(uint8_t useForSystemTimeSync, JsonObject output);

} // namespace ewfm
