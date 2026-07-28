#include "platform/BlobKeyValidation.h"

#include <unity.h>

using namespace ewfm;

void test_blob_key_segment_accepts_valid_charset() {
    TEST_ASSERT_TRUE(isValidBlobKeySegment("gauge1", 6));
    TEST_ASSERT_TRUE(isValidBlobKeySegment("A-Z_0.9", 7));
}

void test_blob_key_segment_rejects_empty_dot_and_dotdot() {
    TEST_ASSERT_FALSE(isValidBlobKeySegment("", 0));
    TEST_ASSERT_FALSE(isValidBlobKeySegment(".", 1));
    TEST_ASSERT_FALSE(isValidBlobKeySegment("..", 2));
}

void test_blob_key_segment_rejects_bad_charset() {
    TEST_ASSERT_FALSE(isValidBlobKeySegment("a/b", 3));
    TEST_ASSERT_FALSE(isValidBlobKeySegment("a b", 3));
    TEST_ASSERT_FALSE(isValidBlobKeySegment("a\0b", 3));
}

void test_blob_key_accepts_nested_valid_key() {
    TEST_ASSERT_TRUE(isValidBlobKey(std::string("foo/42/gauge1")));
    TEST_ASSERT_TRUE(isValidBlobKey(std::string("single")));
}

void test_blob_key_rejects_empty_key() {
    // Empty must fail so wipeAll() has to be a distinct method rather than removeByPrefix("").
    TEST_ASSERT_FALSE(isValidBlobKey(std::string("")));
}

void test_blob_key_rejects_leading_trailing_and_doubled_slash() {
    TEST_ASSERT_FALSE(isValidBlobKey(std::string("/foo")));
    TEST_ASSERT_FALSE(isValidBlobKey(std::string("foo/")));
    TEST_ASSERT_FALSE(isValidBlobKey(std::string("foo//bar")));
}

void test_blob_key_rejects_path_traversal_segment() {
    TEST_ASSERT_FALSE(isValidBlobKey(std::string("foo/../bar")));
    TEST_ASSERT_FALSE(isValidBlobKey(std::string("..")));
}

void test_blob_key_rejects_oversized_key_and_too_many_segments() {
    const std::string oversized(kBlobStoreMaxKeyBytes + 1U, 'a');
    TEST_ASSERT_FALSE(isValidBlobKey(oversized));

    std::string tooManySegments;
    for (size_t i = 0; i <= kBlobStoreMaxKeySegments; ++i) {
        if (i > 0U) {
            tooManySegments += "/";
        }
        tooManySegments += "a";
    }
    TEST_ASSERT_FALSE(isValidBlobKey(tooManySegments));
}
