#pragma once

#include <cstdint>

namespace ewfm {

class ISystemStats {
public:
    virtual ~ISystemStats() = default;
    virtual uint32_t freeHeapBytes() const = 0;
    // 0-100, computed as 100 - (largest allocatable block * 100 / free heap).
    virtual uint8_t heapFragmentationPercent() const = 0;
    // Lowest free heap observed since boot (low-water mark).
    virtual uint32_t minFreeHeapBytes() const = 0;
    // Largest single contiguous block currently allocatable - the number that actually predicts
    // large-allocation failures, unlike total free heap.
    virtual uint32_t largestFreeBlockBytes() const = 0;
};

class ManualSystemStats final : public ISystemStats {
public:
    uint32_t freeHeapBytes() const override {
        return freeHeapBytes_;
    }
    uint8_t heapFragmentationPercent() const override {
        return heapFragmentationPercent_;
    }
    uint32_t minFreeHeapBytes() const override {
        return minFreeHeapBytes_;
    }
    uint32_t largestFreeBlockBytes() const override {
        return largestFreeBlockBytes_;
    }
    void set(uint32_t freeHeapBytes, uint8_t heapFragmentationPercent, uint32_t minFreeHeapBytes = 0, uint32_t largestFreeBlockBytes = 0) {
        freeHeapBytes_ = freeHeapBytes;
        heapFragmentationPercent_ = heapFragmentationPercent;
        minFreeHeapBytes_ = minFreeHeapBytes;
        largestFreeBlockBytes_ = largestFreeBlockBytes;
    }

private:
    uint32_t freeHeapBytes_{0};
    uint8_t heapFragmentationPercent_{0};
    uint32_t minFreeHeapBytes_{0};
    uint32_t largestFreeBlockBytes_{0};
};

} // namespace ewfm
