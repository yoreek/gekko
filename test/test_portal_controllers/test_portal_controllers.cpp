#include "portal/controllers/BaseController.h"

#include <unity.h>

using namespace ewfm;

class TestController final : public BaseController {
public:
    using BaseController::addErrorEnvelope;
    using BaseController::addSuccessEnvelope;
    using BaseController::BaseController;
    using BaseController::corsAllowHeaders;
    using BaseController::corsAllowMethods;
    using BaseController::dispatch;
    using BaseController::parseBody;

    void index() override {
        indexCalled = true;
    }

    void show() override {
        showCalled = true;
    }

    void create() override {
        createCalled = true;
    }

    void update() override {
        updateCalled = true;
    }

    void destroy() override {
        destroyCalled = true;
    }

    void options() override {
        optionsCalled = true;
    }

    void cmd() override {
        cmdCalled = true;
    }

    void flush() override {
        flushCalled = true;
    }

    bool indexCalled{false};
    bool showCalled{false};
    bool createCalled{false};
    bool updateCalled{false};
    bool destroyCalled{false};
    bool optionsCalled{false};
    bool cmdCalled{false};
    bool flushCalled{false};
    bool parsedBody{false};
};

void test_base_controller_dispatches_action_to_virtual_override() {
    TestController controller(nullptr, BaseController::Action::Options);

    controller.dispatch();

    TEST_ASSERT_TRUE(controller.optionsCalled);
    TEST_ASSERT_FALSE(controller.indexCalled);
    TEST_ASSERT_FALSE(controller.createCalled);
}

void test_base_controller_parses_valid_and_invalid_json_body() {
    TestController controller(nullptr, BaseController::Action::Create);
    uint8_t invalidBody[] = "{bad json";
    controller.dispatch(invalidBody, sizeof(invalidBody) - 1U);
    TEST_ASSERT_FALSE(controller.createCalled);

    TestController validController(nullptr, BaseController::Action::Create);
    uint8_t validBody[] = "{\"name\":\"demo\"}";
    validController.dispatch(validBody, sizeof(validBody) - 1U);
    TEST_ASSERT_TRUE(validController.createCalled);
}

void test_base_controller_builds_standard_envelopes() {
    DynamicJsonDocument okDoc(64);
    TestController::addSuccessEnvelope(okDoc);
    TEST_ASSERT_TRUE(okDoc["success"].as<bool>());

    DynamicJsonDocument errDoc(128);
    TestController::addErrorEnvelope(errDoc, "BAD_JSON", "bad json");
    TEST_ASSERT_FALSE(errDoc["success"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("BAD_JSON", errDoc["code"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("bad json", errDoc["error"].as<const char*>());
}

void test_base_controller_exposes_expected_cors_headers() {
    TEST_ASSERT_EQUAL_STRING("GET, POST, PUT, PATCH, DELETE, OPTIONS", TestController::corsAllowMethods());
    TEST_ASSERT_EQUAL_STRING("Content-Type", TestController::corsAllowHeaders());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_base_controller_dispatches_action_to_virtual_override);
    RUN_TEST(test_base_controller_parses_valid_and_invalid_json_body);
    RUN_TEST(test_base_controller_builds_standard_envelopes);
    RUN_TEST(test_base_controller_exposes_expected_cors_headers);
    return UNITY_END();
}
