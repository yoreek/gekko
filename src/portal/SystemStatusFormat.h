#pragma once

namespace ewfm {

// Pure mapping helpers for the /api/system/status payload. Values mirror the ESP-IDF
// esp_reset_reason_t and esp_partition_type_t/esp_partition_subtype_t numeric constants so the
// functions stay Arduino-free and natively testable. All return values are string literals.
const char* resetReasonToString(int espResetReason);
const char* partitionTypeToString(int type);
const char* partitionSubtypeToString(int type, int subtype);

} // namespace ewfm
