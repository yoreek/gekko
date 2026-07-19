#include "portal/GrowableBuffer.h"

#include <cstdlib>

namespace ewfm {

GrowableBuffer::~GrowableBuffer() {
    std::free(data_);
}

bool GrowableBuffer::ensure(size_t n) {
    if (n <= capacity_) {
        return true;
    }
    size_t newCap = capacity_ < kMinCapacity ? kMinCapacity : capacity_;
    while (newCap < n) {
        newCap *= 2U;
    }
    void* grown = std::realloc(data_, newCap);
    if (grown == nullptr) {
        return false; // keep the existing block usable
    }
    data_ = static_cast<char*>(grown);
    capacity_ = newCap;
    return true;
}

} // namespace ewfm
