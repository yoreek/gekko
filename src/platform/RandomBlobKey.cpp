#include "platform/RandomBlobKey.h"

#if defined(ARDUINO)
#include <esp_random.h>
#include <esp_system.h>
#else
#include <cstdlib>
#endif

namespace ewfm {

namespace {
constexpr char kAlnumAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
constexpr size_t kAlnumAlphabetSize = sizeof(kAlnumAlphabet) - 1; // exclude the trailing '\0'
} // namespace

char randomAlnumChar() {
#if defined(ARDUINO)
    return kAlnumAlphabet[::esp_random() % kAlnumAlphabetSize];
#else
    return kAlnumAlphabet[static_cast<size_t>(::rand()) % kAlnumAlphabetSize]; // NOLINT
#endif
}

} // namespace ewfm
