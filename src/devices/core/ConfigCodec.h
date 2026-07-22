#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

// Legacy `*ConfigV<n>` structs are marked [[deprecated]] so that -Werror=deprecated-declarations
// turns any *active* use (a runtime member, an adapter's Config, an encode target) into a build
// error. Migration/decode code legitimately names old versions -- wrap those regions in this
// pair to opt out. This is the "exception for migrations" from docs/device-config-versioning.md.
#define EWFM_LEGACY_CONFIG_USE_BEGIN _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define EWFM_LEGACY_CONFIG_USE_END _Pragma("GCC diagnostic pop")

namespace ewfm {

template <size_t kMagicSize, typename T> constexpr size_t fixedConfigBlobSize(const char (&)[kMagicSize], const T&) {
    static_assert(kMagicSize > 0, "config magic must not be empty");
    static_assert(std::is_trivially_copyable<T>::value, "fixed config codec requires a trivially copyable type");
    return (kMagicSize - 1U) + sizeof(T);
}

template <size_t kMagicSize, typename T>
bool appendFixedConfigSegment(const char (&magic)[kMagicSize], const T& config, uint8_t* blob, size_t capacity, size_t& pos) {
    static_assert(kMagicSize > 0, "config magic must not be empty");
    static_assert(std::is_trivially_copyable<T>::value, "fixed config codec requires a trivially copyable type");
    const size_t size = (kMagicSize - 1U) + sizeof(T);
    if (pos + size > capacity) {
        return false;
    }
    std::memcpy(blob + pos, magic, kMagicSize - 1U);
    pos += kMagicSize - 1U;
    std::memcpy(blob + pos, &config, sizeof(T));
    pos += sizeof(T);
    return true;
}

template <size_t kMagicSize, typename T>
bool encodeFixedConfigBlob(const char (&magic)[kMagicSize], const T& config, uint8_t* blob, size_t capacity) {
    static_assert(kMagicSize > 0, "config magic must not be empty");
    static_assert(std::is_trivially_copyable<T>::value, "fixed config codec requires a trivially copyable type");
    const size_t size = (kMagicSize - 1U) + sizeof(T);
    if (capacity < size) {
        return false;
    }
    std::memcpy(blob, magic, kMagicSize - 1U);
    std::memcpy(blob + (kMagicSize - 1U), &config, sizeof(T));
    return true;
}

template <size_t kMagicSize, typename T>
bool readFixedConfigSegment(const char (&magic)[kMagicSize], const uint8_t* blob, size_t size, size_t& pos, T& config) {
    static_assert(kMagicSize > 0, "config magic must not be empty");
    static_assert(std::is_trivially_copyable<T>::value, "fixed config codec requires a trivially copyable type");
    const size_t segmentSize = (kMagicSize - 1U) + sizeof(T);
    if (pos + segmentSize > size) {
        return false;
    }
    if (std::memcmp(blob + pos, magic, kMagicSize - 1U) != 0) {
        return false;
    }
    pos += kMagicSize - 1U;
    std::memcpy(&config, blob + pos, sizeof(T));
    pos += sizeof(T);
    return true;
}

template <size_t kMagicSize, typename T>
bool decodeFixedConfigBlob(const char (&magic)[kMagicSize], const uint8_t* blob, size_t size, T& config) {
    static_assert(kMagicSize > 0, "config magic must not be empty");
    static_assert(std::is_trivially_copyable<T>::value, "fixed config codec requires a trivially copyable type");
    const size_t blobSize = (kMagicSize - 1U) + sizeof(T);
    if (size != blobSize) {
        return false;
    }
    if (std::memcmp(blob, magic, kMagicSize - 1U) != 0) {
        return false;
    }
    std::memcpy(&config, blob + (kMagicSize - 1U), sizeof(T));
    return true;
}

template <size_t kMagicSize, typename T>
bool decodeValidatedFixedConfigBlob(const char (&magic)[kMagicSize], const uint8_t* blob, size_t size, T& config) {
    return decodeFixedConfigBlob(magic, blob, size, config) && config.validate().ok();
}

} // namespace ewfm
