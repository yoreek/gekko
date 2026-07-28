#include "platform/BlobKeyValidation.h"

namespace ewfm {

namespace {
bool isValidSegmentChar(const char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-';
}
} // namespace

bool isValidBlobKeySegment(const char* segment, const size_t len) {
    if (segment == nullptr || len == 0U) {
        return false;
    }
    if (len == 1U && segment[0] == '.') {
        return false;
    }
    if (len == 2U && segment[0] == '.' && segment[1] == '.') {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        if (!isValidSegmentChar(segment[i])) {
            return false;
        }
    }
    return true;
}

bool isValidBlobKey(const char* key, const size_t len) {
    if (key == nullptr || len == 0U || len > kBlobStoreMaxKeyBytes) {
        return false;
    }
    if (key[0] == '/' || key[len - 1U] == '/') {
        return false;
    }

    size_t segmentCount = 0U;
    size_t segmentStart = 0U;
    for (size_t i = 0; i <= len; ++i) {
        if (i == len || key[i] == '/') {
            const size_t segmentLen = i - segmentStart;
            if (!isValidBlobKeySegment(key + segmentStart, segmentLen)) {
                return false;
            }
            ++segmentCount;
            if (segmentCount > kBlobStoreMaxKeySegments) {
                return false;
            }
            segmentStart = i + 1U;
        }
    }
    return true;
}

} // namespace ewfm
