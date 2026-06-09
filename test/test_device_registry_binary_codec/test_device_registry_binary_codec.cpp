#include "devices/registry/DeviceRegistryBinaryCodec.h"

#include <unity.h>

using namespace ewfm;

namespace {

DeviceRecord makeRecord(DeviceId id, DeviceTypeId typeId, const char* name, const std::string& payload) {
    DeviceRecord record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = id;
    record.header.typeId = typeId;
    record.header.configVersion = 2;
    record.header.configRevision = 7;
    record.header.payloadLength = static_cast<uint32_t>(payload.size());
    record.header.payloadChecksum = DeviceRegistryBinaryCodec::payloadChecksum(payload);
    record.name = name;
    record.enabled = true;
    record.hasParent = false;
    record.parentDeviceId = 0;
    record.status = DeviceStatus::Ready;
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    record.configPayload = payload;
    return record;
}

} // namespace

void test_codec_hex_round_trip() {
    const std::string source = "\x01\x02\xA3\xff";
    const std::string hex = DeviceRegistryBinaryCodec::toHex(source);
    TEST_ASSERT_EQUAL_STRING("0102a3ff", hex.c_str());

    std::string restored;
    TEST_ASSERT_TRUE(DeviceRegistryBinaryCodec::fromHex(hex, restored));
    TEST_ASSERT_EQUAL_UINT32(source.size(), restored.size());
    TEST_ASSERT_EQUAL_MEMORY(source.data(), restored.data(), source.size());
}

void test_codec_index_round_trip() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({101, 1});
    snapshot.indexEntries.push_back({102, 2});

    const std::string blob = DeviceRegistryBinaryCodec::serializeIndex(snapshot);
    DeviceRegistrySnapshot decoded{};
    DeviceValidationResult result = DeviceRegistryBinaryCodec::parseIndex(blob, decoded);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(2, decoded.indexEntries.size());
    TEST_ASSERT_EQUAL_UINT32(101, decoded.indexEntries[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(2, decoded.indexEntries[1].typeId);
}

void test_codec_record_round_trip() {
    DeviceRecord record = makeRecord(501, 7, "sensor", "payload-v1");
    record.hasParent = true;
    record.parentDeviceId = 500;
    record.enabled = false;
    record.status = DeviceStatus::DependencyBlocked;
    record.persistencePolicy = DevicePersistencePolicy::Coalesced;

    const std::string blob = DeviceRegistryBinaryCodec::serializeRecord(record);
    DeviceRecord decoded{};
    DeviceValidationResult result = DeviceRegistryBinaryCodec::parseRecord(blob, decoded);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(record.header.deviceId, decoded.header.deviceId);
    TEST_ASSERT_EQUAL_UINT32(record.header.typeId, decoded.header.typeId);
    TEST_ASSERT_EQUAL_UINT32(record.header.configVersion, decoded.header.configVersion);
    TEST_ASSERT_EQUAL_UINT32(record.header.configRevision, decoded.header.configRevision);
    TEST_ASSERT_EQUAL_UINT32(record.header.payloadLength, decoded.header.payloadLength);
    TEST_ASSERT_EQUAL_UINT32(record.header.payloadChecksum, decoded.header.payloadChecksum);
    TEST_ASSERT_EQUAL_STRING(record.name.c_str(), decoded.name.c_str());
    TEST_ASSERT_FALSE(decoded.enabled);
    TEST_ASSERT_TRUE(decoded.hasParent);
    TEST_ASSERT_EQUAL_UINT32(record.parentDeviceId, decoded.parentDeviceId);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(decoded.status));
    TEST_ASSERT_EQUAL(static_cast<int>(DevicePersistencePolicy::Coalesced), static_cast<int>(decoded.persistencePolicy));
    TEST_ASSERT_EQUAL_STRING(record.configPayload.c_str(), decoded.configPayload.c_str());
}

void test_codec_record_rejects_checksum_mismatch() {
    const DeviceRecord record = makeRecord(701, 9, "node", "abcdef");
    std::string blob = DeviceRegistryBinaryCodec::serializeRecord(record);
    TEST_ASSERT_TRUE(blob.size() > 0);

    blob[blob.size() - 1] = static_cast<char>(blob.back() ^ 0x01);
    DeviceRecord decoded{};
    DeviceValidationResult result = DeviceRegistryBinaryCodec::parseRecord(blob, decoded);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(result.error));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_codec_hex_round_trip);
    RUN_TEST(test_codec_index_round_trip);
    RUN_TEST(test_codec_record_round_trip);
    RUN_TEST(test_codec_record_rejects_checksum_mismatch);
    return UNITY_END();
}
