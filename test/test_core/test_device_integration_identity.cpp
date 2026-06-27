#include "integrations/common/DeviceIntegrationIdentity.h"

#include <unity.h>

using namespace ewfm;

void test_external_device_id_uses_controller_identity_and_device_id() {
    const std::string value = makeExternalDeviceId("esp32-main", 0x1A2B3C4D);
    TEST_ASSERT_EQUAL_STRING("esp32-main-dev-1a2b3c4d", value.c_str());
}

void test_external_device_id_sanitizes_controller_identity() {
    const std::string value = makeExternalDeviceId(" Home Hub #1 ", 42);
    TEST_ASSERT_EQUAL_STRING("home_hub__1-dev-0000002a", value.c_str());
}

void test_external_device_id_falls_back_for_empty_identity() {
    const std::string value = makeExternalDeviceId("   ", 7);
    TEST_ASSERT_EQUAL_STRING("controller-dev-00000007", value.c_str());
}
