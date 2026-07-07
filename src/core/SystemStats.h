#pragma once

#include <cstdint>

namespace ewfm {

class ISystemStats {
public:
    virtual ~ISystemStats() = default;
    virtual uint32_t freeHeapBytes() const = 0;
    // 0-100, computed as 100 - (largest allocatable block * 100 / free heap).
    virtual uint8_t heapFragmentationPercent() const = 0;
};

class ManualSystemStats final : public ISystemStats {
public:
    uint32_t freeHeapBytes() const override {
        return freeHeapBytes_;
    }
    uint8_t heapFragmentationPercent() const override {
        return heapFragmentationPercent_;
    }
    void set(uint32_t freeHeapBytes, uint8_t heapFragmentationPercent) {
        freeHeapBytes_ = freeHeapBytes;
        heapFragmentationPercent_ = heapFragmentationPercent;
    }

private:
    uint32_t freeHeapBytes_{0};
    uint8_t heapFragmentationPercent_{0};
};

} // namespace ewfm
