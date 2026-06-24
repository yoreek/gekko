#include "devices/registry/DeviceRegistryPersistenceCoordinator.h"

#include <unity.h>

using namespace ewfm;

void test_persistence_coordinator_tracks_dirty_state_and_flush_window() {
    DeviceRegistryPersistenceCoordinator coordinator;
    coordinator.reset(10);

    TEST_ASSERT_FALSE(coordinator.hasPendingPersistence());
    TEST_ASSERT_EQUAL_UINT32(0, coordinator.firstDirtyAt());
    TEST_ASSERT_EQUAL_UINT32(10, coordinator.lastChangeAt());

    coordinator.markConfigDirty(101, 20);
    TEST_ASSERT_TRUE(coordinator.hasPendingPersistence());
    TEST_ASSERT_EQUAL_UINT32(20, coordinator.firstDirtyAt());
    TEST_ASSERT_EQUAL_UINT32(20, coordinator.lastChangeAt());
    TEST_ASSERT_EQUAL_UINT32(1, coordinator.dirtyConfigRecordIds().size());

    coordinator.markConfigDirty(101, 30);
    TEST_ASSERT_EQUAL_UINT32(1, coordinator.dirtyConfigRecordIds().size());
    TEST_ASSERT_EQUAL_UINT32(30, coordinator.lastChangeAt());
    TEST_ASSERT_FALSE(coordinator.shouldFlush(400, 500, 2000));
    TEST_ASSERT_TRUE(coordinator.shouldFlush(531, 500, 2000));

    coordinator.markRetainedDirty(101, 40);
    TEST_ASSERT_EQUAL_UINT32(1, coordinator.dirtyRetainedStateIds().size());

    TEST_ASSERT_TRUE(coordinator.hasAnyPersistenceWork());

    coordinator.clearConfigDirtyAfterImmediateFlush();
    TEST_ASSERT_TRUE(coordinator.hasPendingPersistence());
    coordinator.clearRetainedTracking(101);
    coordinator.markClean();
    TEST_ASSERT_FALSE(coordinator.hasPendingPersistence());
    TEST_ASSERT_FALSE(coordinator.hasAnyPersistenceWork());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_persistence_coordinator_tracks_dirty_state_and_flush_window);
    return UNITY_END();
}
