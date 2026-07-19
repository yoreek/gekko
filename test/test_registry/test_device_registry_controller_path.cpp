#include "portal/controllers/DeviceRegistryController.h"

#include <unity.h>

using namespace ewfm;

void test_device_registry_controller_parses_show_path_exactly() {
    DeviceId id = 0;
    TEST_ASSERT_TRUE(DeviceRegistryController::parseDeviceIdPathForTest("/api/devices/670845748", false, id));
    TEST_ASSERT_EQUAL_UINT32(670845748UL, static_cast<uint32_t>(id));

    id = 0;
    TEST_ASSERT_FALSE(DeviceRegistryController::parseDeviceIdPathForTest("/api/devices/670845748/extra", false, id));
    TEST_ASSERT_EQUAL_UINT32(0UL, static_cast<uint32_t>(id));
}

void test_device_registry_controller_requires_command_suffix_for_cmd() {
    DeviceId id = 0;
    TEST_ASSERT_TRUE(DeviceRegistryController::parseDeviceIdPathForTest("/api/devices/670845748/command", true, id));
    TEST_ASSERT_EQUAL_UINT32(670845748UL, static_cast<uint32_t>(id));

    id = 0;
    TEST_ASSERT_FALSE(DeviceRegistryController::parseDeviceIdPathForTest("/api/devices/670845748", true, id));
    TEST_ASSERT_EQUAL_UINT32(0UL, static_cast<uint32_t>(id));
}

void test_device_registry_controller_parses_layout_suffix() {
    DeviceId id = 0;
    TEST_ASSERT_TRUE(DeviceRegistryController::parseLayoutPathForTest("/api/devices/42/layout", id));
    TEST_ASSERT_EQUAL_UINT32(42UL, static_cast<uint32_t>(id));

    // The plain and command paths must not be mistaken for a layout request.
    id = 0;
    TEST_ASSERT_FALSE(DeviceRegistryController::parseLayoutPathForTest("/api/devices/42", id));
    TEST_ASSERT_FALSE(DeviceRegistryController::parseLayoutPathForTest("/api/devices/42/command", id));
    TEST_ASSERT_FALSE(DeviceRegistryController::parseLayoutPathForTest("/api/devices/42/layout/extra", id));
    TEST_ASSERT_EQUAL_UINT32(0UL, static_cast<uint32_t>(id));
}
