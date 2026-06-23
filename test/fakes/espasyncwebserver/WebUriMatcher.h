#pragma once

class AsyncURIMatcher {
public:
    static AsyncURIMatcher exact(const char*) {
        return {};
    }
    static AsyncURIMatcher prefix(const char*) {
        return {};
    }
};
