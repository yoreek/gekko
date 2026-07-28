#include "littlefs/FakeLittleFs.h"
#include "platform/BlobStoreDeviceCleanupSink.h"
#include "platform/LittleFsBlobStore.h"

#include <unity.h>

using namespace ewfm;

namespace {
using Store = LittleFsBlobStoreCore<test::FakeLittleFs, test::FakeFile>;

bool putString(Store& store, const std::string& key, const std::string& value) {
    Store::WriteHandle handle = store.beginPut(key);
    if (!handle.valid()) {
        return false;
    }
    if (!value.empty() && !handle.write(reinterpret_cast<const uint8_t*>(value.data()), value.size())) {
        return false;
    }
    return handle.commit();
}

DeviceEvent makeDeviceDeletedEvent(const DeviceId deviceId) {
    DeviceEvent event{};
    event.kind = DeviceEventKind::DeviceDeleted;
    event.deviceId = deviceId;
    return event;
}
} // namespace

void test_blob_store_device_cleanup_sink_removes_only_the_deleted_devices_blobs() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_TRUE(putString(store, "dev/2a/AAAAAAAA", "logo"));
    TEST_ASSERT_TRUE(putString(store, "dev/2a/BBBBBBBB", "icon"));
    TEST_ASSERT_TRUE(putString(store, "dev/2b/CCCCCCCC", "other-device"));

    BlobStoreDeviceCleanupSinkCore<Store> sink(store);
    sink.onDeviceEvent(makeDeviceDeletedEvent(0x2AU));

    TEST_ASSERT_FALSE(store.exists("dev/2a/AAAAAAAA"));
    TEST_ASSERT_FALSE(store.exists("dev/2a/BBBBBBBB"));
    TEST_ASSERT_TRUE(store.exists("dev/2b/CCCCCCCC"));
}

void test_blob_store_device_cleanup_sink_ignores_non_delete_events() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(putString(store, "dev/2a/AAAAAAAA", "logo"));

    BlobStoreDeviceCleanupSinkCore<Store> sink(store);
    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.deviceId = 0x2AU;
    sink.onDeviceEvent(updated);

    TEST_ASSERT_TRUE(store.exists("dev/2a/AAAAAAAA"));
}

void test_blob_store_device_cleanup_sink_deleting_device_with_no_blobs_is_a_no_op() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    BlobStoreDeviceCleanupSinkCore<Store> sink(store);
    sink.onDeviceEvent(makeDeviceDeletedEvent(0x99U));

    TEST_ASSERT_TRUE(store.begin()); // store still usable afterward
}
