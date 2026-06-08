#pragma once

#include "devices/core/DeviceTypes.h"

#include <utility>

namespace ewfm {

class IDeviceIdSource {
public:
    IDeviceIdSource() = default;
    IDeviceIdSource(const IDeviceIdSource&) = delete;
    IDeviceIdSource& operator=(const IDeviceIdSource&) = delete;
    IDeviceIdSource(IDeviceIdSource&&) = delete;
    IDeviceIdSource& operator=(IDeviceIdSource&&) = delete;
    virtual ~IDeviceIdSource() = default;

    virtual bool next(DeviceId& out) = 0;
};

class SequentialDeviceIdSource final : public IDeviceIdSource {
public:
    explicit SequentialDeviceIdSource(DeviceId start = 1) : next_(start) {}

    bool next(DeviceId& out) override {
        out = next_++;
        return true;
    }

private:
    DeviceId next_{1};
};

template <typename Predicate>
DeviceValidationResult assignUniqueDeviceId(IDeviceIdSource& source, Predicate&& isDuplicate, DeviceId& out, size_t maxAttempts = kMaxDeviceIdGenerationAttempts) {
    for (size_t attempt = 0; attempt < maxAttempts; ++attempt) {
        DeviceId candidate{0};
        if (!source.next(candidate)) {
            return {DeviceError::StorageError, "device id source failed"};
        }
        if (deviceIdIsReserved(candidate) || isDuplicate(candidate)) {
            continue;
        }
        out = candidate;
        return {};
    }
    return {DeviceError::DuplicateDeviceId, "unable to generate unique device id"};
}

#if defined(ARDUINO)
class EspRandomDeviceIdSource final : public IDeviceIdSource {
public:
    bool next(DeviceId& out) override;
};
#endif

} // namespace ewfm
