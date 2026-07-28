#include "littlefs/FakeLittleFs.h"
#include "platform/LittleFsBlobStore.h"

#include <unity.h>

using namespace ewfm;
using Store = LittleFsBlobStoreCore<test::FakeLittleFs, test::FakeFile>;

namespace {
bool getString(const Store& store, const std::string& key, std::string& out) {
    uint8_t buffer[256];
    size_t len = 0U;
    if (!store.get(key, buffer, sizeof(buffer), len)) {
        return false;
    }
    out.assign(reinterpret_cast<const char*>(buffer), len);
    return true;
}

bool putString(Store& store, const std::string& key, const std::string& value) {
    Store::WriteHandle handle = store.beginPut(key);
    if (!handle.valid()) {
        return false;
    }
    if (!value.empty() && !handle.write(reinterpret_cast<const uint8_t*>(value.data()), value.size())) {
        return false;
    }
    return handle.commit();
}
} // namespace

void test_blob_store_put_then_get_round_trips_bytes() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_TRUE(putString(store, "foo/bar", "hello world"));
    std::string out;
    TEST_ASSERT_TRUE(getString(store, "foo/bar", out));
    TEST_ASSERT_EQUAL_STRING("hello world", out.c_str());
}

void test_blob_store_commit_atomically_replaces_existing_blob() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_TRUE(putString(store, "k", "first"));
    TEST_ASSERT_TRUE(putString(store, "k", "second-longer-value"));

    std::string out;
    TEST_ASSERT_TRUE(getString(store, "k", out));
    TEST_ASSERT_EQUAL_STRING("second-longer-value", out.c_str());
}

void test_blob_store_uncommitted_write_leaves_existing_blob_untouched() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_TRUE(putString(store, "k", "original"));
    {
        Store::WriteHandle handle = store.beginPut("k");
        TEST_ASSERT_TRUE(handle.valid());
        const uint8_t garbage[] = {1, 2, 3};
        TEST_ASSERT_TRUE(handle.write(garbage, sizeof(garbage)));
        // handle goes out of scope here without commit() - destructor must abort, not commit.
    }

    std::string out;
    TEST_ASSERT_TRUE(getString(store, "k", out));
    TEST_ASSERT_EQUAL_STRING("original", out.c_str());
}

void test_blob_store_remove_by_prefix_on_leaf_key_removes_only_that_file() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_TRUE(putString(store, "a/x", "x"));
    TEST_ASSERT_TRUE(putString(store, "a/y", "y"));

    TEST_ASSERT_TRUE(store.removeByPrefix("a/x"));

    TEST_ASSERT_FALSE(store.exists("a/x"));
    TEST_ASSERT_TRUE(store.exists("a/y"));
}

void test_blob_store_remove_by_prefix_on_directory_removes_recursively() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_TRUE(putString(store, "dev/1/gauge", "g1"));
    TEST_ASSERT_TRUE(putString(store, "dev/1/label", "l1"));
    TEST_ASSERT_TRUE(putString(store, "dev/2/gauge", "g2"));

    TEST_ASSERT_TRUE(store.removeByPrefix("dev/1"));

    TEST_ASSERT_FALSE(store.exists("dev/1/gauge"));
    TEST_ASSERT_FALSE(store.exists("dev/1/label"));
    TEST_ASSERT_TRUE(store.exists("dev/2/gauge"));
}

void test_blob_store_remove_by_prefix_on_missing_prefix_is_a_no_op_success() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_TRUE(putString(store, "keep", "v"));
    TEST_ASSERT_TRUE(store.removeByPrefix("nothing/here"));
    TEST_ASSERT_TRUE(store.exists("keep"));
}

void test_blob_store_wipe_all_clears_store_but_leaves_it_usable() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_TRUE(putString(store, "a", "1"));
    TEST_ASSERT_TRUE(putString(store, "b/c", "2"));

    TEST_ASSERT_TRUE(store.wipeAll());

    TEST_ASSERT_FALSE(store.exists("a"));
    TEST_ASSERT_FALSE(store.exists("b/c"));
    // Store must still be usable afterward - wipeAll() recreates its own root.
    TEST_ASSERT_TRUE(putString(store, "a", "again"));
    std::string out;
    TEST_ASSERT_TRUE(getString(store, "a", out));
    TEST_ASSERT_EQUAL_STRING("again", out.c_str());
}

void test_blob_store_confines_all_operations_to_its_reserved_root() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    // Simulate another feature's data living outside the store's reserved root, exactly like the
    // dose journal's "/dj" and schedule presets' "/sap" share the same devdata mount.
    TEST_ASSERT_TRUE(fs.mkdir("/dj"));
    {
        test::FakeFile f = fs.open("/dj/journal.bin", "w", true);
        TEST_ASSERT_TRUE(static_cast<bool>(f));
        const uint8_t data[] = {9, 9, 9};
        f.write(data, sizeof(data));
        f.close();
    }

    TEST_ASSERT_TRUE(putString(store, "x", "y"));
    TEST_ASSERT_TRUE(store.wipeAll());

    // A generic REST client can never construct a key that reaches "/dj" - confirm wipeAll(),
    // which iterates only the store's own reserved root, left it completely untouched.
    TEST_ASSERT_TRUE(fs.exists("/dj/journal.bin"));
}

void test_blob_store_refuses_write_when_free_space_guard_trips() {
    test::FakeLittleFs fs;
    fs.setCapacity(10000U, 10000U - kBlobStoreMinFreeBytes + 1U); // 1 byte under the free-space floor
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    Store::WriteHandle handle = store.beginPut("k");
    TEST_ASSERT_FALSE(handle.valid());
}

void test_blob_store_begin_put_generated_returns_working_handle_under_prefix() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    std::string key;
    Store::WriteHandle handle = store.beginPutGenerated("dev1", key);
    TEST_ASSERT_TRUE(handle.valid());
    TEST_ASSERT_EQUAL_STRING("dev1/", key.substr(0, 5).c_str());
    TEST_ASSERT_EQUAL_UINT32(5U + kBlobKeyRandomSuffixLength, key.size());

    const uint8_t data[] = {1, 2, 3};
    TEST_ASSERT_TRUE(handle.write(data, sizeof(data)));
    TEST_ASSERT_TRUE(handle.commit());

    std::string out;
    TEST_ASSERT_TRUE(getString(store, key, out));
    TEST_ASSERT_EQUAL_UINT32(3U, out.size());
}

void test_blob_store_begin_put_generated_avoids_existing_key() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    std::string firstKey;
    TEST_ASSERT_TRUE(store.beginPutGenerated("dev1", firstKey).valid());
    TEST_ASSERT_TRUE(putString(store, firstKey, "taken"));

    std::string secondKey;
    Store::WriteHandle handle = store.beginPutGenerated("dev1", secondKey);
    TEST_ASSERT_TRUE(handle.valid());
    TEST_ASSERT_TRUE(secondKey != firstKey);
    handle.abort();
}

void test_blob_store_auto_creates_intermediate_directories_for_nested_keys() {
    test::FakeLittleFs fs;
    Store store(fs);
    TEST_ASSERT_TRUE(store.begin());

    // No mkdir of "deeply/nested/path" is ever called - beginPut() must create it on its own.
    TEST_ASSERT_TRUE(putString(store, "deeply/nested/path/leaf", "v"));
    std::string out;
    TEST_ASSERT_TRUE(getString(store, "deeply/nested/path/leaf", out));
    TEST_ASSERT_EQUAL_STRING("v", out.c_str());
}
