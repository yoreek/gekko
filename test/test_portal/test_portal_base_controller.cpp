#include "portal/controllers/BaseController.h"

#include <unity.h>

using namespace ewfm;

class TestController final : public BaseController {
public:
    using BaseController::addErrorEnvelope;
    using BaseController::addSuccessEnvelope;
    using BaseController::appendUploadFile;
    using BaseController::BaseController;
    using BaseController::beginUploadFile;
    using BaseController::clearRequestBody;
    using BaseController::corsAllowHeaders;
    using BaseController::corsAllowMethods;
    using BaseController::dispatch;
    using BaseController::finishUploadFile;
    using BaseController::parseBody;
    using BaseController::setUploadTmpDir;
    using BaseController::uploadTmpDir;

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
};

// Mirrors a multipart-upload controller (e.g. MqttController's ca-cert route,
// DeviceSetupTransferController's import route): the file arrives via a separate onUpload
// callback, so the Create dispatch is the zero-arg dispatch() with body_ left null.
class UploadStyleTestController final : public BaseController {
public:
    using BaseController::BaseController;
    using BaseController::dispatch;

    void create() override {
        createCalled = true;
    }

    bool createCalled{false};

protected:
    // Does not chain to BaseController::beforeChain(): that shared chain runs parseBody() for
    // every Action::Create dispatch regardless of whether this particular route ever expects a
    // JSON body, which would reject this dispatch with "invalid body" before create() runs.
    const RulesChain* beforeChain() override {
        static constexpr HookRule rules[] = {
            {&BaseController::beforeCorsOptions, ALL},
        };
        static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), nullptr};
        return &node;
    }
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

void test_base_controller_default_chain_rejects_bodyless_create_dispatch() {
    // Documents the defect: a controller that keeps the default beforeChain() (chains to
    // BaseController::beforeChain()) cannot support a multipart-upload Create route, because the
    // shared parseBody-for-Create rule runs unconditionally and body_ is never set for a
    // zero-arg dispatch() call.
    TestController controller(nullptr, BaseController::Action::Create);
    controller.dispatch();
    TEST_ASSERT_FALSE(controller.createCalled);
}

void test_base_controller_upload_style_chain_allows_bodyless_create_dispatch() {
    // Documents the fix pattern (used by MqttController's ca-cert route and
    // DeviceSetupTransferController's import route): a beforeChain() override that does not
    // chain to the base's parseBody-for-Create rule lets a zero-arg Create dispatch reach
    // create() normally.
    UploadStyleTestController controller(nullptr, BaseController::Action::Create);
    controller.dispatch();
    TEST_ASSERT_TRUE(controller.createCalled);
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

void test_base_controller_request_file_defaults_and_tmp_dir() {
    BaseController::RequestFile file;
    TEST_ASSERT_EQUAL_STRING("", file.tmpPath);
    TEST_ASSERT_EQUAL_STRING("", file.originalFilename);
    TEST_ASSERT_EQUAL_STRING("", file.contentType);
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(file.size));
    TEST_ASSERT_FALSE(file.claimed);
    TEST_ASSERT_FALSE(file.present);

    BaseController::setUploadTmpDir(nullptr);
    TEST_ASSERT_EQUAL_STRING("/tmp", TestController::uploadTmpDir());
    TestController::setUploadTmpDir("/littlefs/uploads");
    TEST_ASSERT_EQUAL_STRING("/littlefs/uploads", TestController::uploadTmpDir());
    TestController::setUploadTmpDir(nullptr);
}

void test_base_controller_request_state_defaults() {
    BaseController::RequestState state;
    TEST_ASSERT_EQUAL_UINT32(BaseController::kRequestStateMagic, state.magic);
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(state.fileCount));
    TEST_ASSERT_EQUAL_STRING("", state.files[0].tmpPath);
    TEST_ASSERT_FALSE(state.files[0].claimed);
    TEST_ASSERT_FALSE(state.files[0].present);
}

void test_base_controller_request_body_helper_accumulates_and_clears() {
    AsyncWebServerRequest request;
    uint8_t chunk1[] = {'h', 'e', 'l', 'l', 'o', ' '};
    uint8_t chunk2[] = {'w', 'o', 'r', 'l', 'd'};

    TEST_ASSERT_FALSE(BaseController::appendRequestBody(&request, chunk1, sizeof(chunk1), 0U, sizeof(chunk1) + sizeof(chunk2)));
    TEST_ASSERT_NOT_NULL(request._tempObject);
    TEST_ASSERT_TRUE(BaseController::appendRequestBody(&request, chunk2, sizeof(chunk2), sizeof(chunk1), sizeof(chunk1) + sizeof(chunk2)));
    TEST_ASSERT_EQUAL_STRING("hello world", static_cast<const char*>(request._tempObject));

    BaseController::clearRequestBody(&request);
    TEST_ASSERT_NULL(request._tempObject);
}

void test_base_controller_request_body_helper_handles_null_request() {
    uint8_t body[] = {1U, 2U, 3U};
    TEST_ASSERT_FALSE(TestController::appendRequestBody(nullptr, body, sizeof(body), 0U, sizeof(body)));
    TestController::clearRequestBody(nullptr);
}

void test_base_controller_upload_lifecycle_tracks_files_and_limits() {
    AsyncWebServerRequest request;
    TestController::setUploadTmpDir("/littlefs/uploads");

    {
        TestController controller(&request, BaseController::Action::Create);

        size_t fileIndex = 999U;
        TEST_ASSERT_TRUE(TestController::beginUploadFile(&request, "alpha.bin", "application/octet-stream", 12U, fileIndex));
        TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(fileIndex));

        size_t secondIndex = 999U;
        TEST_ASSERT_TRUE(TestController::beginUploadFile(&request, "bravo.txt", "text/plain", 7U, secondIndex));
        TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(secondIndex));

        size_t thirdIndex = 999U;
        TEST_ASSERT_TRUE(TestController::beginUploadFile(&request, "charlie.json", "application/json", 5U, thirdIndex));
        TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(thirdIndex));

        size_t overflowIndex = 999U;
        TEST_ASSERT_FALSE(TestController::beginUploadFile(&request, "overflow.bin", "text/plain", 1U, overflowIndex));

        TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(controller.fileCount()));
        const BaseController::RequestFile* files = controller.files();
        TEST_ASSERT_NOT_NULL(files);
        TEST_ASSERT_EQUAL_STRING("/littlefs/uploads/upload-1.bin", files[0].tmpPath);
        TEST_ASSERT_EQUAL_STRING("alpha.bin", files[0].originalFilename);
        TEST_ASSERT_EQUAL_STRING("application/octet-stream", files[0].contentType);
        TEST_ASSERT_EQUAL_UINT32(12U, static_cast<uint32_t>(files[0].size));
        TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(files[0].received));
        TEST_ASSERT_TRUE(files[0].present);
        TEST_ASSERT_FALSE(files[0].claimed);

        TEST_ASSERT_TRUE(controller.moveFile(0U, "/littlefs/uploads/final-alpha.bin"));
        files = controller.files();
        TEST_ASSERT_EQUAL_STRING("/littlefs/uploads/final-alpha.bin", files[0].tmpPath);
        TEST_ASSERT_TRUE(files[0].claimed);
    }

    TEST_ASSERT_NULL(request._tempObject);
    TestController::setUploadTmpDir(nullptr);
}

void test_base_controller_upload_append_finish_and_claim() {
    AsyncWebServerRequest request;
    TestController::setUploadTmpDir("/littlefs/uploads");

    {
        TestController controller(&request, BaseController::Action::Create);

        size_t fileIndex = 0U;
        TEST_ASSERT_TRUE(TestController::beginUploadFile(&request, "payload.bin", "application/octet-stream", 11U, fileIndex));

        const uint8_t firstChunk[] = {'h', 'e', 'l', 'l', 'o', ' '};
        const uint8_t secondChunk[] = {'w', 'o', 'r', 'l', 'd'};
        TEST_ASSERT_TRUE(TestController::appendUploadFile(&request, fileIndex, firstChunk, sizeof(firstChunk), 0U, 11U));
        TEST_ASSERT_TRUE(TestController::appendUploadFile(&request, fileIndex, secondChunk, sizeof(secondChunk), sizeof(firstChunk), 11U));
        TEST_ASSERT_TRUE(TestController::finishUploadFile(&request, fileIndex, true));

        TEST_ASSERT_TRUE(controller.claimFile(fileIndex));
        const BaseController::RequestFile* files = controller.files();
        TEST_ASSERT_NOT_NULL(files);
        TEST_ASSERT_EQUAL_UINT32(11U, static_cast<uint32_t>(files[0].received));
        TEST_ASSERT_TRUE(files[0].claimed);
        TEST_ASSERT_TRUE(files[0].present);
    }

    TEST_ASSERT_NULL(request._tempObject);
    TestController::setUploadTmpDir(nullptr);
}
