#include "portal/SystemStatusFormat.h"

#include <unity.h>

using ewfm::partitionSubtypeToString;
using ewfm::partitionTypeToString;
using ewfm::resetReasonToString;

void test_system_status_format_maps_reset_reasons() {
    TEST_ASSERT_EQUAL_STRING("poweron", resetReasonToString(1));
    TEST_ASSERT_EQUAL_STRING("external", resetReasonToString(2));
    TEST_ASSERT_EQUAL_STRING("software", resetReasonToString(3));
    TEST_ASSERT_EQUAL_STRING("panic", resetReasonToString(4));
    TEST_ASSERT_EQUAL_STRING("interruptWatchdog", resetReasonToString(5));
    TEST_ASSERT_EQUAL_STRING("taskWatchdog", resetReasonToString(6));
    TEST_ASSERT_EQUAL_STRING("otherWatchdog", resetReasonToString(7));
    TEST_ASSERT_EQUAL_STRING("deepsleep", resetReasonToString(8));
    TEST_ASSERT_EQUAL_STRING("brownout", resetReasonToString(9));
    TEST_ASSERT_EQUAL_STRING("sdio", resetReasonToString(10));
    TEST_ASSERT_EQUAL_STRING("unknown", resetReasonToString(0));
    TEST_ASSERT_EQUAL_STRING("unknown", resetReasonToString(-1));
    TEST_ASSERT_EQUAL_STRING("unknown", resetReasonToString(99));
}

void test_system_status_format_maps_partition_types() {
    TEST_ASSERT_EQUAL_STRING("app", partitionTypeToString(0x00));
    TEST_ASSERT_EQUAL_STRING("data", partitionTypeToString(0x01));
    TEST_ASSERT_EQUAL_STRING("other", partitionTypeToString(0x40));
    TEST_ASSERT_EQUAL_STRING("other", partitionTypeToString(-1));
}

void test_system_status_format_maps_partition_subtypes() {
    TEST_ASSERT_EQUAL_STRING("factory", partitionSubtypeToString(0x00, 0x00));
    TEST_ASSERT_EQUAL_STRING("ota", partitionSubtypeToString(0x00, 0x10));
    TEST_ASSERT_EQUAL_STRING("ota", partitionSubtypeToString(0x00, 0x1F));
    TEST_ASSERT_EQUAL_STRING("test", partitionSubtypeToString(0x00, 0x20));
    TEST_ASSERT_EQUAL_STRING("other", partitionSubtypeToString(0x00, 0x21));

    TEST_ASSERT_EQUAL_STRING("otadata", partitionSubtypeToString(0x01, 0x00));
    TEST_ASSERT_EQUAL_STRING("phy", partitionSubtypeToString(0x01, 0x01));
    TEST_ASSERT_EQUAL_STRING("nvs", partitionSubtypeToString(0x01, 0x02));
    TEST_ASSERT_EQUAL_STRING("coredump", partitionSubtypeToString(0x01, 0x03));
    TEST_ASSERT_EQUAL_STRING("nvsKeys", partitionSubtypeToString(0x01, 0x04));
    TEST_ASSERT_EQUAL_STRING("efuse", partitionSubtypeToString(0x01, 0x05));
    TEST_ASSERT_EQUAL_STRING("fat", partitionSubtypeToString(0x01, 0x81));
    TEST_ASSERT_EQUAL_STRING("spiffs", partitionSubtypeToString(0x01, 0x82));
    TEST_ASSERT_EQUAL_STRING("other", partitionSubtypeToString(0x01, 0x7F));

    TEST_ASSERT_EQUAL_STRING("other", partitionSubtypeToString(0x40, 0x00));
}
