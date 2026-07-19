#pragma once

#include "metrics/MetricValueResolver.h"
#include "time/DateTime.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

// Sentinel used to decide whether a DateTime carries a real synced wall clock or is still the
// default-constructed "unset" value (epoch 0 -> year 1970). Mirrors AutoSwitchDevice.cpp's
// kAutoSwitchMinValidYear convention.
constexpr uint16_t kMetricTimeMinValidYear = 2020U;

// Hour-omitting "H:MM:SS" / "M:SS" duration text (moved verbatim from the pre-DateTime
// system.time code - this is now system.uptime's default, no-filter text).
void formatDurationDefault(uint32_t durationMs, char* output, size_t capacity);

// Token-pattern date/time formatting. Longest-match-first tokens: YYYY, YY, MM, M, DD, D, HH, H,
// mm, m, ss, s, EEEE, EEE. Text inside [square brackets] is copied literally (moment.js/date-fns
// escaping convention), e.g. "HH:mm:ss [hrs]". Returns false (output left empty) if the pattern
// contains a token this formatter doesn't recognize.
bool formatDateTimePattern(const DateTime& value, const char* pattern, char* output, size_t capacity);

// Duration token-pattern formatting on a plain millisecond count. Tokens: HH/H (TOTAL elapsed
// hours, not clamped to 24), mm/m, ss/s. Same [literal] escaping. Returns false for a
// DateTime-only token (YYYY, EEEE, ...).
bool formatDurationPattern(uint32_t durationMs, const char* pattern, char* output, size_t capacity);

// toFixed()-style fixed-decimal formatting. digits must be 0-6 (callers validate this at filter
// parse time). Returns false for a non-numeric MetricValue (anything but Int/Float/Duration) or a
// non-finite float (NaN/Inf).
bool formatFixedDecimals(const MetricValue& value, uint8_t digits, char* output, size_t capacity);

// True for Int/Float/Duration/DateTime. Used to expose a raw numeric preview alongside the
// preformatted MetricValue::text (e.g. for the REST placeholder catalog), so a client-side
// filter can reformat a value it only ever sees as a number, without needing MetricValue's C++
// layout.
bool metricValueHasNumericPreview(const MetricValue& value);

// Widens whichever alternative is active per valueType. DateTime's number is its unixtime()
// (already tz-adjusted local seconds, per DateTime.h's own "flavor of epoch" convention).
double metricValueNumericPreview(const MetricValue& value);

} // namespace ewfm
