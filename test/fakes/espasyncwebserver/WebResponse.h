#pragma once

#include "WebTypes.h"

class AsyncWebServerResponse {
public:
    void addHeader(const char*, const char*) {}
};

class AsyncResponseStream {
public:
    size_t print(char) {
        return 1U;
    }
    size_t print(const char*) {
        return 0U;
    }
    size_t print(const std::string&) {
        return 0U;
    }
    size_t print(unsigned long) {
        return 0U;
    }
    size_t print(long) {
        return 0U;
    }
    size_t print(unsigned int) {
        return 0U;
    }
    size_t print(int) {
        return 0U;
    }
};
