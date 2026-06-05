#include "update/FirmwareUpdate.h"

#include <unity.h>

using namespace ewfm;

void test_disabled_update_is_rejected() {
    FirmwareUpdatePolicy policy;
    policy.enabled = false;
    policy.availableBytes = 1024;
    TEST_ASSERT_EQUAL(static_cast<int>(FirmwareUpdateError::Disabled), static_cast<int>(policy.validate(128)));
}

void test_empty_image_is_rejected() {
    FirmwareUpdatePolicy policy;
    policy.enabled = true;
    policy.availableBytes = 1024;
    TEST_ASSERT_EQUAL(static_cast<int>(FirmwareUpdateError::EmptyImage), static_cast<int>(policy.validate(0)));
}

void test_oversized_image_is_rejected() {
    FirmwareUpdatePolicy policy;
    policy.enabled = true;
    policy.availableBytes = 1024;
    TEST_ASSERT_EQUAL(static_cast<int>(FirmwareUpdateError::ImageTooLarge), static_cast<int>(policy.validate(2048)));
}

void test_valid_image_is_accepted() {
    FirmwareUpdatePolicy policy;
    policy.enabled = true;
    policy.availableBytes = 1024;
    TEST_ASSERT_EQUAL(static_cast<int>(FirmwareUpdateError::None), static_cast<int>(policy.validate(512)));
}

void test_oversized_metadata_is_rejected() {
    FirmwareUpdatePolicy policy;
    policy.enabled = true;
    policy.availableBytes = 1024;
    policy.maxMetadataBytes = 8;
    TEST_ASSERT_EQUAL(static_cast<int>(FirmwareUpdateError::MetadataTooLarge), static_cast<int>(policy.validate(512, 9)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_disabled_update_is_rejected);
    RUN_TEST(test_empty_image_is_rejected);
    RUN_TEST(test_oversized_image_is_rejected);
    RUN_TEST(test_valid_image_is_accepted);
    RUN_TEST(test_oversized_metadata_is_rejected);
    return UNITY_END();
}
