#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceSetupTransferCodec.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <unity.h>

using namespace ewfm;

namespace {
struct FixedDeviceIdSource final : public IDeviceIdSource {
    explicit FixedDeviceIdSource(std::initializer_list<DeviceId> ids) : ids_(ids) {}

    bool next(DeviceId& out) override {
        if (index_ >= ids_.size()) {
            return false;
        }
        out = ids_[index_++];
        return true;
    }

    std::vector<DeviceId> ids_;
    size_t index_{0};
};

struct FailingStorage final : public IConfigStorage {
    bool begin(const char* namespaceName, bool readOnly) override {
        return storage_.begin(namespaceName, readOnly);
    }
    void end() override {
        storage_.end();
    }
    bool hasKey(const char* key) const override {
        return storage_.hasKey(key);
    }
    bool putString(const char* key, const std::string& value) override {
        return storage_.putString(key, value);
    }
    bool getString(const char* key, std::string& value) const override {
        return storage_.getString(key, value);
    }
    bool putBlob(const char* key, const uint8_t* value, size_t size) override {
        if (failNextBlob_) {
            failNextBlob_ = false;
            return false;
        }
        return storage_.putBlob(key, value, size);
    }
    bool getBlob(const char* key, uint8_t* value, size_t& size) const override {
        return storage_.getBlob(key, value, size);
    }
    bool putBlob(const char* key, const std::vector<uint8_t>& value) override {
        return putBlob(key, value.data(), value.size());
    }
    bool getBlob(const char* key, std::vector<uint8_t>& value) const override {
        return storage_.getBlob(key, value);
    }
    bool putUInt(const char* key, uint32_t value) override {
        return storage_.putUInt(key, value);
    }
    bool getUInt(const char* key, uint32_t& value) const override {
        return storage_.getUInt(key, value);
    }
    bool putBool(const char* key, bool value) override {
        return storage_.putBool(key, value);
    }
    bool getBool(const char* key, bool& value) const override {
        return storage_.getBool(key, value);
    }
    bool remove(const char* key) override {
        return storage_.remove(key);
    }
    bool clear() override {
        return storage_.clear();
    }

    void failNextBlobWrite() {
        failNextBlob_ = true;
    }

    MemoryConfigStorage storage_;
    bool failNextBlob_{false};
};

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyConfig(const DummyDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

void writeTextFile(const char* path, const std::string& text) {
    std::ofstream file(path, std::ios::trunc);
    TEST_ASSERT_TRUE(file.is_open());
    file << text;
    file.close();
}

DeviceCreateRequest makeDummyCreateRequest(const char* name) {
    DummyDeviceConfigV1 config{};
    config.enabled = 1U;
    TEST_ASSERT_TRUE(copyBoundedText(config.name, name));
    DeviceCreateRequest request{};
    request.typeId = DummyDevice::descriptor().typeId;
    request.name = name;
    request.configBlob = encodeDummyConfig(config);
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.enabled = true;
    return request;
}
} // namespace

void test_device_setup_export_includes_metadata_and_redacts_secret_strings() {
    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{1001};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("Aquarium Lamp"), 0).ok());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, registry.registryRevision()));
    TEST_ASSERT_TRUE(bundle.find("transferSchemaVersion") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"record\":{\"id\":") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"typeName\":\"dummy\"") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"deps\":[]") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("config_blob_hex") == std::string::npos);
    TEST_ASSERT_EQUAL(std::string::npos, bundle.find("\"status\""));
    TEST_ASSERT_EQUAL(std::string::npos, bundle.find("persistencePolicy"));
    TEST_ASSERT_EQUAL(std::string::npos, bundle.find("password"));
}

void test_device_setup_export_round_trips_back_into_registry() {
    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{1001};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("Aquarium Lamp"), 0).ok());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, registry.registryRevision()));

    const char* path = "/tmp/device_setup_transfer_bundle.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size());
    TEST_ASSERT_TRUE(parsed.ok());
    TEST_ASSERT_EQUAL_UINT32(1U, parsed.deviceCount);

    MemoryConfigStorage restoreStorage;
    FixedDeviceIdSource restoreIdSource{2001};
    DeviceRegistryStore restoreRegistryStore(restoreStorage);
    DeviceRegistry restored(restoreRegistryStore, typeRegistry, restoreIdSource);
    TEST_ASSERT_TRUE(restored.restore(parsed.snapshot, parsed.configBlobs, parsed.registryRevision, 0).ok());
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(restored.list().size()));
    TEST_ASSERT_NOT_NULL(restored.runtime(1001));
    TEST_ASSERT_EQUAL_STRING("Aquarium Lamp", restored.runtime(1001)->name());
}

void test_device_setup_transfer_rejects_unsupported_version() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":99,\"registrySchemaVersion\":1,\"registry"
                               "Revision\":1,\"deviceCount\":0}\n";
    const char* path = "/tmp/device_setup_transfer_bad_version.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size());
    TEST_ASSERT_FALSE(parsed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidVersion), static_cast<int>(parsed.validation.error));
}

void test_device_setup_transfer_rejects_legacy_flat_device_record() {
    const std::string bundle =
        "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":1,\"registrySchemaVersion\":1,\"registryRevision\":1,"
        "\"deviceCount\":1}\n"
        "{\"kind\":\"device\",\"id\":1001,\"typeName\":\"dummy\",\"configRevision\":1,\"name\":\"legacy\",\"enabled\":true,"
        "\"config_blob_hex\":\"00\"}\n";
    const char* path = "/tmp/device_setup_transfer_flat_legacy.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size());
    TEST_ASSERT_FALSE(parsed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(parsed.validation.error));
}

void test_device_setup_restore_failure_leaves_live_registry_unchanged() {
    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{1001};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("Aquarium Lamp"), 0).ok());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, registry.registryRevision()));

    const char* path = "/tmp/device_setup_transfer_restore_failure.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size());
    TEST_ASSERT_TRUE(parsed.ok());

    FailingStorage failingStorage;
    FixedDeviceIdSource failingIdSource{3001};
    DeviceRegistryStore failingRegistryStore(failingStorage);
    DeviceRegistry failingRegistry(failingRegistryStore, typeRegistry, failingIdSource);
    TEST_ASSERT_TRUE(failingRegistry.begin(0).ok());
    TEST_ASSERT_TRUE(failingRegistry.create(makeDummyCreateRequest("Existing Lamp"), 0).ok());
    failingStorage.failNextBlobWrite();

    const DeviceValidationResult restore = failingRegistry.restore(parsed.snapshot, parsed.configBlobs, parsed.registryRevision, 0);
    TEST_ASSERT_FALSE(restore.ok());
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(failingRegistry.list().size()));
    TEST_ASSERT_NOT_NULL(failingRegistry.runtime(3001));
    TEST_ASSERT_EQUAL_STRING("Existing Lamp", failingRegistry.runtime(3001)->name());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_device_setup_export_includes_metadata_and_redacts_secret_strings);
    RUN_TEST(test_device_setup_export_round_trips_back_into_registry);
    RUN_TEST(test_device_setup_transfer_rejects_unsupported_version);
    RUN_TEST(test_device_setup_transfer_rejects_legacy_flat_device_record);
    RUN_TEST(test_device_setup_restore_failure_leaves_live_registry_unchanged);
    return UNITY_END();
}
