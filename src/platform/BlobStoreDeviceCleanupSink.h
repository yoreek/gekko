#pragma once

#include "integrations/common/DeviceEventBus.h"
#include "platform/LittleFsBlobStore.h"

#include <cstdio>

namespace ewfm {

// Purges every blob a device owns when the device is deleted from the registry. Relies on the
// "dev/<deviceId as hex>/<random suffix>" key convention (docs/blob-store.md) - any feature that
// uploads a device-scoped blob under that prefix gets its cleanup for free here, without needing
// its own cleanup sink (unlike DoseJournalCleanupSink/SchedulePresetCleanupSink, which each own a
// distinct devdata directory rather than a key prefix within the generic blob store).
//
// Templated on the store type (same reason as LittleFsBlobStoreCore itself) so a native unit test
// can inject LittleFsBlobStoreCore<test::FakeLittleFs, test::FakeFile> and verify the actual
// removeByPrefix call happens, rather than only being exercisable against the real hardware type.
template <typename Store> class BlobStoreDeviceCleanupSinkCore final : public IDeviceEventSink {
public:
    explicit BlobStoreDeviceCleanupSinkCore(Store& store) : store_(store) {}

    void onDeviceEvent(const DeviceEvent& event) override {
        if (event.kind != DeviceEventKind::DeviceDeleted) {
            return;
        }
        char prefix[16]{};
        std::snprintf(prefix, sizeof(prefix), "dev/%x", static_cast<unsigned int>(event.deviceId));
        (void)store_.removeByPrefix(prefix);
    }

    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}

private:
    Store& store_;
};

using BlobStoreDeviceCleanupSink = BlobStoreDeviceCleanupSinkCore<LittleFsBlobStore>;

} // namespace ewfm
