#include "portal/GrowableBuffer.h"
#include "portal/controllers/BaseController.h"

#include <ArduinoJson.h>
#include <string>
#include <unity.h>

using namespace ewfm;

namespace {
std::string pieceAsString(const BaseController::ChunkedBody& body) {
    return std::string(body.buf.data(), body.len);
}
} // namespace

void test_growable_buffer_starts_at_min_capacity_and_doubles() {
    GrowableBuffer buf;
    TEST_ASSERT_TRUE(buf.ensure(10));
    // A tiny request still lands on the minimum capacity (64), not an exact 10-byte block.
    TEST_ASSERT_EQUAL_UINT32(64U, static_cast<uint32_t>(buf.capacity()));

    // One byte past capacity doubles rather than fitting exactly.
    TEST_ASSERT_TRUE(buf.ensure(65));
    TEST_ASSERT_EQUAL_UINT32(128U, static_cast<uint32_t>(buf.capacity()));
}

void test_growable_buffer_never_shrinks_and_reuses_block() {
    GrowableBuffer buf;
    TEST_ASSERT_TRUE(buf.ensure(200));
    const size_t grownCapacity = buf.capacity();
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(200U, static_cast<uint32_t>(grownCapacity));
    char* grownData = buf.data();

    // A smaller request neither reallocates nor shrinks.
    TEST_ASSERT_TRUE(buf.ensure(8));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(grownCapacity), static_cast<uint32_t>(buf.capacity()));
    TEST_ASSERT_EQUAL_PTR(grownData, buf.data());
}

void test_chunked_body_emit_copies_raw_bytes() {
    BaseController::ChunkedBody body;
    TEST_ASSERT_TRUE(body.emit("]}", 2));
    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(body.len));
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(body.pos));
    TEST_ASSERT_EQUAL_STRING("]}", pieceAsString(body).c_str());
}

void test_chunked_body_emit_json_with_and_without_leading_comma() {
    BaseController::ChunkedBody body;

    StaticJsonDocument<64> doc;
    doc["id"] = 7;

    TEST_ASSERT_TRUE(body.emitJson(doc, false));
    TEST_ASSERT_EQUAL_STRING("{\"id\":7}", pieceAsString(body).c_str());

    TEST_ASSERT_TRUE(body.emitJson(doc, true));
    TEST_ASSERT_EQUAL_STRING(",{\"id\":7}", pieceAsString(body).c_str());
}

void test_chunked_body_reuses_buffer_across_pieces() {
    BaseController::ChunkedBody body;

    StaticJsonDocument<128> big;
    for (int i = 0; i < 4; ++i) {
        big[std::string("k") + std::to_string(i)] = i;
    }
    TEST_ASSERT_TRUE(body.emitJson(big, false));
    const size_t capacityAfterBig = body.buf.capacity();

    // A smaller follow-up piece keeps the grown buffer and reports the correct shorter length.
    StaticJsonDocument<32> small;
    small["k"] = 1;
    TEST_ASSERT_TRUE(body.emitJson(small, false));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(capacityAfterBig), static_cast<uint32_t>(body.buf.capacity()));
    TEST_ASSERT_EQUAL_STRING("{\"k\":1}", pieceAsString(body).c_str());
}
