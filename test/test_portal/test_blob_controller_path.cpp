#include "portal/controllers/BlobController.h"

#include <unity.h>

using namespace ewfm;

void test_blob_controller_parses_simple_key() {
    std::string key;
    TEST_ASSERT_TRUE(BlobController::parseBlobKeyForTest("/api/blobs/foo", key));
    TEST_ASSERT_EQUAL_STRING("foo", key.c_str());
}

void test_blob_controller_parses_nested_key_with_embedded_slashes() {
    std::string key;
    TEST_ASSERT_TRUE(BlobController::parseBlobKeyForTest("/api/blobs/foo/42/gauge1", key));
    TEST_ASSERT_EQUAL_STRING("foo/42/gauge1", key.c_str());
}

void test_blob_controller_rejects_missing_key() {
    std::string key;
    TEST_ASSERT_FALSE(BlobController::parseBlobKeyForTest("/api/blobs/", key));
    TEST_ASSERT_FALSE(BlobController::parseBlobKeyForTest("/api/blobs", key));
}

void test_blob_controller_rejects_wrong_prefix() {
    std::string key;
    TEST_ASSERT_FALSE(BlobController::parseBlobKeyForTest("/api/devices/42", key));
}

void test_blob_controller_rejects_path_traversal_key() {
    std::string key;
    TEST_ASSERT_FALSE(BlobController::parseBlobKeyForTest("/api/blobs/../secret", key));
    TEST_ASSERT_FALSE(BlobController::parseBlobKeyForTest("/api/blobs/foo/../bar", key));
}
