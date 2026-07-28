#pragma once

#include <cstddef>
#include <string>

namespace ewfm {

constexpr size_t kBlobKeyRandomSuffixLength = 8;
constexpr size_t kBlobKeyGenerationMaxAttempts = 5;

// One random [A-Za-z0-9] character. Backed by esp_random() on the device, rand() off-device (see
// RandomBlobKey.cpp) - the same ARDUINO/non-ARDUINO split already used by EspRandomDeviceIdSource
// (src/devices/core/DeviceIdGenerator.cpp).
char randomAlnumChar();

// Generates `prefix + "/" + <kBlobKeyRandomSuffixLength random alnum chars>` (or just the random
// suffix if `prefix` is empty), retrying on collision (as reported by `exists`) up to `maxAttempts`
// times. Templated on RandomCharFn/ExistsFn - not on any concrete filesystem type - so unit tests
// can drive the retry-on-collision path with a deterministic character sequence and a fake
// existence check, the same dependency-injection shape as assignUniqueDeviceId<Predicate> in
// src/devices/core/DeviceIdGenerator.h.
template <typename RandomCharFn, typename ExistsFn>
bool generateUniqueBlobKey(const std::string& prefix, RandomCharFn randomChar, ExistsFn exists, std::string& outKey,
                           size_t maxAttempts = kBlobKeyGenerationMaxAttempts) {
    for (size_t attempt = 0; attempt < maxAttempts; ++attempt) {
        std::string candidate = prefix;
        if (!candidate.empty()) {
            candidate += '/';
        }
        for (size_t i = 0; i < kBlobKeyRandomSuffixLength; ++i) {
            candidate += randomChar();
        }
        if (!exists(candidate)) {
            outKey = candidate;
            return true;
        }
    }
    return false;
}

} // namespace ewfm
