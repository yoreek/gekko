#include "platform/RandomBlobKey.h"

#include <unity.h>

using namespace ewfm;

namespace {
// Deterministic stand-in for randomAlnumChar(): yields "AAAAAAAA", then "BBBBBBBB", then
// "CCCCCCCC", ... on successive attempts (kBlobKeyRandomSuffixLength chars per attempt) - lets
// tests target a specific attempt without depending on real randomness.
class ScriptedRandomChar {
public:
    char operator()() {
        const char c = static_cast<char>('A' + (index_ / kBlobKeyRandomSuffixLength));
        ++index_;
        return c;
    }

private:
    size_t index_{0};
};
} // namespace

void test_generate_unique_blob_key_succeeds_on_first_attempt_with_no_prefix() {
    ScriptedRandomChar randomChar;
    std::string key;
    const bool ok = generateUniqueBlobKey("", randomChar, [](const std::string&) { return false; }, key);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("AAAAAAAA", key.c_str());
}

void test_generate_unique_blob_key_joins_prefix_with_slash() {
    ScriptedRandomChar randomChar;
    std::string key;
    const bool ok = generateUniqueBlobKey("dev1", randomChar, [](const std::string&) { return false; }, key);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("dev1/AAAAAAAA", key.c_str());
}

void test_generate_unique_blob_key_retries_on_collision() {
    ScriptedRandomChar randomChar;
    std::string key;
    // First attempt ("AAAAAAAA") collides, second ("BBBBBBBB") does not.
    const bool ok = generateUniqueBlobKey("", randomChar, [](const std::string& candidate) { return candidate == "AAAAAAAA"; }, key);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("BBBBBBBB", key.c_str());
}

void test_generate_unique_blob_key_fails_after_exhausting_attempts() {
    ScriptedRandomChar randomChar;
    std::string key;
    const bool ok = generateUniqueBlobKey("", randomChar, [](const std::string&) { return true; }, key, 3);
    TEST_ASSERT_FALSE(ok);
}
