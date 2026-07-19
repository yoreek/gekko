#pragma once

#include <ArduinoJson.h>
#include <cstddef>

namespace ewfm {

class IJsonChunkSink {
public:
    virtual ~IJsonChunkSink() = default;
    virtual bool emit(const char* data, size_t size) = 0;
    virtual bool emitJson(JsonDocument& document, bool leadingComma) = 0;
};

class IJsonChunkProducer {
public:
    virtual ~IJsonChunkProducer() = default;
    virtual bool next(IJsonChunkSink& sink) = 0;
};

} // namespace ewfm
