#include <unity.h>

void test_base_controller_dispatches_action_to_virtual_override();
void test_base_controller_parses_valid_and_invalid_json_body();
void test_base_controller_default_chain_rejects_bodyless_create_dispatch();
void test_base_controller_upload_style_chain_allows_bodyless_create_dispatch();
void test_base_controller_builds_standard_envelopes();
void test_base_controller_exposes_expected_cors_headers();
void test_base_controller_request_file_defaults_and_tmp_dir();
void test_base_controller_request_state_defaults();
void test_base_controller_request_body_helper_accumulates_and_clears();
void test_base_controller_request_body_helper_handles_null_request();
void test_base_controller_upload_lifecycle_tracks_files_and_limits();
void test_base_controller_upload_append_finish_and_claim();
void test_fake_esp_async_web_server_header_is_available();
void test_ws_message_builders_create_compact_envelopes();
void test_ws_manager_attaches_and_detaches_from_dispatcher();
void test_ws_manager_receives_device_events_when_attached();
void test_ws_manager_stops_receiving_after_detach();
void test_ws_manager_ignores_registry_persistence_cleared_events();
void test_ws_manager_broadcasts_snapshots_only_when_clients_are_connected();
void test_ws_manager_resyncs_all_device_snapshots_for_new_clients();
void test_ws_status_messages_are_serializable();

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_base_controller_dispatches_action_to_virtual_override);
    RUN_TEST(test_base_controller_parses_valid_and_invalid_json_body);
    RUN_TEST(test_base_controller_default_chain_rejects_bodyless_create_dispatch);
    RUN_TEST(test_base_controller_upload_style_chain_allows_bodyless_create_dispatch);
    RUN_TEST(test_base_controller_builds_standard_envelopes);
    RUN_TEST(test_base_controller_exposes_expected_cors_headers);
    RUN_TEST(test_base_controller_request_file_defaults_and_tmp_dir);
    RUN_TEST(test_base_controller_request_state_defaults);
    RUN_TEST(test_base_controller_request_body_helper_accumulates_and_clears);
    RUN_TEST(test_base_controller_request_body_helper_handles_null_request);
    RUN_TEST(test_base_controller_upload_lifecycle_tracks_files_and_limits);
    RUN_TEST(test_base_controller_upload_append_finish_and_claim);
    RUN_TEST(test_fake_esp_async_web_server_header_is_available);
    RUN_TEST(test_ws_message_builders_create_compact_envelopes);
    RUN_TEST(test_ws_manager_attaches_and_detaches_from_dispatcher);
    RUN_TEST(test_ws_manager_receives_device_events_when_attached);
    RUN_TEST(test_ws_manager_stops_receiving_after_detach);
    RUN_TEST(test_ws_manager_ignores_registry_persistence_cleared_events);
    RUN_TEST(test_ws_manager_broadcasts_snapshots_only_when_clients_are_connected);
    RUN_TEST(test_ws_manager_resyncs_all_device_snapshots_for_new_clients);
    RUN_TEST(test_ws_status_messages_are_serializable);
    return UNITY_END();
}
