#pragma once

#include "time/DateTime.h"

#include <cstddef>
#include <optional>

namespace ewfm {

// ISO 8601 parsing/formatting, kept separate from DateTime so the value type itself has no
// string-format knowledge. Accepts "YYYY-MM-DD(T| )HH:MM[:SS]" optionally followed by "Z" or a
// "+HH:MM"/"-HH:MM" offset. Returns std::nullopt (not a flagged-invalid object) on any malformed
// input - std::optional has zero heap cost (inline bool + storage), matching this project's
// preference for stack/static allocation.
std::optional<DateTime> parseIso8601(const char* str, size_t len);

// Formats into a caller-owned buffer (no heap allocation). Uses "Z" for zero offset, otherwise
// "+HH:MM"/"-HH:MM".
void formatIso8601(const DateTime& dt, char* buf, size_t bufSize);

} // namespace ewfm
