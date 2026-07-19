#pragma once

#include "WebResponse.h"

class AsyncWebServerRequest {
public:
    void* _tempObject{nullptr};

    WebRequestMethod method() const {
        return method_;
    }
    void setMethod(const WebRequestMethod method) {
        method_ = method;
    }
    const char* url() const {
        return url_.c_str();
    }
    void setUrl(const char* value) {
        url_ = value == nullptr ? "" : value;
    }
    size_t contentLength() const {
        return contentLength_;
    }
    void setContentLength(const size_t value) {
        contentLength_ = value;
    }

    AsyncWebServerResponse* beginResponse(int, const char* = nullptr, const char* = nullptr) {
        return &response_;
    }
    AsyncWebServerResponse* beginResponse(int, const char*, const uint8_t*, size_t) {
        return &response_;
    }
    AsyncResponseStream* beginResponseStream(const char*, size_t = 0U) {
        return &stream_;
    }
    void send(AsyncWebServerResponse*) {}
    void send(AsyncResponseStream*) {}

private:
    WebRequestMethod method_{HTTP_GET};
    std::string url_{};
    size_t contentLength_{0U};
    AsyncWebServerResponse response_{};
    AsyncResponseStream stream_{};
};
