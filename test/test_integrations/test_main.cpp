#include <unity.h>

void test_device_api_adapter_registry_resolves_dummy();
void test_dummy_device_api_adapter_parses_create_request();
void test_dummy_device_api_adapter_rejects_invalid_payload();
void test_dummy_device_api_adapter_serializes_record();
void test_device_api_adapter_registry_resolves_onewire();
void test_onewire_api_adapter_parses_create_request();
void test_onewire_api_adapter_rejects_invalid_config_shape();
void test_onewire_api_adapter_serializes_runtime_scan_snapshot();
void test_onewire_api_adapter_parses_update_config_request();
void test_onewire_api_adapter_partial_update_preserves_internal_pullup();
void test_onewire_api_adapter_rejects_missing_update_config();
void test_ssd1306_device_api_adapter_encodes_layout_update_payload();

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_device_api_adapter_registry_resolves_dummy);
    RUN_TEST(test_dummy_device_api_adapter_parses_create_request);
    RUN_TEST(test_dummy_device_api_adapter_rejects_invalid_payload);
    RUN_TEST(test_dummy_device_api_adapter_serializes_record);
    RUN_TEST(test_device_api_adapter_registry_resolves_onewire);
    RUN_TEST(test_onewire_api_adapter_parses_create_request);
    RUN_TEST(test_onewire_api_adapter_rejects_invalid_config_shape);
    RUN_TEST(test_onewire_api_adapter_serializes_runtime_scan_snapshot);
    RUN_TEST(test_onewire_api_adapter_parses_update_config_request);
    RUN_TEST(test_onewire_api_adapter_partial_update_preserves_internal_pullup);
    RUN_TEST(test_onewire_api_adapter_rejects_missing_update_config);
    RUN_TEST(test_ssd1306_device_api_adapter_encodes_layout_update_payload);
    return UNITY_END();
}
