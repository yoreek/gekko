#include "devices/sensors/temperature/TemperatureReadingPublisher.h"

#include <unity.h>

using namespace ewfm;

void test_temperature_reading_publisher_publishes_first_reading_and_deadbands_small_changes() {
    TemperatureReadingPublisher publisher;
    publisher.configure(false, 10);

    TEST_ASSERT_TRUE(publisher.publish(23000, 100));
    TEST_ASSERT_TRUE(publisher.reading().valid);
    TEST_ASSERT_EQUAL_INT32(23000, publisher.reading().milliCelsius);
    TEST_ASSERT_EQUAL_STRING("ok", publisher.status());

    // A change smaller than the report delta should not require a dirty publish.
    TEST_ASSERT_FALSE(publisher.publish(23005, 200));
    TEST_ASSERT_EQUAL_INT32(23005, publisher.reading().milliCelsius);

    // A change large enough to cross the delta should require a dirty publish.
    TEST_ASSERT_TRUE(publisher.publish(23200, 300));
}

void test_temperature_reading_publisher_report_always_forces_dirty_on_every_publish() {
    TemperatureReadingPublisher publisher;
    publisher.configure(true, 100);

    TEST_ASSERT_TRUE(publisher.publish(20000, 10));
    TEST_ASSERT_TRUE(publisher.publish(20000, 20));
    TEST_ASSERT_TRUE(publisher.publish(20000, 30));
}

void test_temperature_reading_publisher_invalidate_marks_dirty_once_and_clears_reading() {
    TemperatureReadingPublisher publisher;
    publisher.configure(false, 10);
    publisher.publish(20000, 10);

    TEST_ASSERT_TRUE(publisher.invalidate("not_found"));
    TEST_ASSERT_FALSE(publisher.reading().valid);
    TEST_ASSERT_EQUAL_INT32(0, publisher.reading().milliCelsius);
    TEST_ASSERT_EQUAL_STRING("not_found", publisher.status());

    // Repeating the same invalidation status should not require another dirty publish.
    TEST_ASSERT_FALSE(publisher.invalidate("not_found"));

    // A different status should require another dirty publish, even though the reading stays invalid.
    TEST_ASSERT_TRUE(publisher.invalidate("crc_error"));
}

void test_temperature_reading_publisher_republishes_after_recovering_from_error_status() {
    TemperatureReadingPublisher publisher;
    publisher.configure(false, 100);
    publisher.publish(20000, 10);
    publisher.invalidate("crc_error");

    // Republishing the exact same value after an error status must still be reported, since the
    // status transitions from "crc_error" back to "ok".
    TEST_ASSERT_TRUE(publisher.publish(20000, 20));
}

void test_temperature_reading_publisher_forces_heartbeat_republish_after_five_minutes_unchanged() {
    TemperatureReadingPublisher publisher;
    publisher.configure(false, 10);

    TEST_ASSERT_TRUE(publisher.publish(23000, 1000));

    // Same value, well within the deadband, less than 5 minutes later: no forced republish.
    TEST_ASSERT_FALSE(publisher.publish(23000, 1000 + 299999U));

    // Same value, but now 5 minutes have elapsed since the *last actual publish*: force one even
    // though nothing changed, so observers can see the sensor is still alive.
    TEST_ASSERT_TRUE(publisher.publish(23000, 1000 + 300000U));

    // The heartbeat clock resets from that forced publish, not from every call.
    TEST_ASSERT_FALSE(publisher.publish(23000, 1000 + 300000U + 1U));
}

void test_temperature_reading_publisher_set_status_quietly_does_not_mark_dirty() {
    TemperatureReadingPublisher publisher;
    publisher.configure(false, 10);
    publisher.invalidate("dependency_busy");

    publisher.setStatusQuietly("not_ready");
    TEST_ASSERT_EQUAL_STRING("not_ready", publisher.status());
    TEST_ASSERT_FALSE(publisher.reading().valid);
}
