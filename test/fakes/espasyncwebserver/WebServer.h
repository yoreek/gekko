#pragma once

#include "WebRequest.h"
#include "WebUriMatcher.h"

class AsyncWebServer {
public:
    template <typename... Args> void on(Args&&...) {}

    template <typename... Args> void onNotFound(Args&&...) {}

    void begin() {}
    void end() {}
};
