#include <ESPAsyncWebServer.h>
#include <unity.h>

void test_fake_esp_async_web_server_header_is_available() {
    AsyncWebServer server;
    server.begin();
    server.end();

    AsyncWebServerRequest request;
    request.setUrl("/api/test");
    request.setContentLength(42U);

    TEST_ASSERT_EQUAL_STRING("/api/test", request.url());
    TEST_ASSERT_EQUAL_UINT32(42U, static_cast<uint32_t>(request.contentLength()));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_fake_esp_async_web_server_header_is_available);
    return UNITY_END();
}
