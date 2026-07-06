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
void test_ha_entity_adapter_registry_resolves_gpio_switch_and_rejects_unknown();
void test_gpio_switch_ha_entity_adapter_builds_discovery_payload();
void test_gpio_switch_ha_entity_adapter_builds_state_payload_for_on_off_and_skips_disabled();
void test_gpio_switch_ha_entity_adapter_parses_on_off_commands_case_insensitively();
void test_ha_discovery_bridge_publishes_birth_and_subscribes_on_connect();
void test_ha_discovery_bridge_skips_unopted_in_device_on_create();
void test_ha_discovery_bridge_publishes_discovery_and_state_after_opt_in();
void test_ha_discovery_bridge_retracts_discovery_on_delete();
void test_ha_discovery_bridge_routes_incoming_command_to_device_registry();

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
    RUN_TEST(test_ha_entity_adapter_registry_resolves_gpio_switch_and_rejects_unknown);
    RUN_TEST(test_gpio_switch_ha_entity_adapter_builds_discovery_payload);
    RUN_TEST(test_gpio_switch_ha_entity_adapter_builds_state_payload_for_on_off_and_skips_disabled);
    RUN_TEST(test_gpio_switch_ha_entity_adapter_parses_on_off_commands_case_insensitively);
    RUN_TEST(test_ha_discovery_bridge_publishes_birth_and_subscribes_on_connect);
    RUN_TEST(test_ha_discovery_bridge_skips_unopted_in_device_on_create);
    RUN_TEST(test_ha_discovery_bridge_publishes_discovery_and_state_after_opt_in);
    RUN_TEST(test_ha_discovery_bridge_retracts_discovery_on_delete);
    RUN_TEST(test_ha_discovery_bridge_routes_incoming_command_to_device_registry);
    return UNITY_END();
}
