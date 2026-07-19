#pragma once

#include "devices/core/DeviceRuntimeBase.h"

namespace ewfm {

// Shared access protocol for runtimes that perform short, cooperative operations through a
// required bus dependency. BusDevice owns the transaction lease; this base only centralizes the
// repeated dependency lookup, readiness, and lease-acquisition checks.
template <typename BusDevice, DeviceRole Role> class BusDependentDeviceBase : public DeviceRuntimeBase {
protected:
    enum class DependencyAccessResult : uint8_t {
        Ready = 0,
        Missing = 1,
        Busy = 2,
    };

    explicit BusDependentDeviceBase(PState initialState) : DeviceRuntimeBase(initialState) {}

    BusDevice* dependencyBus() const {
        return static_cast<BusDevice*>(dependencyRuntime(Role));
    }

    bool dependencyBusReady() const {
        const BusDevice* dependency = dependencyBus();
        return dependency != nullptr && dependency->status() == DeviceStatus::Ready;
    }

    bool dependencyGenerationChanged(uint32_t lastGeneration) const {
        const BusDevice* dependency = dependencyBus();
        return dependency != nullptr && lastGeneration != 0U && dependency->generation() != lastGeneration;
    }

    DependencyAccessResult beginDependencyTransaction(typename BusDevice::DependencyTransaction& transaction) const {
        BusDevice* dependency = dependencyBus();
        if (dependency == nullptr || dependency->status() != DeviceStatus::Ready) {
            return DependencyAccessResult::Missing;
        }

        transaction = dependency->beginDependencyTransaction();
        return transaction ? DependencyAccessResult::Ready : DependencyAccessResult::Busy;
    }
};

} // namespace ewfm
