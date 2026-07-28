#pragma once

#include <cstddef>
#include <string>

namespace ewfm {

// Blob-store keys map 1:1 onto filesystem paths under the devdata mount root, so validation must
// reject anything that could path-traverse, produce an absolute escape, or create ambiguous
// segments before a key is ever combined into a path.
constexpr size_t kBlobStoreMaxKeyBytes = 96;
constexpr size_t kBlobStoreMaxKeySegments = 8;

// A single '/'-separated path segment: length 1..N, charset [A-Za-z0-9_.-], and not exactly "."
// or "..".
bool isValidBlobKeySegment(const char* segment, size_t len);

// A full key: non-empty, <= kBlobStoreMaxKeyBytes, no leading/trailing/doubled '/', every
// '/'-separated segment passes isValidBlobKeySegment, and segment count <= kBlobStoreMaxKeySegments.
// Deliberately rejects "" - this is what lets wipeAll() be a distinct method rather than an
// accidental removeByPrefix("").
bool isValidBlobKey(const char* key, size_t len);

inline bool isValidBlobKey(const std::string& key) {
    return isValidBlobKey(key.data(), key.size());
}

} // namespace ewfm
