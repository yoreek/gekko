#pragma once

#include <cstddef>

namespace ewfm {

// A single heap block that grows by doubling and is never shrunk. Reused across all pieces of one
// chunked response so that, instead of one alloc+free per emitted item (heavy heap churn, and a
// fresh contiguous request per item on a fragmented heap), only a handful of reallocs happen before
// the buffer settles at the size of the largest single piece. Peak occupancy therefore stays bound
// to one piece - the property the chunked-response fix depends on - with far fewer allocations.
class GrowableBuffer {
public:
    GrowableBuffer() = default;
    ~GrowableBuffer();

    GrowableBuffer(const GrowableBuffer&) = delete;
    GrowableBuffer& operator=(const GrowableBuffer&) = delete;

    // Ensure capacity() >= n, growing via realloc (doubling) if needed. Returns false on allocation
    // failure, leaving the existing buffer intact.
    bool ensure(size_t n);

    char* data() {
        return data_;
    }
    const char* data() const {
        return data_;
    }
    size_t capacity() const {
        return capacity_;
    }

private:
    static constexpr size_t kMinCapacity = 64;

    char* data_ = nullptr;
    size_t capacity_ = 0;
};

} // namespace ewfm
